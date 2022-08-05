#include "gfx/effects/tonemap.hpp"

#include "imgui.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Pipeline.hpp"
#include "gfx/samplers.hpp"
#include "gfx/shader.hpp"
#include "sys/vulkan.hpp"

namespace minote {

auto Tonemap::apply(vuk::Future _source) -> vuk::Future {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("tonemap");
	rg->attach_in("source", _source);
	rg->attach_image("target", vuk::ImageAttachment{
		.format = vuk::Format::eR8G8B8A8Unorm,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->inference_rule("target", vuk::same_extent_as("source"));
	
	rg->add_pass(vuk::Pass{
		.name = "tonemap/apply",
		.resources = {
			"source"_image >> vuk::eComputeSampled,
			"target"_image >> vuk::eComputeWrite >> "target/final",
		},
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("tonemap/apply")
			   .bind_image(0, 0, "source").bind_sampler(0, 0, NearestClamp)
			   .bind_image(0, 1, "target");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, genConstants());
			
			auto target = *cmd.get_resource_image_attachment("target");
			auto targetSize = target.extent.extent;
			cmd.specialize_constants(0, targetSize.width);
			cmd.specialize_constants(1, targetSize.height);
			
			cmd.dispatch_invocations(targetSize.width, targetSize.height);
			
		},
	});
	
	return vuk::Future(rg, "target/final");
	
}

GET_SHADER(tonemap_apply_cs);
void Tonemap::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto applyPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(applyPci, tonemap_apply_cs, "tonemap/apply.cs.hlsl");
	ctx.create_named_pipeline("tonemap/apply", applyPci);
	
	m_compiled = true;
	
}

auto Tonemap::genConstants() -> array<vec4, 4> {
	
	auto result = array<vec4, 4>();
	
	result[0].x() = contrast;
	result[0].y() = shoulder;
	f32 cs = contrast * shoulder;
	
	f32 z0 = -pow(midIn,contrast);
	f32 z1 = pow(hdrMax,cs)*pow(midIn,contrast);
	f32 z2 = pow(hdrMax,contrast)*pow(midIn,cs)*midOut;
	f32 z3 = pow(hdrMax,cs)*midOut;
	f32 z4 = pow(midIn,cs)*midOut;
	result[0].z() = -((z0+(midOut*(z1-z2))/(z3-z4))/z4);
	
	f32 w0 = pow(hdrMax,cs)*pow(midIn,contrast);
	f32 w1 = pow(hdrMax,contrast)*pow(midIn,cs)*midOut;
	f32 w2 = pow(hdrMax,cs)*midOut;
	f32 w3 = pow(midIn,cs)*midOut;
	result[0].w() = (w0-w1)/(w2-w3);
	
	result[1] = vec4((saturation+vec3(contrast))/crosstalkSaturation, 0.0f);
	result[2] = vec4(crosstalk, 0.0f);
	result[3] = vec4(crosstalkSaturation, 0.0f);
	
	return result;
	
}

void Tonemap::drawImguiDebug(string_view _name) {
	
	ImGui::Begin(string(_name).c_str());
	ImGui::SliderFloat("Contrast", &contrast, 0.5f, 5.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderFloat("HDR max", &hdrMax, 1.0f, 128.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderFloat("Mid in", &midIn, 0.01f, 1.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderFloat("Mid out", &midOut, 0.01f, 0.99f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderFloat3("Saturation", &saturation[0], 0.0f, 10.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderFloat3("Crosstalk", &crosstalk[0], 1.0f, 256.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderFloat3("Crosstalk saturation", &crosstalkSaturation[0], 1.0f, 64.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	ImGui::End();
	
}

}
