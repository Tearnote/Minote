#include "gfx/sky.hpp"

#include "config.hpp"

#include <vector>
#include "vuk/CommandBuffer.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

auto Atmosphere::Params::earth() -> Params {
	
	auto const EarthRayleighScaleHeight = 8.0f;
	auto const EarthMieScaleHeight = 1.2f;
	auto const MieScattering = vec3(0.003996f, 0.003996f, 0.003996f);
	auto const MieExtinction = vec3(0.004440f, 0.004440f, 0.004440f);
	
	return Params{
		.BottomRadius = 6360.0f,
		.TopRadius = 6460.0f,
		.RayleighDensityExpScale = -1.0f / EarthRayleighScaleHeight,
		.RayleighScattering = {0.005802f, 0.013558f, 0.033100f},
		.MieDensityExpScale = -1.0f / EarthMieScaleHeight,
		.MieScattering = MieScattering,
		.MieExtinction = MieExtinction,
		.MieAbsorption = max(MieExtinction - MieScattering, vec3(0.0f)),
		.MiePhaseG = 0.8f,
		.AbsorptionDensity0LayerWidth = 25.0f,
		.AbsorptionDensity0ConstantTerm = -2.0f / 3.0f,
		.AbsorptionDensity0LinearTerm = 1.0f / 15.0f,
		.AbsorptionDensity1ConstantTerm = 8.0f / 3.0f,
		.AbsorptionDensity1LinearTerm = -1.0f / 15.0f,
		.AbsorptionExtinction = {0.000650f, 0.001881f, 0.000085f},
		.GroundAlbedo = {0.0f, 0.0f, 0.0f},
	};
	
}

Atmosphere::Atmosphere(vuk::PerThreadContext& _ptc, Params const& _params) {
	
	transmittance = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = TransmittanceFormat,
		.extent = {TransmittanceWidth, TransmittanceHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	});
	
	multiScattering = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = MultiScatteringFormat,
		.extent = {MultiScatteringWidth, MultiScatteringHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	});
	
	auto paramsNotConst = Params(_params);
	params = _ptc.create_buffer<Params>(
		vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eUniformBuffer | vuk::BufferUsageFlagBits::eTransferDst,
		std::span(&paramsNotConst, 1_zu)).first;
	
	if (!pipelinesCreated) {
		
		auto skyGenTransmittancePci = vuk::ComputePipelineCreateInfo();
		skyGenTransmittancePci.add_spirv(std::vector<u32>{
#include "spv/skyGenTransmittance.comp.spv"
		}, "skyGenTransmittance.comp");
		_ptc.ctx.create_named_pipeline("sky_gen_transmittance", skyGenTransmittancePci);

		auto skyGenMultiScatteringPci = vuk::ComputePipelineCreateInfo();
		skyGenMultiScatteringPci.add_spirv(std::vector<u32>{
#include "spv/skyGenMultiScattering.comp.spv"
		}, "skyGenMultiScattering.comp");
		_ptc.ctx.create_named_pipeline("sky_gen_multi_scattering", skyGenMultiScatteringPci);
		
		pipelinesCreated = true;
		
	}
	
}

auto Atmosphere::precalculate() -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Sky transmittance LUT",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeWrite),
		},
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 1, *params)
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
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 1, *params)
			   .bind_sampled_image(0, 2, "sky_transmittance", LinearClamp)
			   .bind_storage_image(1, 0, "sky_multi_scattering")
			   .bind_compute_pipeline("sky_gen_multi_scattering");
			cmd.dispatch_invocations(MultiScatteringWidth, MultiScatteringHeight, 1);
			
		},
	});
	
	rg.attach_image("sky_transmittance",
		vuk::ImageAttachment::from_texture(transmittance),
		vuk::Access::eNone,
		vuk::Access::eComputeSampled);
	rg.attach_image("sky_multi_scattering",
		vuk::ImageAttachment::from_texture(multiScattering),
		vuk::Access::eNone,
		vuk::Access::eComputeSampled);
		
	return rg;
	
}

Sky::Sky(vuk::PerThreadContext& _ptc, Atmosphere const& _atmosphere):
	atmosphere(_atmosphere) {
	
	skyView = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = SkyViewFormat,
		.extent = {SkyViewWidth, SkyViewHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	});
	
	skyCubemapView = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = SkyViewFormat,
		.extent = {SkyViewWidth, SkyViewHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	});
	
	aerialPerspective = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = AerialPerspectiveFormat,
		.extent = {AerialPerspectiveWidth, AerialPerspectiveHeight, AerialPerspectiveDepth},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	});
	
	if (!pipelinesCreated) {
		
		auto skyGenSkyViewPci = vuk::ComputePipelineCreateInfo();
		skyGenSkyViewPci.add_spirv(std::vector<u32>{
#include "spv/skyGenSkyView.comp.spv"
		}, "skyGenSkyView.comp");
		_ptc.ctx.create_named_pipeline("sky_gen_sky_view", skyGenSkyViewPci);

		auto skyDrawPci = vuk::PipelineBaseCreateInfo();
		skyDrawPci.add_spirv(std::vector<u32>{
#include "spv/skyDraw.vert.spv"
		}, "skyDraw.vert");
		skyDrawPci.add_spirv(std::vector<u32>{
#include "spv/skyDraw.frag.spv"
		}, "skyDraw.frag");
		skyDrawPci.depth_stencil_state.depthWriteEnable = false;
		skyDrawPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eEqual;
		_ptc.ctx.create_named_pipeline("sky_draw", skyDrawPci);

		auto skyDrawCubemapPci = vuk::ComputePipelineCreateInfo();
		skyDrawCubemapPci.add_spirv(std::vector<u32>{
#include "spv/skyDrawCubemap.comp.spv"
		}, "skyDrawCubemap.comp");
		_ptc.ctx.create_named_pipeline("sky_draw_cubemap", skyDrawCubemapPci);

		auto skyAerialPerspectivePci = vuk::ComputePipelineCreateInfo();
		skyAerialPerspectivePci.add_spirv(std::vector<u32>{
#include "spv/skyGenAerialPerspective.comp.spv"
		}, "skyGenAerialPerspectivePci.comp");
		_ptc.ctx.create_named_pipeline("sky_gen_aerial_perspective", skyAerialPerspectivePci);
		
		pipelinesCreated = true;
		
	}
	
}

auto Sky::calculate(vuk::Buffer _world, Camera const& _camera) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Sky view LUT",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeSampled),
			"sky_multi_scattering"_image(vuk::eComputeSampled),
			"sky_sky_view"_image(vuk::eComputeWrite),
		},
		.execute = [this, _world, _camera](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, *atmosphere.params)
			   .bind_sampled_image(0, 2, "sky_transmittance", LinearClamp)
			   .bind_sampled_image(0, 3, "sky_multi_scattering", LinearClamp)
			   .bind_storage_image(1, 0, "sky_sky_view")
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu,
				_camera.position);
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
		.execute = [this, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, *atmosphere.params)
			   .bind_sampled_image(0, 2, "sky_transmittance", LinearClamp)
			   .bind_sampled_image(0, 3, "sky_multi_scattering", LinearClamp)
			   .bind_storage_image(1, 0, "sky_cubemap_sky_view")
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu,
				vec3(0_m, 0_m, 1_m));
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
		.execute = [this, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, *atmosphere.params)
			   .bind_sampled_image(0, 2, "sky_transmittance", LinearClamp)
			   .bind_sampled_image(0, 3, "sky_multi_scattering", LinearClamp)
			   .bind_storage_image(1, 0, "sky_aerial_perspective")
			   .bind_compute_pipeline("sky_gen_aerial_perspective");
			cmd.dispatch_invocations(AerialPerspectiveWidth, AerialPerspectiveHeight, AerialPerspectiveDepth);
			
		},
	});
	
	rg.attach_image("sky_transmittance",
		vuk::ImageAttachment::from_texture(atmosphere.transmittance),
		vuk::Access::eComputeSampled,
		vuk::Access::eComputeSampled);
	rg.attach_image("sky_multi_scattering",
		vuk::ImageAttachment::from_texture(atmosphere.multiScattering),
		vuk::Access::eComputeSampled,
		vuk::Access::eComputeSampled);
	rg.attach_image("sky_sky_view",
		vuk::ImageAttachment::from_texture(skyView),
		vuk::Access::eNone,
		vuk::Access::eNone);
	rg.attach_image("sky_cubemap_sky_view",
		vuk::ImageAttachment::from_texture(skyCubemapView),
		vuk::Access::eNone,
		vuk::Access::eNone);
	rg.attach_image("sky_aerial_perspective",
		vuk::ImageAttachment::from_texture(aerialPerspective),
		vuk::Access::eNone,
		vuk::Access::eNone);
	
	return rg;
	
}

auto Sky::draw(vuk::Buffer _world, vuk::Name _targetColor,
	vuk::Name _targetDepth) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Background sky",
		.resources = {
			"sky_transmittance"_image(vuk::eFragmentSampled),
			"sky_sky_view"_image(vuk::eFragmentSampled),
			vuk::Resource(_targetColor, vuk::Resource::Type::eImage, vuk::eColorWrite),
			vuk::Resource(_targetDepth, vuk::Resource::Type::eImage, vuk::eDepthStencilRW),
		},
		.execute = [this, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, *atmosphere.params)
			   .bind_sampled_image(0, 2, "sky_transmittance", LinearClamp)
			   .bind_sampled_image(1, 0, "sky_sky_view", LinearClamp)
			   .bind_graphics_pipeline("sky_draw");
			cmd.draw(3, 1, 0, 0);
			
		},
	});
	
	rg.attach_image("sky_transmittance",
		vuk::ImageAttachment::from_texture(atmosphere.transmittance),
		vuk::Access::eComputeSampled,
		vuk::Access::eComputeSampled);
	rg.attach_image("sky_sky_view",
		vuk::ImageAttachment::from_texture(skyView),
		vuk::Access::eNone,
		vuk::Access::eNone);
	
	return rg;
	
}

auto Sky::drawCubemap(vuk::Buffer _world, vuk::Name _target,
	uvec2 _targetSize) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Cubemap sky",
		.resources = {
			"sky_transmittance"_image(vuk::eComputeSampled),
			"sky_cubemap_sky_view"_image(vuk::eComputeSampled),
			vuk::Resource(_target, vuk::Resource::Type::eImage, vuk::eComputeWrite),
		},
		.execute = [this, _world, _target, _targetSize](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, *atmosphere.params)
			   .bind_sampled_image(0, 2, "sky_transmittance", LinearClamp)
			   .bind_sampled_image(1, 0, "sky_cubemap_sky_view", LinearClamp)
			   .bind_storage_image(1, 1, _target)
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
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu,
				vec3(0_m, 0_m, 1_m));
			
			cmd.dispatch_invocations(_targetSize.x, _targetSize.y, 6);
			
		},
	});
	
	rg.attach_image("sky_transmittance",
		vuk::ImageAttachment::from_texture(atmosphere.transmittance),
		vuk::Access::eComputeSampled,
		vuk::Access::eComputeSampled);
	rg.attach_image("sky_cubemap_sky_view",
		vuk::ImageAttachment::from_texture(skyCubemapView),
		vuk::Access::eNone,
		vuk::Access::eNone);
	
	return rg;
}

}
