#include "gfx/imgui.hpp"

#include <cstdlib>
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"
#include "vuk/AllocatorHelpers.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/Pipeline.hpp"
#include "vuk/Partials.hpp"
#include "util/verify.hpp"
#include "util/array.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/log.hpp"
#include "gfx/samplers.hpp"
#include "gfx/engine.hpp"
#include "gfx/util.hpp"
#include "sys/vulkan.hpp"

#include "spv/imgui.vs.hpp"
#include "spv/imgui.ps.hpp"

namespace minote {

bool Imgui::InputReader::process(SDL_Event const& _e) {
	
	ImGui_ImplSDL2_ProcessEvent(&_e);
	
	if (_e.type == SDL_KEYDOWN)
		if (ImGui::GetIO().WantCaptureKeyboard) return true;
	if (_e.type == SDL_MOUSEBUTTONDOWN || _e.type == SDL_MOUSEMOTION)
		if (ImGui::GetIO().WantCaptureMouse) return true;
	return false;
	
}

Imgui::Imgui(vuk::Allocator& _allocator):
	m_insideFrame(false) {
	
	ASSUME(!m_exists);
	m_exists = true;
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForVulkan(s_vulkan->window().handle());
	
	auto& io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_vuk";
	// We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	
	setTheme();
	uploadFont(_allocator); // s_engine might not be initialized at this point
	
	auto& ctx = *s_vulkan->context;
	auto imguiPci = vuk::PipelineBaseCreateInfo();
	addSpirv(imguiPci, imgui_vs, "imgui.vs.hlsl");
	addSpirv(imguiPci, imgui_ps, "imgui.ps.hlsl");
	ctx.create_named_pipeline("imgui", imguiPci);
	
	L_DEBUG("ImGui initialized");
	
}

Imgui::~Imgui() {
	
	ImGui_ImplSDL2_Shutdown();
	m_exists = false;
	
}

auto Imgui::getInputReader() -> InputReader {
	
	auto lock = std::lock_guard(m_stateLock);
	ImGui_ImplSDL2_NewFrame();
	return InputReader();
	
}

void Imgui::begin() {
	
	auto lock = std::lock_guard(m_stateLock);
	if (m_insideFrame) return;
	
	ImGui::NewFrame();
	m_insideFrame = true;
	
}

auto Imgui::render(vuk::Future _target) -> vuk::Future {
	
	auto lock = std::lock_guard(m_stateLock);
	if (!m_insideFrame) begin();
	
	// Finalize the frame
	ImGui::Render();
	auto* drawdata = ImGui::GetDrawData();
	
	// Fill up the vertex and index buffers
	auto vertexSize = drawdata->TotalVtxCount * sizeof(ImDrawVert);
	auto  indexSize = drawdata->TotalIdxCount * sizeof(ImDrawIdx);
	auto imvert = *vuk::allocate_buffer_cross_device(s_engine->frameAllocator(),
		{vuk::MemoryUsage::eCPUtoGPU, vertexSize});
	auto  imind = *vuk::allocate_buffer_cross_device(s_engine->frameAllocator(),
		{vuk::MemoryUsage::eCPUtoGPU, indexSize});
	
	auto vtxDst = 0_zu;
	auto idxDst = 0_zu;
	for (auto* list: span(drawdata->CmdLists, drawdata->CmdListsCount)) {
		auto imverto = imvert->add_offset(vtxDst * sizeof(ImDrawVert));
		auto  imindo =  imind->add_offset(idxDst * sizeof(ImDrawIdx));
		std::memcpy(imverto.mapped_ptr, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert));
		std::memcpy( imindo.mapped_ptr, list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += list->VtxBuffer.Size;
		idxDst += list->IdxBuffer.Size;
	}
	
	auto rg = std::make_shared<vuk::RenderGraph>();
	rg->attach_in("input", _target);
	
	// Execute all imgui draws in this pass
	rg->add_pass(vuk::Pass{
		.name = "imgui",
		.resources = {
			"input"_image >> vuk::eColorRW >> "output",
		},
		.execute = [this, imvert=imvert.get(), imind=imind.get(), drawdata](vuk::CommandBuffer& cmd) {
			
			cmd.bind_graphics_pipeline("imgui")
			   .set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
			   .set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_rasterization({})
			   .broadcast_color_blend(vuk::BlendPreset::eAlphaBlend);
			
			cmd.bind_image(0, 0, *m_font.view).bind_sampler(0, 0, TrilinearRepeat);
			constexpr auto ImguiIndexSize = 
				sizeof(ImDrawIdx) == 2?
					vuk::IndexType::eUint16 :
					vuk::IndexType::eUint32;
			if (imind.size > 0)
				cmd.bind_index_buffer(imind, ImguiIndexSize);
			cmd.bind_vertex_buffer(0, imvert, 0, vuk::Packed{
				vuk::Format::eR32G32Sfloat,
				vuk::Format::eR32G32Sfloat,
				vuk::Format::eR8G8B8A8Unorm,
			});
			
			struct Constants {
				vec2 scale;
				vec2 translate;
			};
			auto scale = vec2{
				2.0f / drawdata->DisplaySize.x,
				2.0f / drawdata->DisplaySize.y,
			};
			auto constants = Constants{
				.scale = scale,
				.translate = vec2(-1.0f) - vec2{drawdata->DisplayPos.x, drawdata->DisplayPos.y} * scale,
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, constants);
			
			// Will project scissor/clipping rectangles into framebuffer space
			auto clipOff = drawdata->DisplayPos;         // (0,0) unless using multi-viewports
			auto clipScale = drawdata->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
			
			// Render command lists
			// (Because we merged all buffers into a single one, we maintain our own offset into them)
			auto globalVtxOffset = 0;
			auto globalIdxOffset = 0;
			for (auto* list: span(drawdata->CmdLists, drawdata->CmdListsCount)) {
				for (auto& pcmd: list->CmdBuffer) {
					
					// Project scissor/clipping rectangles into framebuffer space
					auto clipRect = vec4{
						(pcmd.ClipRect.x - clipOff.x) * clipScale.x,
						(pcmd.ClipRect.y - clipOff.y) * clipScale.y,
						(pcmd.ClipRect.z - clipOff.x) * clipScale.x,
						(pcmd.ClipRect.w - clipOff.y) * clipScale.y,
					};
					auto fbWidth = cmd.get_ongoing_renderpass().extent.width;
					auto fbHeight = cmd.get_ongoing_renderpass().extent.height;
					if (clipRect.x() >= fbWidth || clipRect.y() >= fbHeight ||
						clipRect.z() <     0.0f || clipRect.w() <  0.0f) continue;
					
					// Negative offsets are illegal for vkCmdSetScissor
					clipRect.x() = max(clipRect.x(), 0.0f);
					clipRect.y() = max(clipRect.y(), 0.0f);
					cmd.set_scissor(0, vuk::Rect2D{
						.offset = {i32(clipRect.x()), i32(clipRect.y())},
						.extent = {u32(clipRect.z() - clipRect.x()), u32(clipRect.w() - clipRect.y())},
					});
					
					// Bind texture
					if (pcmd.TextureId) {
						auto& view = *reinterpret_cast<vuk::ImageView*>(pcmd.TextureId);
						cmd.bind_image(0, 0, view, vuk::ImageLayout::eShaderReadOnlyOptimal).bind_sampler(0, 0, TrilinearClamp);
					}
					
					// Draw
					cmd.draw_indexed(pcmd.ElemCount, 1, pcmd.IdxOffset + globalIdxOffset, pcmd.VtxOffset + globalVtxOffset, 0);
					
				}
				globalIdxOffset += list->IdxBuffer.Size;
				globalVtxOffset += list->VtxBuffer.Size;
				
			}
		}
	});
	
	m_insideFrame = false;
	return vuk::Future(rg, "output");
	
	
}

void Imgui::setTheme() {
	
	ImGui::GetStyle().FrameRounding = 4.0f;
	ImGui::GetStyle().GrabRounding = 4.0f;
	
	auto* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	
}

void Imgui::uploadFont(vuk::Allocator& _allocator) {
	
	auto& ctx = _allocator.get_context();
	auto& io = ImGui::GetIO();
	
	// Retrieve font bitmap
	auto* pixels = static_cast<unsigned char*>(nullptr);
	auto width = 0;
	auto height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	
	// Upload the font to GPU
	auto [font, stub] = vuk::create_texture(_allocator,
		vuk::Format::eR8G8B8A8Srgb,
		vuk::Extent3D{u32(width), u32(height), 1u},
		pixels, false);
	m_font = std::move(font);
	stub.wait(_allocator);
	ctx.debug.set_name(m_font, "imgui/font");
	io.Fonts->TexID = ImTextureID(&m_font.view.get());
	
}

}
