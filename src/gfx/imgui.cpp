#include "gfx/imgui.hpp"

#include "vuk/AllocatorHelpers.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/Pipeline.hpp"
#include "vuk/Partials.hpp"
#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

auto ImGui_ImplVuk_Init(vuk::Allocator& _allocator) -> ImguiData {
	
	auto& ctx = _allocator.get_context();
	
	// Set theme
	
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
	
	// Basic properties
	
	auto& io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_vuk";
	// We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	
	// Create the font texture
	
	// Retrieve default font bitmap
	auto* pixels = static_cast<unsigned char*>(nullptr);
	auto width = 0;
	auto height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	
	// Upload the font to GPU
	auto data = ImguiData();
	auto [tex, stub] = vuk::create_texture(_allocator,
		vuk::Format::eR8G8B8A8Srgb,
		vuk::Extent3D{u32(width), u32(height), 1u},
		pixels, false);
	data.fontTex = std::move(tex);
	stub.wait(_allocator);
	ctx.debug.set_name(data.fontTex, "ImGui/font");
	
	data.fontSI = std::make_unique<vuk::SampledImage>(vuk::SampledImage::Global{ *data.fontTex.view, TrilinearRepeat, vuk::ImageLayout::eShaderReadOnlyOptimal });
	io.Fonts->TexID = ImTextureID(data.fontSI.get());
	
	// Create the ImGui pipeline
	
	auto imguiPci = vuk::PipelineBaseCreateInfo();
	imguiPci.add_spirv(std::vector<u32>{
#include "spv/imgui.vert.spv"
	}, "imgui.vert");
	imguiPci.add_spirv(std::vector<u32>{
#include "spv/imgui.frag.spv"
	}, "imgui.frag");
	ctx.create_named_pipeline("imgui", imguiPci);
	
	L_DEBUG("ImGui initialized");
	
	return data;
	
}

void ImGui_ImplVuk_Render(vuk::Allocator& _allocator, vuk::RenderGraph& _rg,
	vuk::Name _targetFrom, vuk::Name _targetTo,
	ImguiData& _imdata, ImDrawData* _drawdata,
	ivector<vuk::SampledImage> const& _sampledImages) {
	
	auto reset_render_state = [](const ImguiData& imdata, vuk::CommandBuffer& cmd,
		ImDrawData* drawdata, vuk::Buffer vertex, vuk::Buffer index) {
		
		cmd.bind_image(0, 0, *imdata.fontTex.view).bind_sampler(0, 0, TrilinearRepeat);
		if (index.size > 0)
			cmd.bind_index_buffer(index, sizeof(ImDrawIdx) == 2?
				vuk::IndexType::eUint16 :
				vuk::IndexType::eUint32);
		cmd.bind_vertex_buffer(0, vertex, 0, vuk::Packed{
			vuk::Format::eR32G32Sfloat,
			vuk::Format::eR32G32Sfloat,
			vuk::Format::eR8G8B8A8Unorm });
		cmd.bind_graphics_pipeline("imgui");
		cmd.set_viewport(0, vuk::Rect2D::framebuffer());
		
		struct PC {
			vec2 scale;
			vec2 translate;
		};
		auto pc = PC();
		pc.scale = {
			2.0f / drawdata->DisplaySize.x,
			2.0f / drawdata->DisplaySize.y};
		pc.translate = {
			-1.0f - drawdata->DisplayPos.x * pc.scale[0],
			-1.0f - drawdata->DisplayPos.y * pc.scale[1] };
		cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, pc);
		
	};
	
	auto vertexSize = _drawdata->TotalVtxCount * sizeof(ImDrawVert);
	auto  indexSize = _drawdata->TotalIdxCount * sizeof(ImDrawIdx);
	auto imvert = *allocate_buffer_cross_device(_allocator, { vuk::MemoryUsage::eCPUtoGPU, vertexSize });
	auto  imind = *allocate_buffer_cross_device(_allocator, { vuk::MemoryUsage::eCPUtoGPU, indexSize });
	
	auto vtxDst = 0_zu;
	auto idxDst = 0_zu;
	for (auto n: iota(0, _drawdata->CmdListsCount)) {
		
		auto* cmdList = _drawdata->CmdLists[n];
		auto imverto = imvert->add_offset(vtxDst * sizeof(ImDrawVert));
		auto  imindo =  imind->add_offset(idxDst * sizeof(ImDrawIdx));
		vuk::host_data_to_buffer(_allocator, vuk::DomainFlagBits{}, imverto,
			std::span(cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size)).wait(_allocator);
		vuk::host_data_to_buffer(_allocator, vuk::DomainFlagBits{},  imindo,
			std::span(cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size)).wait(_allocator);
		
		vtxDst += cmdList->VtxBuffer.Size;
		idxDst += cmdList->IdxBuffer.Size;
		
	}
	
	auto resources = std::vector<vuk::Resource>();
	resources.emplace_back(vuk::Resource{ _targetFrom, vuk::Resource::Type::eImage, vuk::eColorRW, _targetTo });
	for (auto& si: _sampledImages)
		if (!si.is_global)
			resources.emplace_back(vuk::Resource{ si.rg_attachment.attachment_name, vuk::Resource::Type::eImage, vuk::Access::eFragmentSampled });
	
	auto pass = vuk::Pass{
		.name = "Imgui",
		.resources = std::move(resources),
		.execute = [&_imdata, &_allocator, imvert=imvert.get(), imind=imind.get(),
			_drawdata, reset_render_state, _targetFrom](vuk::CommandBuffer& cmd) {
			
			cmd.set_dynamic_state(vuk::DynamicStateFlagBits::eScissor);
			cmd.set_rasterization(vuk::PipelineRasterizationStateCreateInfo{});
			cmd.set_color_blend(_targetFrom, vuk::BlendPreset::eAlphaBlend);
			reset_render_state(_imdata, cmd, _drawdata, imvert, imind);
			
			// Will project scissor/clipping rectangles into framebuffer space
			auto clip_off = _drawdata->DisplayPos;         // (0,0) unless using multi-viewports
			auto clip_scale = _drawdata->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
			
			// Render command lists
			// (Because we merged all buffers into a single one, we maintain our own offset into them)
			int global_vtx_offset = 0;
			int global_idx_offset = 0;
			for (auto n: iota(0, _drawdata->CmdListsCount)) {
				
				auto* cmd_list = _drawdata->CmdLists[n];
				for (auto cmd_i: iota(0, cmd_list->CmdBuffer.Size)) {
					
					auto* pcmd = &cmd_list->CmdBuffer[cmd_i];
					if (pcmd->UserCallback != nullptr) {
						
						// User callback, registered via ImDrawList::AddCallback()
						// (ImDrawCallback_ResetRenderState is a special callback value
						// used by the user to request the renderer to reset render state.)
						if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
							reset_render_state(_imdata, cmd, _drawdata, imvert, imind);
						else
							pcmd->UserCallback(cmd_list, pcmd);
							
					} else {
						
						// Project scissor/clipping rectangles into framebuffer space
						auto clip_rect = ImVec4{
							(pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
							(pcmd->ClipRect.y - clip_off.y) * clip_scale.y,
							(pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
							(pcmd->ClipRect.w - clip_off.y) * clip_scale.y};
						
						auto fb_width = cmd.get_ongoing_renderpass().extent.width;
						auto fb_height = cmd.get_ongoing_renderpass().extent.height;
						if (clip_rect.x < fb_width && clip_rect.y < fb_height &&
							clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
							
							// Negative offsets are illegal for vkCmdSetScissor
							if (clip_rect.x < 0.0f)
								clip_rect.x = 0.0f;
							if (clip_rect.y < 0.0f)
								clip_rect.y = 0.0f;
							
							// Apply scissor/clipping rectangle
							auto scissor = vuk::Rect2D{
								.offset = {i32(clip_rect.x), i32(clip_rect.y)},
								.extent = {u32(clip_rect.z - clip_rect.x), u32(clip_rect.w - clip_rect.y)} };
							cmd.set_scissor(0, scissor);
							
							// Bind texture
							if (pcmd->TextureId) {
								
								auto& si = *reinterpret_cast<vuk::SampledImage*>(pcmd->TextureId);
								if (si.is_global) {
									
									cmd.bind_image(0, 0, si.global.iv).bind_sampler(0, 0, si.global.sci);
									
								} else {
									
									if (si.rg_attachment.ivci) {
										
										auto ivci = *si.rg_attachment.ivci;
										auto res_img = cmd.get_resource_image(si.rg_attachment.attachment_name);
										ivci.image = *res_img;
										auto iv = vuk::allocate_image_view(_allocator, ivci);
										cmd.bind_image(0, 0, **iv).bind_sampler(0, 0, si.rg_attachment.sci);
										
									} else {
										
										cmd.bind_image(0, 0, si.rg_attachment.attachment_name).bind_sampler(0, 0, si.rg_attachment.sci);
										
									}
									
								}
								
							}
							
							// Draw
							cmd.draw_indexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
							
						}
						
					}
					
				}
				global_idx_offset += cmd_list->IdxBuffer.Size;
				global_vtx_offset += cmd_list->VtxBuffer.Size;
				
			}
			
		}
		
	};
	
	_rg.add_pass(std::move(pass));
	
}

}
