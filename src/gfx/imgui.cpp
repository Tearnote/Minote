#include "gfx/imgui.hpp"

#include <cmath>
#include "vuk/CommandBuffer.hpp"
#include "vuk/Context.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

// Functions copied and customized from vuk examples source

auto LinearImVec4(float r, float g, float b, float a) -> ImVec4 {
	
	constexpr auto power = 2.2f;
	
	return ImVec4(
		std::pow(r, power),
		std::pow(g, power),
		std::pow(b, power),
		std::pow(a, power));
	
};

ImguiData ImGui_ImplVuk_Init(vuk::PerThreadContext& ptc) {
	
	// Set theme
	ImGui::GetStyle().FrameRounding = 4.0f;
	ImGui::GetStyle().GrabRounding = 4.0f;
	
	ImVec4* colors = ImGui::GetStyle().Colors;
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
	
	auto& io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_vuk";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	ImguiData data;
	auto [tex, stub] = ptc.create_texture(vuk::Format::eR8G8B8A8Srgb, vuk::Extent3D{ (unsigned)width, (unsigned)height, 1u }, pixels);
	data.font_texture = std::move(tex);
	ptc.ctx.debug.set_name(data.font_texture, "ImGui/font");
	vuk::SamplerCreateInfo sci;
	sci.minFilter = sci.magFilter = vuk::Filter::eLinear;
	sci.mipmapMode = vuk::SamplerMipmapMode::eLinear;
	sci.addressModeU = sci.addressModeV = sci.addressModeW = vuk::SamplerAddressMode::eRepeat;
	data.font_sci = sci;
	data.font_si = std::make_unique<vuk::SampledImage>(vuk::SampledImage::Global{ *data.font_texture.view, sci, vuk::ImageLayout::eShaderReadOnlyOptimal });
	io.Fonts->TexID = (ImTextureID)data.font_si.get();
	vuk::PipelineBaseCreateInfo imguiPci;
	imguiPci.add_spirv(std::vector<u32>{
#include "spv/imgui.vert.spv"
	}, "imgui.vert");
	imguiPci.add_spirv(std::vector<u32>{
#include "spv/imgui.frag.spv"
	}, "imgui.frag");
	imguiPci.set_blend(vuk::BlendPreset::eAlphaBlend);
	ptc.ctx.create_named_pipeline("imgui", imguiPci);
	return data;
}

void ImGui_ImplVuk_Render(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, vuk::Name src_target, vuk::Name dst_target, ImguiData& data, ImDrawData* draw_data) {
	auto reset_render_state = [](const ImguiData& data, vuk::CommandBuffer& command_buffer, ImDrawData* draw_data, vuk::Buffer vertex, vuk::Buffer index) {
		command_buffer.bind_sampled_image(0, 0, *data.font_texture.view, data.font_sci);
		if (index.size > 0) {
			command_buffer.bind_index_buffer(index, sizeof(ImDrawIdx) == 2 ? vuk::IndexType::eUint16 : vuk::IndexType::eUint32);
		}
		command_buffer.bind_vertex_buffer(0, vertex, 0, vuk::Packed{ vuk::Format::eR32G32Sfloat, vuk::Format::eR32G32Sfloat, vuk::Format::eR8G8B8A8Unorm });
		command_buffer.bind_graphics_pipeline("imgui");
		command_buffer.set_viewport(0, vuk::Rect2D::framebuffer());
		struct PC {
			float scale[2];
			float translate[2];
		} pc;
		pc.scale[0] = 2.0f / draw_data->DisplaySize.x;
		pc.scale[1] = 2.0f / draw_data->DisplaySize.y;
		pc.translate[0] = -1.0f - draw_data->DisplayPos.x * pc.scale[0];
		pc.translate[1] = -1.0f - draw_data->DisplayPos.y * pc.scale[1];
		command_buffer.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, pc);
	};

	size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
	auto imvert = ptc.allocate_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer | vuk::BufferUsageFlagBits::eTransferDst, vertex_size, 1);
	auto imind = ptc.allocate_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer | vuk::BufferUsageFlagBits::eTransferDst, index_size, 1);

	size_t vtx_dst = 0, idx_dst = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		auto imverto = imvert;
		imverto.offset += vtx_dst * sizeof(ImDrawVert);
		auto imindo = imind;
		imindo.offset += idx_dst * sizeof(ImDrawIdx);

		ptc.upload(imverto, std::span(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size));
		ptc.upload(imindo, std::span(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}

	ptc.wait_all_transfers();
	vuk::Pass pass{
		.name = "Imgui",
		.resources = { vuk::Resource{dst_target, vuk::Resource::Type::eImage, vuk::eColorRW} },
		.execute = [&data, imvert, imind, draw_data, reset_render_state](vuk::CommandBuffer& command_buffer) {
			reset_render_state(data, command_buffer, draw_data, imvert, imind);
			// Will project scissor/clipping rectangles into framebuffer space
			ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
			ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

			// Render command lists
			// (Because we merged all buffers into a single one, we maintain our own offset into them)
			int global_vtx_offset = 0;
			int global_idx_offset = 0;
			for (int n = 0; n < draw_data->CmdListsCount; n++) {
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
					if (pcmd->UserCallback != nullptr) {
						// User callback, registered via ImDrawList::AddCallback()
						// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
						if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
							reset_render_state(data, command_buffer, draw_data, imvert, imind);
						else
							pcmd->UserCallback(cmd_list, pcmd);
					} else {
						// Project scissor/clipping rectangles into framebuffer space
						ImVec4 clip_rect;
						clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
						clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
						clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
						clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

						auto fb_width = command_buffer.get_ongoing_renderpass().extent.width;
						auto fb_height = command_buffer.get_ongoing_renderpass().extent.height;
						if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
							// Negative offsets are illegal for vkCmdSetScissor
							if (clip_rect.x < 0.0f)
								clip_rect.x = 0.0f;
							if (clip_rect.y < 0.0f)
								clip_rect.y = 0.0f;

							// Apply scissor/clipping rectangle
							vuk::Rect2D scissor;
							scissor.offset.x = (int32_t)(clip_rect.x);
							scissor.offset.y = (int32_t)(clip_rect.y);
							scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
							scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
							command_buffer.set_scissor(0, scissor);

							// Bind texture
							if (pcmd->TextureId) {
								auto& si = *reinterpret_cast<vuk::SampledImage*>(pcmd->TextureId);
								if (si.is_global) {
									command_buffer.bind_sampled_image(0, 0, si.global.iv, si.global.sci);
								} else {
									if (si.rg_attachment.ivci) {
										command_buffer.bind_sampled_image(0, 0, si.rg_attachment.attachment_name, *si.rg_attachment.ivci, si.rg_attachment.sci);
									} else {
										command_buffer.bind_sampled_image(0, 0, si.rg_attachment.attachment_name, si.rg_attachment.sci);
									}
								}
							}
							// Draw
							command_buffer.draw_indexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
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
	for (auto& si : ptc.get_sampled_images()) {
		if (!si.is_global) {
			pass.resources.push_back(vuk::Resource(si.rg_attachment.attachment_name, vuk::Resource::Type::eImage, vuk::Access::eFragmentSampled));
		}
	}

	rg.add_pass(std::move(pass));
	rg.add_alias(dst_target, src_target);
}

}
