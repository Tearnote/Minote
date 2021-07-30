#include "gfx/imgui.hpp"

#include "vuk/CommandBuffer.hpp"
#include "vuk/Context.hpp"
#include "base/container/array.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "base/math.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

auto LinearImVec4(f32 r, f32 g, f32 b, f32 a) -> ImVec4 {
	
	constexpr auto power = 2.2f;
	
	return ImVec4(
		pow(r, power),
		pow(g, power),
		pow(b, power),
		pow(a, power));
	
};

auto ImGui_ImplVuk_Init(vuk::PerThreadContext& _ptc) -> ImguiData {
	
	// Set theme
	
	ImGui::GetStyle().FrameRounding = 4.0f;
	ImGui::GetStyle().GrabRounding = 4.0f;
	
	auto* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = LinearImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled] = LinearImVec4(0.36f, 0.42f, 0.47f, 1.00f);
	colors[ImGuiCol_WindowBg] = LinearImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_ChildBg] = LinearImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_PopupBg] = LinearImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = LinearImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_BorderShadow] = LinearImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = LinearImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = LinearImVec4(0.12f, 0.20f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = LinearImVec4(0.09f, 0.12f, 0.14f, 1.00f);
	colors[ImGuiCol_TitleBg] = LinearImVec4(0.09f, 0.12f, 0.14f, 0.65f);
	colors[ImGuiCol_TitleBgActive] = LinearImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = LinearImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = LinearImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = LinearImVec4(0.02f, 0.02f, 0.02f, 0.39f);
	colors[ImGuiCol_ScrollbarGrab] = LinearImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = LinearImVec4(0.18f, 0.22f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = LinearImVec4(0.09f, 0.21f, 0.31f, 1.00f);
	colors[ImGuiCol_CheckMark] = LinearImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = LinearImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = LinearImVec4(0.37f, 0.61f, 1.00f, 1.00f);
	colors[ImGuiCol_Button] = LinearImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = LinearImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_ButtonActive] = LinearImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = LinearImVec4(0.20f, 0.25f, 0.29f, 0.55f);
	colors[ImGuiCol_HeaderHovered] = LinearImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = LinearImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = LinearImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = LinearImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = LinearImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = LinearImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = LinearImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = LinearImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = LinearImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabHovered] = LinearImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_TabActive] = LinearImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = LinearImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = LinearImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_PlotLines] = LinearImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = LinearImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = LinearImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = LinearImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = LinearImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = LinearImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = LinearImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = LinearImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = LinearImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = LinearImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	
	// Basic properties
	
	auto& io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_vuk";
	// We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	
	// Create the font texture
	
	// Retrieve default font bitmap
	auto* pixels = (unsigned char*)(nullptr);
	auto width = int(0);
	auto height = int(0);
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	
	// Upload the font to GPU
	auto data = ImguiData();
	data.fontTex = _ptc.create_texture(
		vuk::Format::eR8G8B8A8Srgb,
		vuk::Extent3D{u32(width), u32(height), 1u},
		pixels).first;
	_ptc.ctx.debug.set_name(data.fontTex, "ImGui/font");
	
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
	imguiPci.set_blend(vuk::BlendPreset::eAlphaBlend);
	_ptc.ctx.create_named_pipeline("imgui", imguiPci);
	
	return data;
	
}

void ImGui_ImplVuk_Render(vuk::PerThreadContext& _ptc, vuk::RenderGraph& _rg,
	vuk::Name _source, vuk::Name _target, ImguiData& _imdata, ImDrawData* _drawdata) {
	
	// This is a real mess, but it works. Mostly copypasted from vuk example code
	// and reformatted.
	
	auto reset_render_state = [](const ImguiData& imdata, vuk::CommandBuffer& cmd,
		ImDrawData* drawdata, vuk::Buffer vertex, vuk::Buffer index) {
		
		cmd.bind_sampled_image(0, 0, *imdata.fontTex.view, TrilinearRepeat);
		if (index.size > 0)
			cmd.bind_index_buffer(index, sizeof(ImDrawIdx) == 2? vuk::IndexType::eUint16 : vuk::IndexType::eUint32);
		cmd.bind_vertex_buffer(0, vertex, 0, vuk::Packed{
			vuk::Format::eR32G32Sfloat,
			vuk::Format::eR32G32Sfloat,
			vuk::Format::eR8G8B8A8Unorm});
		cmd.bind_graphics_pipeline("imgui");
		cmd.set_viewport(0, vuk::Rect2D::framebuffer());
		
		struct PC {
			
			sarray<f32, 2> scale;
			sarray<f32, 2> translate;
			
		} pc;
		pc.scale[0] = 2.0f / drawdata->DisplaySize.x;
		pc.scale[1] = 2.0f / drawdata->DisplaySize.y;
		pc.translate[0] = -1.0f - drawdata->DisplayPos.x * pc.scale[0];
		pc.translate[1] = -1.0f - drawdata->DisplayPos.y * pc.scale[1];
		cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, pc);
		
	};
	
	auto vertex_size = _drawdata->TotalVtxCount * sizeof(ImDrawVert);
	auto index_size = _drawdata->TotalIdxCount * sizeof(ImDrawIdx);
	auto imvert = _ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer | vuk::BufferUsageFlagBits::eTransferDst,
		vertex_size, 1);
	auto imind = _ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eIndexBuffer | vuk::BufferUsageFlagBits::eTransferDst,
		index_size, 1);
	
	auto vtx_dst = 0_zu;
	auto idx_dst = 0_zu;
	for (auto n: iota(0, _drawdata->CmdListsCount)) {
		
		auto* cmd_list = _drawdata->CmdLists[n];
		auto imverto = imvert;
		imverto.offset += vtx_dst * sizeof(ImDrawVert);
		auto imindo = imind;
		imindo.offset += idx_dst * sizeof(ImDrawIdx);
	
		_ptc.upload(imverto, std::span(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size));
		_ptc.upload(imindo, std::span(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
		
	}
	
	_ptc.wait_all_transfers();
	
	auto pass = vuk::Pass{
		.name = "Imgui",
		.resources = { vuk::Resource(_target, vuk::Resource::Type::eImage, vuk::eColorRW) },
		.execute = [&_imdata, imvert, imind, _drawdata, reset_render_state](vuk::CommandBuffer& cmd) {
			
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
									
									cmd.bind_sampled_image(0, 0, si.global.iv, si.global.sci);
									
								} else {
									
									if (si.rg_attachment.ivci)
										cmd.bind_sampled_image(0, 0, si.rg_attachment.attachment_name, *si.rg_attachment.ivci, si.rg_attachment.sci);
									else
										cmd.bind_sampled_image(0, 0, si.rg_attachment.attachment_name, si.rg_attachment.sci);
									
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
	
	// add rendergraph dependencies to be transitioned
	// make all rendergraph sampled images available
	for (auto& si: _ptc.get_sampled_images()) {
		
		if (!si.is_global)
			pass.resources.push_back(vuk::Resource(si.rg_attachment.attachment_name, vuk::Resource::Type::eImage, vuk::Access::eFragmentSampled));
			
	}
	
	_rg.add_pass(std::move(pass));
	_rg.add_alias(_target, _source);
	
}

}
