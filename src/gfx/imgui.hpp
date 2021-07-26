#pragma once

#include "imgui.h"
#include "vuk/SampledImage.hpp"
#include "vuk/RenderGraph.hpp"

namespace minote::gfx {

struct ImguiData {
	vuk::Texture font_texture;
	vuk::SamplerCreateInfo font_sci;
	std::unique_ptr<vuk::SampledImage> font_si;
};

auto ImGui_ImplVuk_Init(vuk::PerThreadContext& ptc) -> ImguiData;

void ImGui_ImplVuk_Render(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg,
	vuk::Name src_target, vuk::Name dst_target, ImguiData& data, ImDrawData* draw_data);

}
