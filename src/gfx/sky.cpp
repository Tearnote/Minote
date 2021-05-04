#include "gfx/sky.hpp"

#include <vector>
#include "vuk/CommandBuffer.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

Sky::Sky(vuk::Context& ctx):
	transmittance(ctx.allocate_texture(vuk::ImageCreateInfo{
		.format = TransmittanceFormat,
		.extent = {TransmittanceWidth, TransmittanceHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	})) {
	auto skyGenTransmittancePci = vuk::ComputePipelineCreateInfo();
	skyGenTransmittancePci.add_spirv(std::vector<u32>{
#include "spv/skyGenTransmittance.comp.spv"
	}, "skyGenTransmittancePci.comp");
	ctx.create_named_pipeline("sky_gen_transmittance", skyGenTransmittancePci);
}

auto Sky::generateAtmosphereModel(AtmosphereParams const& _atmosphere, glm::uvec2 resolution, glm::mat4 viewProjection) -> vuk::RenderGraph {
	auto rg = vuk::RenderGraph();
	rg.add_pass({
		.name = "Sky transmittance LUT generation",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeRW),
		},
		.execute = [&_atmosphere, &resolution, &viewProjection](vuk::CommandBuffer& cmd) {
			auto* globals = cmd.map_scratch_uniform_binding<Globals>(0, 0);
			auto* atmosphere = cmd.map_scratch_uniform_binding<AtmosphereParams>(0, 1);
			*globals = Globals{
				.gSkyInvViewProjMat = glm::inverse(viewProjection),
				.gResolution = resolution,
				.RayMarchMinMaxSPP = {4.0f, 14.0f},
				.gSunIlluminance = {1.0f, 1.0f, 1.0f},
				.sun_direction = {0.00f, 0.90045f, 0.43497f},
			};
			*atmosphere = _atmosphere;
			cmd.bind_sampled_image(0, 2, "sky_transmittance", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_storage_image(1, 0, "sky_transmittance")
			   .bind_compute_pipeline("sky_gen_transmittance");
			cmd.dispatch_invocations(256, 64);
		},
	});
	rg.attach_image("sky_transmittance", vuk::ImageAttachment::from_texture(transmittance), {}, {});
	return rg;
}

}
