#pragma once

#include "imgui.h"
#include "vuk/SampledImage.hpp"
#include "vuk/RenderGraph.hpp"
#include "gfx/resources/pool.hpp"

namespace minote::gfx {

// GPU resources used by Imgui
struct ImguiData {
	
	vuk::Texture fontTex;
	std::unique_ptr<vuk::SampledImage> fontSI;
	
};

// Initialize Imgui. Run once after Vulkan is initialized, and keep the result
// struct around as long as Imgui is being used.
auto ImGui_ImplVuk_Init(vuk::PerThreadContext&) -> ImguiData;

// Draw all GUI elements that were queued up for this frame.
void ImGui_ImplVuk_Render(Pool&, vuk::PerThreadContext&, vuk::RenderGraph&,
	vuk::Name source, vuk::Name target, ImguiData&, ImDrawData*);

}
