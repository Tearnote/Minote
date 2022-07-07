#pragma once

#include "imgui.h"
#include "vuk/SampledImage.hpp"
#include "vuk/RenderGraph.hpp"
#include "util/vector.hpp"

namespace minote {

// GPU resources used by Imgui
struct ImguiData {
	
	vuk::Texture fontTex;
	std::unique_ptr<vuk::SampledImage> fontSI;
	
};

// Initialize Imgui. Run once after Vulkan is initialized, and keep the result
// struct around as long as Imgui is being used.
auto ImGui_ImplVuk_Init(vuk::Allocator&) -> ImguiData;

// Draw all GUI elements that were queued up for this frame.
void ImGui_ImplVuk_Render(vuk::Allocator&, vuk::RenderGraph&,
	vuk::Name targetFrom, vuk::Name targetTo,
	ImguiData&, ImDrawData*,
	ivector<vuk::SampledImage> const& sampledImages);

}
