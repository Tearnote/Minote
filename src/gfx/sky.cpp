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
	})),
	multiScattering(ctx.allocate_texture(vuk::ImageCreateInfo{
		.format = MultiScatteringFormat,
		.extent = {MultiScatteringWidth, MultiScatteringHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	})),
	skyView(ctx.allocate_texture(vuk::ImageCreateInfo{
		.format = SkyViewFormat,
		.extent = {SkyViewWidth, SkyViewHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	})) {
	auto skyGenTransmittancePci = vuk::ComputePipelineCreateInfo();
	skyGenTransmittancePci.add_spirv(std::vector<u32>{
#include "spv/skyGenTransmittance.comp.spv"
	}, "skyGenTransmittancePci.comp");
	ctx.create_named_pipeline("sky_gen_transmittance", skyGenTransmittancePci);

	auto skyGenMultiScatteringPci = vuk::ComputePipelineCreateInfo();
	skyGenMultiScatteringPci.add_spirv(std::vector<u32>{
#include "spv/skyGenMultiScattering.comp.spv"
	}, "skyGenMultiScatteringPci.comp");
	ctx.create_named_pipeline("sky_gen_multi_scattering", skyGenMultiScatteringPci);

	auto skyGenSkyViewPci = vuk::ComputePipelineCreateInfo();
	skyGenSkyViewPci.add_spirv(std::vector<u32>{
#include "spv/skyGenSkyView.comp.spv"
	}, "skyGenSkyViewPci.comp");
	ctx.create_named_pipeline("sky_gen_sky_view", skyGenSkyViewPci);
}

auto Sky::generateAtmosphereModel(AtmosphereParams const& atmosphere, vuk::PerThreadContext& ptc,
	glm::uvec2 resolution, glm::vec3 cameraPos, glm::mat4 viewProjection) -> vuk::RenderGraph {
	auto globals = Globals{
		.gSkyInvViewProjMat = glm::inverse(viewProjection),
		.gResolution = resolution,
		.RayMarchMinMaxSPP = {4.0f, 14.0f},
		.gSunIlluminance = {1.0f, 1.0f, 1.0f},
		.MultipleScatteringFactor = 1.0f,
		.sun_direction = {0.00f, 0.90045f, 0.43497f},
		.camera = cameraPos,
	};
	auto globalsBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(Globals), alignof(Globals));
	std::memcpy(globalsBuf.mapped_ptr, &globals, sizeof(Globals));

	auto atmosphereBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(AtmosphereParams), alignof(AtmosphereParams));
	std::memcpy(atmosphereBuf.mapped_ptr, &atmosphere, sizeof(AtmosphereParams));

	auto rg = vuk::RenderGraph();
	rg.add_pass({
		.name = "Sky transmittance LUT generation",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeRW),
		},
		.execute = [globalsBuf, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, globalsBuf)
			   .bind_uniform_buffer(0, 1, atmosphereBuf)
			   .bind_sampled_image(0, 2, "sky_transmittance", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_storage_image(1, 0, "sky_transmittance")
			   .bind_compute_pipeline("sky_gen_transmittance");
			cmd.dispatch_invocations(TransmittanceWidth, TransmittanceHeight);
		},
	});
	rg.add_pass({
		.name = "Sky multiple scattering LUT generation",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeRead),
			"sky_multi_scattering"_image(vuk::eComputeWrite),
		},
		.execute = [globalsBuf, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, globalsBuf)
			   .bind_uniform_buffer(0, 1, atmosphereBuf)
			   .bind_sampled_image(0, 2, "sky_transmittance", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_storage_image(1, 0, "sky_multi_scattering")
			   .bind_compute_pipeline("sky_gen_multi_scattering");
			cmd.dispatch_invocations(MultiScatteringWidth, MultiScatteringHeight, 1);
		},
	});
	rg.add_pass({
		.name = "Sky view LUT generation",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeRead),
			"sky_multi_scattering"_image(vuk::eComputeRead),
			"sky_sky_view"_image(vuk::eComputeWrite),
		},
		.execute = [globalsBuf, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, globalsBuf)
			   .bind_uniform_buffer(0, 1, atmosphereBuf)
			   .bind_sampled_image(0, 2, "sky_transmittance", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_sampled_image(0, 3, "sky_multi_scattering", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_storage_image(1, 0, "sky_sky_view")
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.dispatch_invocations(SkyViewWidth, SkyViewHeight, 1);
		},
	});
	rg.attach_image("sky_transmittance", vuk::ImageAttachment::from_texture(transmittance), {}, {});
	rg.attach_image("sky_multi_scattering", vuk::ImageAttachment::from_texture(multiScattering), {}, {});
	rg.attach_image("sky_sky_view", vuk::ImageAttachment::from_texture(skyView), {}, {});
	return rg;
}

}
