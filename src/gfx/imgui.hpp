#pragma once

#include "imgui.h"
#include "vuk/SampledImage.hpp"
#include "vuk/RenderGraph.hpp"

namespace minote::gfx {

#ifndef IMGUI_DISABLE

struct ImguiData {
	vuk::Texture font_texture;
	vuk::SamplerCreateInfo font_sci;
	std::unique_ptr<vuk::SampledImage> font_si;
};

ImguiData ImGui_ImplVuk_Init(vuk::PerThreadContext& ptc);

void ImGui_ImplVuk_Render(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, vuk::Name src_target, vuk::Name dst_target, ImguiData& data, ImDrawData* draw_data);

#endif //IMGUI_DISABLE

}
