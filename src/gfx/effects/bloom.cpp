#include "gfx/effects/bloom.hpp"

#include "imgui.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Pipeline.hpp"
#include "gfx/samplers.hpp"
#include "gfx/shader.hpp"
#include "sys/vulkan.hpp"
#include "util/string.hpp"
#include "util/vector.hpp"
#include "util/verify.hpp"
#include "util/util.hpp"

namespace minote {

GET_SHADER(bloom_down_cs);
GET_SHADER(bloom_up_cs);
void Bloom::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto downPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(downPci, bloom_down_cs, "bloom/down.cs.hlsl");
	ctx.create_named_pipeline("bloom/down", downPci);
	
	auto upPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(upPci, bloom_up_cs, "bloom/up.cs.hlsl");
	ctx.create_named_pipeline("bloom/up", upPci);
	
	m_compiled = true;
	
}

auto Bloom::apply(Texture2D<float4> _target) -> Texture2D<float4> {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("bloom");
	rg->attach_in("target", _target);
	rg->attach_image("temp", vuk::ImageAttachment{
		.format = Format,
		.sample_count = vuk::Samples::e1,
		.level_count = passes,
		.layer_count = 1,
	});
	rg->inference_rule("temp", [](vuk::InferenceContext const& ctx, vuk::ImageAttachment& ia) {
		auto extent = ctx.get_image_attachment("target").extent.extent;
		ia.extent = vuk::Dimension3D::absolute(extent.width / 2, extent.height / 2);
	});
	
	// Create a subresource for each temp mip
	auto tempMips = ivector<vuk::Name>(passes);
	for (auto i: iota(0u, passes)) {
		tempMips[i] = vuk::Name("temp").append(to_string(i));
		rg->diverge_image("temp", vuk::Subrange::Image{
			.base_level = i,
			.level_count = 1,
		}, tempMips[i]);
	}
	
	// Downsample phase: repeatedly draw the source image into increasingly smaller mips
	for (auto i: iota(0u, passes)) {
		auto source = i == 0?
			vuk::Name("target") :
			tempMips[i-1];
		auto target = tempMips[i];
		auto targetNew = tempMips[i].append("+");
		rg->add_pass(vuk::Pass{
			.name = vuk::Name("bloom/down").append(to_string(i)),
			.resources = {
				vuk::Resource(source, vuk::Resource::Type::eImage, vuk::eComputeSampled),
				vuk::Resource(target, vuk::Resource::Type::eImage, vuk::eComputeWrite, targetNew),
			},
			.execute = [i, source, target](vuk::CommandBuffer& cmd) {
				cmd.bind_compute_pipeline("bloom/down")
				   .bind_image(0, 0, source).bind_sampler(0, 0, LinearClamp)
				   .bind_image(0, 1, target);
				
				auto sourceIA = *cmd.get_resource_image_attachment(source);
				auto sourceSize = sourceIA.extent.extent;
				sourceSize.width /= 1u << sourceIA.base_level;
				sourceSize.height /= 1u << sourceIA.base_level;
				cmd.specialize_constants(0, sourceSize.width);
				cmd.specialize_constants(1, sourceSize.height);
				auto targetIA = *cmd.get_resource_image_attachment(target);
				auto targetSize = targetIA.extent.extent;
				targetSize.width /= 1u << targetIA.base_level;
				targetSize.height /= 1u << targetIA.base_level;
				cmd.specialize_constants(2, targetSize.width);
				cmd.specialize_constants(3, targetSize.height);
				cmd.specialize_constants(4, i == 0? 1u : 0u); // Use Karis average on first pass
				
				cmd.dispatch_invocations(targetSize.width, targetSize.height);
				
			},
		});
		tempMips[i] = targetNew;
	}
	
	// Upsample phase: additively blend all mips by traversing back down the mip chain
	for (auto i: iota(0u, passes) | reverse) {
		auto source = tempMips[i];
		auto target = i == 0?
			vuk::Name("target") :
			tempMips[i-1];
		auto targetNew = i == 0?
			vuk::Name("target/final") :
			tempMips[i-1].append("+");
		rg->add_pass(vuk::Pass{
			.name = vuk::Name("bloom/up").append(to_string(i)),
			.resources = {
				vuk::Resource(source, vuk::Resource::Type::eImage, vuk::eComputeSampled),
				vuk::Resource(target, vuk::Resource::Type::eImage, vuk::eComputeRW, targetNew),
			},
			.execute = [this, i, source, target](vuk::CommandBuffer& cmd) {
				cmd.bind_compute_pipeline("bloom/up")
				   .bind_image(0, 0, source).bind_sampler(0, 0, LinearClamp)
				   .bind_image(0, 1, target);
				
				auto sourceIA = *cmd.get_resource_image_attachment(source);
				auto sourceSize = sourceIA.extent.extent;
				sourceSize.width /= 1u << sourceIA.base_level;
				sourceSize.height /= 1u << sourceIA.base_level;
				cmd.specialize_constants(0, sourceSize.width);
				cmd.specialize_constants(1, sourceSize.height);
				auto targetIA = *cmd.get_resource_image_attachment(target);
				auto targetSize = targetIA.extent.extent;
				targetSize.width /= 1u << targetIA.base_level;
				targetSize.height /= 1u << targetIA.base_level;
				cmd.specialize_constants(2, targetSize.width);
				cmd.specialize_constants(3, targetSize.height);
				
				auto power = i == 0? strength : 1.0f;
				cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, power);
				
				cmd.dispatch_invocations(targetSize.width, targetSize.height);
				
			},
		});
		if (i != 0) tempMips[i-1] = targetNew;
	}
	
	// rg->converge_image("temp", "temp/final"); // pending vuk bugfix
	return vuk::Future(rg, "target/final");
	
}

void Bloom::drawImguiDebug(string_view _name) {
	
	ImGui::Begin(string(_name).c_str());
	ImGui::SliderInt("Passes", reinterpret_cast<int*>(&passes), 1u, 8u);
	ImGui::SliderFloat("Strength", &strength, 0.0f, 1.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
	ImGui::End();
	
}

}
