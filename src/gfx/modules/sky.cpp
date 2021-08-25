#include "gfx/modules/sky.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

auto Atmosphere::Params::earth() -> Params {
	
	auto const EarthRayleighScaleHeight = 8.0f;
	auto const EarthMieScaleHeight = 1.2f;
	auto const MieScattering = vec3{0.003996f, 0.003996f, 0.003996f};
	auto const MieExtinction = vec3{0.004440f, 0.004440f, 0.004440f};
	
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
		.GroundAlbedo = {0.0f, 0.0f, 0.0f} };
	
}

void Atmosphere::compile(vuk::PerThreadContext& _ptc) {
	
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
	
}

void Atmosphere::upload(vuk::PerThreadContext& _ptc, vuk::Name _name, Params const& _params) {
	
	transmittance = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = TransmittanceFormat,
		.extent = {TransmittanceWidth, TransmittanceHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled });
	
	multiScattering = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = MultiScatteringFormat,
		.extent = {MultiScatteringWidth, MultiScatteringHeight, 1},
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled });
	
	params = Buffer(_ptc, nameAppend(_name, " params"), std::span(&_params, 1),
		vuk::BufferUsageFlagBits::eUniformBuffer, vuk::MemoryUsage::eGPUonly);
	
}

auto Atmosphere::precalculate() -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Sky transmittance LUT",
		.resources = {
			vuk::Resource(Transmittance_n, vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 1, params)
			   .bind_storage_image(1, 0, Transmittance_n)
			   .bind_compute_pipeline("sky_gen_transmittance");
			cmd.dispatch_invocations(TransmittanceWidth, TransmittanceHeight);
			
		}});
	
	rg.add_pass({
		.name = "Sky multiple scattering LUT",
		.resources = {
			vuk::Resource(Transmittance_n,   vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(MultiScattering_n, vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 1, params)
			   .bind_sampled_image(0, 2, Transmittance_n, LinearClamp)
			   .bind_storage_image(1, 0, MultiScattering_n)
			   .bind_compute_pipeline("sky_gen_multi_scattering");
			cmd.dispatch_invocations(MultiScatteringWidth, MultiScatteringHeight, 1);
			
		}});
	
	rg.attach_image(Transmittance_n,
		vuk::ImageAttachment::from_texture(transmittance),
		vuk::eNone,
		vuk::Access::eComputeSampled);
	rg.attach_image(MultiScattering_n,
		vuk::ImageAttachment::from_texture(multiScattering),
		vuk::eNone,
		vuk::Access::eComputeSampled);
		
	return rg;
	
}

void Atmosphere::cleanup(vuk::PerThreadContext& _ptc) {
	
	params.recycle(_ptc);
	
}

Sky::Sky(vuk::PerThreadContext& _ptc, Atmosphere const& _atmosphere):
	atmosphere(_atmosphere) {
	
	cameraView = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = ViewFormat,
		.extent = {ViewWidth, ViewHeight, 1},
		.usage =
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled });
	
	cubemapView = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = ViewFormat,
		.extent = {ViewWidth, ViewHeight, 1},
		.usage =
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled });
	
	aerialPerspective = _ptc.allocate_texture(vuk::ImageCreateInfo{
		.format = AerialPerspectiveFormat,
		.extent = {AerialPerspectiveWidth, AerialPerspectiveHeight, AerialPerspectiveDepth},
		.usage =
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled });
	
	sunLuminance = _ptc.allocate_buffer(
		vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eStorageBuffer,
		sizeof(vec3), alignof(vec3));
	
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

auto Sky::calculate(Buffer<World> const& _world, Camera const& _camera) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Sky view LUT",
		.resources = {
			vuk::Resource(atmosphere.Transmittance_n,   vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(atmosphere.MultiScattering_n, vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(CameraView_n,                 vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this, &_world, _camera](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.Transmittance_n, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.MultiScattering_n, LinearClamp)
			   .bind_storage_image(1, 0, CameraView_n)
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu, _camera.position);
			cmd.dispatch_invocations(ViewWidth, ViewHeight, 1);
			
		}});
	
	rg.add_pass({
		.name = "Sky cubemap view LUT",
		.resources = {
			vuk::Resource(atmosphere.Transmittance_n,   vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(atmosphere.MultiScattering_n, vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(CubemapView_n,                vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this, &_world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.Transmittance_n, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.MultiScattering_n, LinearClamp)
			   .bind_storage_image(1, 0, CubemapView_n)
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu, CubemapCamera);
			cmd.dispatch_invocations(ViewWidth, ViewHeight, 1);
			
		}});
	
	rg.add_pass({
		.name = "Sky aerial perspective LUT",
		.resources = {
			vuk::Resource(atmosphere.Transmittance_n,   vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(atmosphere.MultiScattering_n, vuk::Resource::Type::eImage, vuk::eComputeSampled),
			vuk::Resource(AerialPerspective_n,          vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this, &_world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.Transmittance_n, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.MultiScattering_n, LinearClamp)
			   .bind_storage_image(1, 0, AerialPerspective_n)
			   .bind_compute_pipeline("sky_gen_aerial_perspective");
			cmd.dispatch_invocations(AerialPerspectiveWidth, AerialPerspectiveHeight, AerialPerspectiveDepth);
			
		}});
	
	rg.attach_image(atmosphere.Transmittance_n,
		vuk::ImageAttachment::from_texture(atmosphere.transmittance),
		vuk::Access::eComputeSampled,
		vuk::Access::eComputeSampled);
	rg.attach_image(atmosphere.MultiScattering_n,
		vuk::ImageAttachment::from_texture(atmosphere.multiScattering),
		vuk::Access::eComputeSampled,
		vuk::Access::eComputeSampled);
	
	rg.attach_image(CameraView_n,
		vuk::ImageAttachment::from_texture(cameraView),
		vuk::eNone,
		vuk::eNone);
	rg.attach_image(CubemapView_n,
		vuk::ImageAttachment::from_texture(cubemapView),
		vuk::eNone,
		vuk::eNone);
	rg.attach_image(AerialPerspective_n,
		vuk::ImageAttachment::from_texture(aerialPerspective),
		vuk::eNone,
		vuk::eNone);
	
	return rg;
	
}

auto Sky::draw(Buffer<World> const& _world, vuk::Name _targetColor,
	vuk::Name _targetDepth, uvec2 _targetSize) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Background sky",
		.resources = {
			vuk::Resource(atmosphere.Transmittance_n, vuk::Resource::Type::eImage, vuk::eFragmentSampled),
			vuk::Resource(CameraView_n,               vuk::Resource::Type::eImage, vuk::eFragmentSampled),
			vuk::Resource(_targetColor,               vuk::Resource::Type::eImage, vuk::eColorWrite),
			vuk::Resource(_targetDepth,               vuk::Resource::Type::eImage, vuk::eDepthStencilRW) },
		.execute = [this, &_world, _targetSize](vuk::CommandBuffer& cmd) {
			
			cmd.set_viewport(0, vuk::Rect2D{ .extent = vukExtent(_targetSize) })
			   .set_scissor(0, vuk::Rect2D{ .extent = vukExtent(_targetSize) })
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.Transmittance_n, LinearClamp)
			   .bind_sampled_image(1, 0, CameraView_n, LinearClamp)
			   .bind_graphics_pipeline("sky_draw");
			cmd.draw(3, 1, 0, 0);
			
		}});
	
	return rg;
	
}

auto Sky::drawCubemap(Buffer<World> const& _world, Cubemap& _dst) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
		.name = "Cubemap sky",
		.resources = {
			vuk::Resource(atmosphere.Transmittance_n, vuk::Resource::Type::eImage,  vuk::eFragmentSampled),
			vuk::Resource(CubemapView_n,              vuk::Resource::Type::eImage,  vuk::eComputeSampled),
			vuk::Resource(_dst.name,                  vuk::Resource::Type::eImage,  vuk::eComputeWrite),
			vuk::Resource(SunLuminance_n,             vuk::Resource::Type::eBuffer, vuk::eComputeWrite) },
		.execute = [&, this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.Transmittance_n, LinearClamp)
			   .bind_sampled_image(1, 0, CubemapView_n, LinearClamp)
			   .bind_storage_image(1, 1, _dst.name)
			   .bind_storage_buffer(1, 3, *sunLuminance)
			   .bind_compute_pipeline("sky_draw_cubemap");
			
			auto* sides = cmd.map_scratch_uniform_binding<sarray<mat4, 6>>(1, 2);
			*sides = to_array<mat4>({
				mat4(mat3{
					0.0f, 0.0f, -1.0f,
					0.0f, -1.0f, 0.0f,
					1.0f, 0.0f, 0.0f}),
				mat4(mat3{
					0.0f, 0.0f, 1.0f,
					0.0f, -1.0f, 0.0f,
					-1.0f, 0.0f, 0.0f}),
				mat4(mat3{
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f}),
				mat4(mat3{
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, -1.0f,
					0.0f, -1.0f, 0.0f}),
				mat4(mat3{
					1.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 0.0f,
					0.0f, 0.0f, 1.0f}),
				mat4(mat3{
					-1.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 0.0f,
					0.0f, 0.0f, -1.0f})});
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu, CubemapCamera);
			
			cmd.dispatch_invocations(_dst.size().x(), _dst.size().y(), 6);
			
		}});
	
	rg.attach_buffer(SunLuminance_n,
		*sunLuminance,
		vuk::eNone,
		vuk::eNone);
	
	return rg;
}

}
