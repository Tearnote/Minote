#include "gfx/imgui.hpp"

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

Imgui::Imgui(vuk::Allocator& _allocator, uvec2 _viewport):
	m_insideFrame(false) {
	
	ASSUME(!m_exists);
	m_exists = true;
	
	auto& ctx = _allocator.get_context();
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForVulkan(s_vulkan->window().handle());
	
	auto& io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_vuk";
	// We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.DisplaySize = ImVec2{
		f32(_viewport.x()),
		f32(_viewport.y()),
	};
	
	setTheme();
	uploadFont(_allocator);
	
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

void Imgui::render(vuk::Allocator& _allocator, vuk::RenderGraph& _rg,
	vuk::Name _targetFrom, vuk::Name _targetTo,
	ivector<vuk::SampledImage> const& _sampledImages) {
	
	auto lock = std::lock_guard(m_stateLock);
	if (!m_insideFrame) begin();
	
	ImGui::Render();
	auto* drawdata = ImGui::GetDrawData();
	
	auto resetRenderState = [this, drawdata](vuk::CommandBuffer& cmd,
		vuk::Buffer vertex, vuk::Buffer index) {
		
		cmd.bind_image(0, 0, *m_font.view).bind_sampler(0, 0, TrilinearRepeat);
		if (index.size > 0)
			cmd.bind_index_buffer(index, sizeof(ImDrawIdx) == 2?
				vuk::IndexType::eUint16 :
				vuk::IndexType::eUint32);
		cmd.bind_vertex_buffer(0, vertex, 0, vuk::Packed{
			vuk::Format::eR32G32Sfloat,
			vuk::Format::eR32G32Sfloat,
			vuk::Format::eR8G8B8A8Unorm,
		});
		cmd.bind_graphics_pipeline("imgui");
		cmd.set_viewport(0, vuk::Rect2D::framebuffer());
		
		struct Constants {
			vec2 scale;
			vec2 translate;
		};
		auto constants = Constants{
			.scale = {
				2.0f / drawdata->DisplaySize.x,
				2.0f / drawdata->DisplaySize.y,
			}
		};
		constants.translate = {
			-1.0f - drawdata->DisplayPos.x * constants.scale[0],
			-1.0f - drawdata->DisplayPos.y * constants.scale[1],
		};
		cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, constants);
		
	};
	
	auto vertexSize = drawdata->TotalVtxCount * sizeof(ImDrawVert);
	auto  indexSize = drawdata->TotalIdxCount * sizeof(ImDrawIdx);
	auto imvert = *allocate_buffer_cross_device(_allocator, { vuk::MemoryUsage::eCPUtoGPU, vertexSize });
	auto  imind = *allocate_buffer_cross_device(_allocator, { vuk::MemoryUsage::eCPUtoGPU, indexSize });
	
	auto vtxDst = 0_zu;
	auto idxDst = 0_zu;
	for (auto* list: span(drawdata->CmdLists, drawdata->CmdListsCount)) {
		
		auto imverto = imvert->add_offset(vtxDst * sizeof(ImDrawVert));
		auto  imindo =  imind->add_offset(idxDst * sizeof(ImDrawIdx));
		vuk::host_data_to_buffer(_allocator, vuk::DomainFlagBits{}, imverto,
			std::span(list->VtxBuffer.Data, list->VtxBuffer.Size)).wait(_allocator);
		vuk::host_data_to_buffer(_allocator, vuk::DomainFlagBits{}, imindo,
			std::span(list->IdxBuffer.Data, list->IdxBuffer.Size)).wait(_allocator);
		
		vtxDst += list->VtxBuffer.Size;
		idxDst += list->IdxBuffer.Size;
		
	}
	
	auto resources = std::vector<vuk::Resource>();
	resources.emplace_back(vuk::Resource{ _targetFrom, vuk::Resource::Type::eImage, vuk::eColorRW, _targetTo });
	for (auto& si: _sampledImages)
		if (!si.is_global)
			resources.emplace_back(vuk::Resource{ si.rg_attachment.attachment_name, vuk::Resource::Type::eImage, vuk::Access::eFragmentSampled });
	
	auto pass = vuk::Pass{
		.name = "Imgui",
		.resources = std::move(resources),
		.execute = [this, &_allocator, imvert=imvert.get(), imind=imind.get(),
			drawdata, resetRenderState, _targetFrom](vuk::CommandBuffer& cmd) {
			
			cmd.set_dynamic_state(vuk::DynamicStateFlagBits::eScissor);
			cmd.set_rasterization(vuk::PipelineRasterizationStateCreateInfo{});
			cmd.set_color_blend(_targetFrom, vuk::BlendPreset::eAlphaBlend);
			resetRenderState(cmd, imvert, imind);
			
			// Will project scissor/clipping rectangles into framebuffer space
			auto clipOff = drawdata->DisplayPos;         // (0,0) unless using multi-viewports
			auto clipScale = drawdata->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
			
			// Render command lists
			// (Because we merged all buffers into a single one, we maintain our own offset into them)
			auto globalVtxOffset = 0;
			auto globalIdxOffset = 0;
			for (auto* list: span(drawdata->CmdLists, drawdata->CmdListsCount)) {
				for (auto cmd_i: iota(0, list->CmdBuffer.Size)) {
					
					auto* pcmd = &list->CmdBuffer[cmd_i];
					if (pcmd->UserCallback) {
						
						// User callback, registered via ImDrawList::AddCallback()
						// (ImDrawCallback_ResetRenderState is a special callback value
						// used by the user to request the renderer to reset render state.)
						if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
							resetRenderState(cmd, imvert, imind);
						else
							pcmd->UserCallback(list, pcmd);
							
					} else {
						
						// Project scissor/clipping rectangles into framebuffer space
						auto clipRect = ImVec4{
							(pcmd->ClipRect.x - clipOff.x) * clipScale.x,
							(pcmd->ClipRect.y - clipOff.y) * clipScale.y,
							(pcmd->ClipRect.z - clipOff.x) * clipScale.x,
							(pcmd->ClipRect.w - clipOff.y) * clipScale.y,
						};
						
						auto fbWidth = cmd.get_ongoing_renderpass().extent.width;
						auto fbHeight = cmd.get_ongoing_renderpass().extent.height;
						if (clipRect.x < fbWidth && clipRect.y < fbHeight &&
							clipRect.z >=   0.0f && clipRect.w >= 0.0f) {
							
							// Negative offsets are illegal for vkCmdSetScissor
							if (clipRect.x < 0.0f)
								clipRect.x = 0.0f;
							if (clipRect.y < 0.0f)
								clipRect.y = 0.0f;
							
							// Apply scissor/clipping rectangle
							auto scissor = vuk::Rect2D{
								.offset = {i32(clipRect.x), i32(clipRect.y)},
								.extent = {u32(clipRect.z - clipRect.x), u32(clipRect.w - clipRect.y)},
							};
							cmd.set_scissor(0, scissor);
							
							// Bind texture
							if (pcmd->TextureId) {
								auto& si = *reinterpret_cast<vuk::SampledImage*>(pcmd->TextureId);
								if (si.is_global) {
									cmd.bind_image(0, 0, si.global.iv).bind_sampler(0, 0, si.global.sci);
								} else {
									if (si.rg_attachment.ivci) {
										auto ivci = *si.rg_attachment.ivci;
										auto resImg = cmd.get_resource_image(si.rg_attachment.attachment_name);
										ivci.image = *resImg;
										auto iv = vuk::allocate_image_view(_allocator, ivci);
										cmd.bind_image(0, 0, **iv).bind_sampler(0, 0, si.rg_attachment.sci);
									} else {
										cmd.bind_image(0, 0, si.rg_attachment.attachment_name).bind_sampler(0, 0, si.rg_attachment.sci);
									}
								}
							}
							
							// Draw
							cmd.draw_indexed(pcmd->ElemCount, 1, pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset, 0);
							
						}
						
					}
					
				}
				globalIdxOffset += list->IdxBuffer.Size;
				globalVtxOffset += list->VtxBuffer.Size;
				
			}
			
		}
		
	};
	
	_rg.add_pass(std::move(pass));
	
	m_insideFrame = false;
	
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
	
	m_fontSI = std::make_unique<vuk::SampledImage>(vuk::SampledImage::Global{ *m_font.view, TrilinearRepeat, vuk::ImageLayout::eShaderReadOnlyOptimal });
	io.Fonts->TexID = ImTextureID(m_fontSI.get());
	
}

}
