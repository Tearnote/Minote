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
			
			cmd.bind_uniform_buffer(0, 0, params)
			   .bind_storage_image(0, 1, transmittance)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, transmittance.size())
			   .bind_compute_pipeline("sky_gen_transmittance");
			cmd.dispatch_invocations(transmittance.size().x(), transmittance.size().y());
			
		}});
	
	_rg.add_pass({
		.name = "Sky multiple scattering LUT",
		.resources = {
			transmittance.resource(vuk::eComputeSampled),
			multiScattering.resource(vuk::eComputeWrite) },
		.execute = [this](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, params)
			   .bind_sampled_image(0, 1, transmittance, LinearClamp)
			   .bind_storage_image(0, 2, multiScattering)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, multiScattering.size())
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
	
	auto skyDrawPci = vuk::ComputePipelineCreateInfo();
	skyDrawPci.add_spirv(std::vector<u32>{
#include "spv/skyDraw.comp.spv"
	}, "skyDraw.comp");
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

Sky::Sky(Pool& _pool, vuk::Name _name, Atmosphere const& _atmosphere):
	name(_name), atmosphere(_atmosphere) {
	
	cameraView = Texture2D::make(_pool, nameAppend(_name, "cameraView"),
		ViewSize, ViewFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	cubemapView = Texture2D::make(_pool, nameAppend(_name, "cubemapView"),
		ViewSize, ViewFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	aerialPerspective = Texture3D::make(_pool, nameAppend(_name, "aerialPerspective"),
		AerialPerspectiveSize, AerialPerspectiveFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	sunLuminance = Buffer<vec3>::make(_pool, nameAppend(_name, "sunLuminance"),
		vuk::BufferUsageFlagBits::eStorageBuffer);
	
}

void Sky::calculate(vuk::RenderGraph& _rg, Buffer<World> _world, Camera const& _camera) {
	
	atmosphere.transmittance.attach(_rg, vuk::eComputeSampled, vuk::eComputeSampled);
	atmosphere.multiScattering.attach(_rg, vuk::eComputeSampled, vuk::eComputeSampled);
	
	cameraView.attach(_rg, vuk::eNone, vuk::eNone);
	cubemapView.attach(_rg, vuk::eNone, vuk::eNone);
	aerialPerspective.attach(_rg, vuk::eNone, vuk::eNone);
	
	_rg.add_pass({
		.name = nameAppend(name, "view LUT"),
		.resources = {
			atmosphere.transmittance.resource(vuk::eComputeSampled),
			atmosphere.multiScattering.resource(vuk::eComputeSampled),
			cameraView.resource(vuk::eComputeWrite) },
		.execute = [this, _world, &_camera](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				vec3 probePosition;
				f32 pad0;
				uvec2 skyViewSize;
			};
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.multiScattering, LinearClamp)
			   .bind_storage_image(0, 4, cameraView)
			   .bind_compute_pipeline("sky_gen_sky_view");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, PushConstants{
				.probePosition = _camera.position,
				.skyViewSize = cameraView.size() });
			
			cmd.dispatch_invocations(cameraView.size().x(), cameraView.size().y(), 1);
			
		}});
	
	_rg.add_pass({
		.name = nameAppend(name, "cubemap view LUT"),
		.resources = {
			atmosphere.transmittance.resource(vuk::eComputeSampled),
			atmosphere.multiScattering.resource(vuk::eComputeSampled),
			cubemapView.resource(vuk::eComputeWrite) },
		.execute = [this, _world](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				vec3 probePosition;
				f32 pad0;
				uvec2 skyViewSize;
			};
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.multiScattering, LinearClamp)
			   .bind_storage_image(0, 4, cubemapView)
			   .bind_compute_pipeline("sky_gen_sky_view");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, PushConstants{
				.probePosition = CubemapCamera,
				.skyViewSize = cubemapView.size() });
			
			cmd.dispatch_invocations(cubemapView.size().x(), cubemapView.size().y(), 1);
			
		}});
	
	_rg.add_pass({
		.name = nameAppend(name, "aerial perspective LUT"),
		.resources = {
			atmosphere.transmittance.resource(vuk::eComputeSampled),
			atmosphere.multiScattering.resource(vuk::eComputeSampled),
			aerialPerspective.resource(vuk::eComputeWrite) },
		.execute = [this, _world](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, atmosphere.multiScattering, LinearClamp)
			   .bind_storage_image(0, 4, aerialPerspective)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, aerialPerspective.size())
			   .bind_compute_pipeline("sky_gen_aerial_perspective");
			cmd.dispatch_invocations(aerialPerspective.size().x(), aerialPerspective.size().y(), aerialPerspective.size().z());
			
		}});
	
}

void Sky::draw(vuk::RenderGraph& _rg, Buffer<World> _world, Texture2D _target, Texture2D _visbuf) {
	
	_rg.add_pass({
		.name = nameAppend(name, "background"),
		.resources = {
			atmosphere.transmittance.resource(vuk::eComputeSampled),
			cameraView.resource(vuk::eComputeSampled),
			_visbuf.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [this, _world, _target, _visbuf](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				uvec2 skyViewSize;
				uvec2 targetSize;
			};
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, cameraView, LinearClamp)
			   .bind_sampled_image(0, 4, _visbuf, NearestClamp)
			   .bind_storage_image(0, 5, _target)
			   .bind_compute_pipeline("sky_draw");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, PushConstants{
				.skyViewSize = cameraView.size(),
				.targetSize = _target.size() });
			
			cmd.dispatch_invocations(_target.size().x(), _target.size().y());
			
		}});
	
}

void Sky::drawCubemap(vuk::RenderGraph& _rg, Buffer<World> _world, Cubemap _target) {
	
	sunLuminance.attach(_rg, vuk::eNone, vuk::eNone);
	
	_rg.add_pass({
		.name = nameAppend(name, "cubemap"),
		.resources = {
			atmosphere.transmittance.resource(vuk::eFragmentSampled),
			cubemapView.resource(vuk::eComputeSampled),
			vuk::Resource(_target.name, vuk::Resource::Type::eImage,  vuk::eComputeWrite),
			sunLuminance.resource(vuk::eComputeWrite) },
		.execute = [this, _world, _target](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				vec3 probePosition;
				u32 cubemapSize;
				uvec2 skyViewSize;
			};
			
			cmd.bind_uniform_buffer(0, 0, _world)
			   .bind_uniform_buffer(0, 1, atmosphere.params)
			   .bind_sampled_image(0, 2, atmosphere.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, cubemapView, LinearClamp)
			   .bind_storage_image(0, 4, _target.name)
			   .bind_storage_buffer(0, 5, sunLuminance)
			   .bind_compute_pipeline("sky_draw_cubemap");
			
			auto* sides = cmd.map_scratch_uniform_binding<array<mat4, 6>>(0, 6);
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
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, PushConstants{
				.probePosition = CubemapCamera,
				.cubemapSize = _target.size().x(),
				.skyViewSize = cubemapView.size() });
			
			cmd.dispatch_invocations(_target.size().x(), _target.size().y(), 6);
			
		}});
	
}

}
