#include "gfx/sky.hpp"

#include "config.hpp"

#include <vector>
#include "vuk/CommandBuffer.hpp"
#include "imgui.h"
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
	})),
	skyCubemapView(ctx.allocate_texture(vuk::ImageCreateInfo{
		.format = SkyViewFormat,
		.extent = {SkyViewWidth, SkyViewHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	})),
	aerialPerspective(ctx.allocate_texture(vuk::ImageCreateInfo{
		.format = AerialPerspectiveFormat,
		.extent = {AerialPerspectiveWidth, AerialPerspectiveHeight, AerialPerspectiveDepth},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	})) {
	auto skyGenTransmittancePci = vuk::ComputePipelineCreateInfo();
	skyGenTransmittancePci.add_spirv(std::vector<u32>{
#include "spv/skyGenTransmittance.comp.spv"
	}, "skyGenTransmittance.comp");
	ctx.create_named_pipeline("sky_gen_transmittance", skyGenTransmittancePci);

	auto skyGenMultiScatteringPci = vuk::ComputePipelineCreateInfo();
	skyGenMultiScatteringPci.add_spirv(std::vector<u32>{
#include "spv/skyGenMultiScattering.comp.spv"
	}, "skyGenMultiScattering.comp");
	ctx.create_named_pipeline("sky_gen_multi_scattering", skyGenMultiScatteringPci);

	auto skyGenSkyViewPci = vuk::ComputePipelineCreateInfo();
	skyGenSkyViewPci.add_spirv(std::vector<u32>{
#include "spv/skyGenSkyView.comp.spv"
	}, "skyGenSkyView.comp");
	ctx.create_named_pipeline("sky_gen_sky_view", skyGenSkyViewPci);

	auto skyDrawPci = vuk::PipelineBaseCreateInfo();
	skyDrawPci.add_spirv(std::vector<u32>{
#include "spv/skyDraw.vert.spv"
	}, "skyDraw.vert");
	skyDrawPci.add_spirv(std::vector<u32>{
#include "spv/skyDraw.frag.spv"
	}, "skyDraw.frag");
	skyDrawPci.depth_stencil_state.depthWriteEnable = false;
	skyDrawPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eEqual;
	ctx.create_named_pipeline("sky_draw", skyDrawPci);

	auto skyDrawCubemapPci = vuk::ComputePipelineCreateInfo();
	skyDrawCubemapPci.add_spirv(std::vector<u32>{
#include "spv/skyDrawCubemap.comp.spv"
	}, "skyDrawCubemap.comp");
	ctx.create_named_pipeline("sky_draw_cubemap", skyDrawCubemapPci);

	auto skyAerialPerspectivePci = vuk::ComputePipelineCreateInfo();
	skyAerialPerspectivePci.add_spirv(std::vector<u32>{
#include "spv/skyGenAerialPerspective.comp.spv"
	}, "skyGenAerialPerspectivePci.comp");
	ctx.create_named_pipeline("sky_gen_aerial_perspective", skyAerialPerspectivePci);
}
#include "GLFW/glfw3.h"
auto Sky::generateAtmosphereModel(AtmosphereParams const& atmosphere, vuk::Buffer world, vuk::PerThreadContext& ptc) -> vuk::RenderGraph {
	auto atmosphereBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(AtmosphereParams), alignof(AtmosphereParams));
	std::memcpy(atmosphereBuf.mapped_ptr, &atmosphere, sizeof(AtmosphereParams));

	auto rg = vuk::RenderGraph();
	rg.add_pass({
		.name = "Sky transmittance LUT",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeWrite),
		},
		.execute = [world, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, world)
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
		.name = "Sky multiple scattering LUT",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeSampled),
			"sky_multi_scattering"_image(vuk::eComputeWrite),
		},
		.execute = [world, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, world)
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
		.name = "Sky view LUT",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeSampled),
			"sky_multi_scattering"_image(vuk::eComputeSampled),
			"sky_sky_view"_image(vuk::eComputeWrite),
		},
		.execute = [world, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, world)
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
	rg.add_pass({
		.name = "Sky cubemap view LUT",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeSampled),
			"sky_multi_scattering"_image(vuk::eComputeSampled),
			"sky_cubemap_sky_view"_image(vuk::eComputeWrite),
		},
		.execute = [world, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, world)
			   .bind_uniform_buffer(0, 1, atmosphereBuf)
			   .bind_sampled_image(0, 2, "sky_transmittance", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_sampled_image(0, 4, "sky_multi_scattering", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_storage_image(1, 0, "sky_cubemap_sky_view")
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.dispatch_invocations(SkyViewWidth, SkyViewHeight, 1);
		},
	});
	rg.add_pass({
		.name = "Sky aerial perspective LUT",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeSampled),
			"sky_multi_scattering"_image(vuk::eComputeSampled),
			"sky_aerial_perspective"_image(vuk::eComputeWrite),
		},
		.execute = [world, atmosphereBuf](vuk::CommandBuffer& cmd) {
			cmd.bind_uniform_buffer(0, 0, world)
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
			   .bind_storage_image(1, 0, "sky_aerial_perspective")
			   .bind_compute_pipeline("sky_gen_aerial_perspective");
			cmd.dispatch_invocations(AerialPerspectiveWidth, AerialPerspectiveHeight, AerialPerspectiveDepth);
		},
	});
	rg.attach_image("sky_transmittance", vuk::ImageAttachment::from_texture(transmittance), {}, {});
	rg.attach_image("sky_multi_scattering", vuk::ImageAttachment::from_texture(multiScattering), {}, {});
	rg.attach_image("sky_sky_view", vuk::ImageAttachment::from_texture(skyView), {}, {});
	rg.attach_image("sky_cubemap_sky_view", vuk::ImageAttachment::from_texture(skyCubemapView), {}, {});
	rg.attach_image("sky_aerial_perspective", vuk::ImageAttachment::from_texture(aerialPerspective), {}, {});
	return rg;
}

auto Sky::draw(AtmosphereParams const& atmosphere, vuk::Name targetColor, vuk::Name targetDepth, vuk::Buffer world, vuk::PerThreadContext& ptc) -> vuk::RenderGraph {
	auto atmosphereBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(AtmosphereParams), alignof(AtmosphereParams));
	std::memcpy(atmosphereBuf.mapped_ptr, &atmosphere, sizeof(AtmosphereParams));

	auto rg = vuk::RenderGraph();
	rg.add_pass({
		.name = "Sky drawing",
		.resources = {
			"sky_transmittance"_image(vuk::eFragmentSampled),
			"sky_sky_view"_image(vuk::eFragmentSampled),
			vuk::Resource(targetColor, vuk::Resource::Type::eImage, vuk::eColorWrite),
			vuk::Resource(targetDepth, vuk::Resource::Type::eImage, vuk::eDepthStencilRW),
		},
		.execute = [world, atmosphereBuf](vuk::CommandBuffer& cmd) {
			auto skyViewSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
			};
			cmd.bind_uniform_buffer(0, 0, world)
			   .bind_uniform_buffer(0, 1, atmosphereBuf)
			   .bind_sampled_image(0, 2, "sky_transmittance", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_sampled_image(1, 0, "sky_sky_view", skyViewSampler)
			   .bind_graphics_pipeline("sky_draw");
			cmd.draw(3, 1, 0, 0);
		},
	});
	rg.attach_image("sky_transmittance", vuk::ImageAttachment::from_texture(transmittance), {}, {});
	rg.attach_image("sky_sky_view", vuk::ImageAttachment::from_texture(skyView), {}, {});
	return rg;
}

auto Sky::drawCubemap(AtmosphereParams const& atmosphere, vuk::Name target, u32 targetSize, vuk::Buffer world, vuk::PerThreadContext& ptc) -> vuk::RenderGraph {
	auto atmosphereBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(AtmosphereParams), alignof(AtmosphereParams));
	std::memcpy(atmosphereBuf.mapped_ptr, &atmosphere, sizeof(AtmosphereParams));

	auto rg = vuk::RenderGraph();
	rg.add_pass({
		.name = "IBL cubemap",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeSampled),
			"sky_cubemap_sky_view"_image(vuk::eComputeSampled),
			vuk::Resource(target, vuk::Resource::Type::eImage, vuk::eComputeWrite),
		},
		.execute = [world, atmosphereBuf, target, targetSize](vuk::CommandBuffer& cmd) {
			auto skyViewSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
			};
			cmd.bind_uniform_buffer(0, 0, world)
			   .bind_uniform_buffer(0, 1, atmosphereBuf)
			   .bind_sampled_image(0, 2, "sky_transmittance", vuk::SamplerCreateInfo{
				   .magFilter = vuk::Filter::eLinear,
				   .minFilter = vuk::Filter::eLinear,
				   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
			   .bind_sampled_image(1, 0, "sky_cubemap_sky_view", skyViewSampler)
			   .bind_storage_image(1, 1, target)
			   .bind_compute_pipeline("sky_draw_cubemap");
			auto* sides = cmd.map_scratch_uniform_binding<std::array<mat4, 6>>(1, 2);
			*sides = std::to_array<mat4>({
			mat3{
				0.0f, 0.0f, -1.0f,
				0.0f, -1.0f, 0.0f,
				1.0f, 0.0f, 0.0f,
			}, mat3{
				0.0f, 0.0f, 1.0f,
				0.0f, -1.0f, 0.0f,
				-1.0f, 0.0f, 0.0f,
			}, mat3{
				1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f,
				0.0f, 1.0f, 0.0f,
			}, mat3{
				1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f,
				0.0f, -1.0f, 0.0f,
			}, mat3{
				1.0f, 0.0f, 0.0f,
				0.0f, -1.0f, 0.0f,
				0.0f, 0.0f, 1.0f,
			}, mat3{
				-1.0f, 0.0f, 0.0f,
				0.0f, -1.0f, 0.0f,
				0.0f, 0.0f, -1.0f,
			}});
			cmd.dispatch_invocations(targetSize, targetSize, 6);
		},
	});
	rg.attach_image("sky_transmittance", vuk::ImageAttachment::from_texture(transmittance), {}, {});
	rg.attach_image("sky_cubemap_sky_view", vuk::ImageAttachment::from_texture(skyCubemapView), {}, {});
	return rg;
}

}
