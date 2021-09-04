#include "gfx/effects/sky.hpp"

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
	
	constexpr auto EarthRayleighScaleHeight = 8.0f;
	constexpr auto EarthMieScaleHeight = 1.2f;
	constexpr auto MieScattering = vec3{0.003996f, 0.003996f, 0.003996f};
	constexpr auto MieExtinction = vec3{0.004440f, 0.004440f, 0.004440f};
	
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

void Atmosphere::upload(Pool& _pool, vuk::Name _name, Params const& _params) {
	
	transmittance = Texture2D::make(_pool, nameAppend(_name, "transmittance"),
		TransmittanceSize, TransmittanceFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	multiScattering = Texture2D::make(_pool, nameAppend(_name, "multiScattering"),
		MultiScatteringSize, MultiScatteringFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	params = Buffer<Params>::make(_pool, nameAppend(_name, "params"),
		vuk::BufferUsageFlagBits::eUniformBuffer,
		std::span(&_params, 1),
		vuk::MemoryUsage::eGPUonly);
	
}

void Atmosphere::precalculate(vuk::RenderGraph& _rg) {
	
	transmittance.attach(_rg, vuk::eNone, vuk::Access::eComputeSampled);
	multiScattering.attach(_rg, vuk::eNone, vuk::Access::eComputeSampled);
	
	_rg.add_pass({
		.name = "Sky transmittance LUT",
		.resources = {
			transmittance.resource(vuk::eComputeWrite) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 1, params)
			   .bind_storage_image(1, 0, transmittance)
			   .bind_compute_pipeline("sky_gen_transmittance");
			cmd.dispatch_invocations(transmittance.size().x(), transmittance.size().y());
			
		}});
	
	_rg.add_pass({
		.name = "Sky multiple scattering LUT",
		.resources = {
			transmittance.resource(vuk::eComputeSampled),
			multiScattering.resource(vuk::eComputeWrite) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 1, params)
			   .bind_sampled_image(0, 2, transmittance, LinearClamp)
			   .bind_storage_image(1, 0, multiScattering)
			   .bind_compute_pipeline("sky_gen_multi_scattering");
			cmd.dispatch_invocations(multiScattering.size().x(), multiScattering.size().y(), 1);
			
		}});
	
}

void Sky::compile(vuk::PerThreadContext& _ptc) {
	
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
	
}

void Sky::calculate(vuk::RenderGraph& _rg, Buffer<World> _world, Camera const& _camera) {
	
	atmosphere.transmittance.attach(_rg, vuk::eComputeSampled, vuk::eComputeSampled);
	atmosphere.multiScattering.attach(_rg, vuk::eComputeSampled, vuk::eComputeSampled);
	
	_rg.add_pass({
		.name = "Sky view LUT",
		.resources = {
			atmosphere.transmittance.resource(vuk::eComputeSampled),
			atmosphere.multiScattering.resource(vuk::eComputeSampled),
			vuk::Resource(CameraView_n, vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this, _world, _camera](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.multiScattering, LinearClamp)
			   .bind_storage_image(1, 0, CameraView_n)
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu, _camera.position);
			cmd.dispatch_invocations(ViewWidth, ViewHeight, 1);
			
		}});
	
	_rg.add_pass({
		.name = "Sky cubemap view LUT",
		.resources = {
			atmosphere.transmittance.resource(vuk::eComputeSampled),
			atmosphere.multiScattering.resource(vuk::eComputeSampled),
			vuk::Resource(CubemapView_n, vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.multiScattering, LinearClamp)
			   .bind_storage_image(1, 0, CubemapView_n)
			   .bind_compute_pipeline("sky_gen_sky_view");
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0_zu, CubemapCamera);
			cmd.dispatch_invocations(ViewWidth, ViewHeight, 1);
			
		}});
	
	_rg.add_pass({
		.name = "Sky aerial perspective LUT",
		.resources = {
			atmosphere.transmittance.resource(vuk::eComputeSampled),
			atmosphere.multiScattering.resource(vuk::eComputeSampled),
			vuk::Resource(AerialPerspective_n, vuk::Resource::Type::eImage, vuk::eComputeWrite) },
		.execute = [this, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.multiScattering, LinearClamp)
			   .bind_storage_image(1, 0, AerialPerspective_n)
			   .bind_compute_pipeline("sky_gen_aerial_perspective");
			cmd.dispatch_invocations(AerialPerspectiveWidth, AerialPerspectiveHeight, AerialPerspectiveDepth);
			
		}});
	
	_rg.attach_image(CameraView_n,
		vuk::ImageAttachment::from_texture(cameraView),
		vuk::eNone,
		vuk::eNone);
	_rg.attach_image(CubemapView_n,
		vuk::ImageAttachment::from_texture(cubemapView),
		vuk::eNone,
		vuk::eNone);
	_rg.attach_image(AerialPerspective_n,
		vuk::ImageAttachment::from_texture(aerialPerspective),
		vuk::eNone,
		vuk::eNone);
	
}

void Sky::draw(vuk::RenderGraph& _rg, Buffer<World> _world, vuk::Name _targetColor,
	vuk::Name _targetDepth, uvec2 _targetSize) {
	
	_rg.add_pass({
		.name = "Background sky",
		.resources = {
			atmosphere.transmittance.resource(vuk::eFragmentSampled),
			vuk::Resource(CameraView_n, vuk::Resource::Type::eImage, vuk::eFragmentSampled),
			vuk::Resource(_targetColor, vuk::Resource::Type::eImage, vuk::eColorWrite),
			vuk::Resource(_targetDepth, vuk::Resource::Type::eImage, vuk::eDepthStencilRW) },
		.execute = [this, _world, _targetSize](vuk::CommandBuffer& cmd) {
			
			cmd.set_viewport(0, vuk::Rect2D{ .extent = vukExtent(_targetSize) })
			   .set_scissor(0, vuk::Rect2D{ .extent = vukExtent(_targetSize) })
			   .bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(1, 0, CameraView_n, LinearClamp)
			   .bind_graphics_pipeline("sky_draw");
			cmd.draw(3, 1, 0, 0);
			
		}});
	
}

void Sky::drawCubemap(vuk::RenderGraph& _rg, Buffer<World> _world, Cubemap _target) {
	
	_rg.add_pass({
		.name = "Cubemap sky",
		.resources = {
			atmosphere.transmittance.resource(vuk::eFragmentSampled),
			vuk::Resource(CubemapView_n,  vuk::Resource::Type::eImage,  vuk::eComputeSampled),
			vuk::Resource(_target.name,      vuk::Resource::Type::eImage,  vuk::eComputeWrite),
			vuk::Resource(SunLuminance_n, vuk::Resource::Type::eBuffer, vuk::eComputeWrite) },
		.execute = [this, _world, _target](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(1, 0, CubemapView_n, LinearClamp)
			   .bind_storage_image(1, 1, _target.name)
			   .bind_storage_buffer(1, 3, *sunLuminance)
			   .bind_compute_pipeline("sky_draw_cubemap");
			
			auto* sides = cmd.map_scratch_uniform_binding<array<mat4, 6>>(1, 2);
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
			
			cmd.dispatch_invocations(_target.size().x(), _target.size().y(), 6);
			
		}});
	
	_rg.attach_buffer(SunLuminance_n,
		*sunLuminance,
		vuk::eNone,
		vuk::eNone);
	
}

}
