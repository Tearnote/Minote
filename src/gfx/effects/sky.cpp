#include "gfx/effects/sky.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "base/util.hpp"
#include "gfx/samplers.hpp"
#include "gfx/models.hpp"
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
	
	auto skyGenTransmittancePci = vuk::ComputePipelineBaseCreateInfo();
	skyGenTransmittancePci.add_spirv(std::vector<u32>{
#include "spv/skyGenTransmittance.comp.spv"
	}, "skyGenTransmittance.comp");
	_ptc.ctx.create_named_pipeline("sky_gen_transmittance", skyGenTransmittancePci);

	auto skyGenMultiScatteringPci = vuk::ComputePipelineBaseCreateInfo();
	skyGenMultiScatteringPci.add_spirv(std::vector<u32>{
#include "spv/skyGenMultiScattering.comp.spv"
	}, "skyGenMultiScattering.comp");
	_ptc.ctx.create_named_pipeline("sky_gen_multi_scattering", skyGenMultiScatteringPci);
	
}

auto Atmosphere::create(Pool& _pool, Frame& _frame, vuk::Name _name,
	Params const& _params) -> Atmosphere {
	
	auto result = Atmosphere();
	
	bool calculate = !_pool.contains(nameAppend(_name, "transmittance"));
	
	result.transmittance = Texture2D::make(_pool, nameAppend(_name, "transmittance"),
		TransmittanceSize, TransmittanceFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	result.multiScattering = Texture2D::make(_pool, nameAppend(_name, "multiScattering"),
		MultiScatteringSize, MultiScatteringFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	result.params = Buffer<Params>::make(_pool, nameAppend(_name, "params"),
		vuk::BufferUsageFlagBits::eUniformBuffer,
		std::span(&_params, 1));
	
	if (calculate) {
		
		result.transmittance.attach(_frame.rg, vuk::eNone, vuk::eComputeSampled);
		result.multiScattering.attach(_frame.rg, vuk::eNone, vuk::eComputeSampled);
		
		_frame.rg.add_pass({
			.name = nameAppend(result.transmittance.name, "gen"),
			.resources = {
				result.transmittance.resource(vuk::eComputeWrite) },
			.execute = [result](vuk::CommandBuffer& cmd) {
				
				cmd.bind_uniform_buffer(0, 0, result.params)
				.bind_storage_image(0, 1, result.transmittance)
				.bind_compute_pipeline("sky_gen_transmittance");
				
				cmd.specialize_constants(0, u32Fromu16(result.transmittance.size()));
				
				cmd.dispatch_invocations(result.transmittance.size().x(), result.transmittance.size().y());
				
			}});
		
		_frame.rg.add_pass({
			.name = nameAppend(result.multiScattering.name, "gen"),
			.resources = {
				result.transmittance.resource(vuk::eComputeSampled),
				result.multiScattering.resource(vuk::eComputeWrite) },
			.execute = [result](vuk::CommandBuffer& cmd) {
				
				cmd.bind_uniform_buffer(0, 0, result.params)
				.bind_sampled_image(0, 1, result.transmittance, LinearClamp)
				.bind_storage_image(0, 2, result.multiScattering)
				.bind_compute_pipeline("sky_gen_multi_scattering");
				
				cmd.specialize_constants(0, u32Fromu16(result.multiScattering.size()));
				
				cmd.dispatch_invocations(result.multiScattering.size().x(), result.multiScattering.size().y(), 1);
				
			}});
		
	}
	
	return result;
	
}

void Sky::compile(vuk::PerThreadContext& _ptc) {
	
	auto skyGenSkyViewPci = vuk::ComputePipelineBaseCreateInfo();
	skyGenSkyViewPci.add_spirv(std::vector<u32>{
#include "spv/skyGenSkyView.comp.spv"
	}, "skyGenSkyView.comp");
	_ptc.ctx.create_named_pipeline("sky_gen_sky_view", skyGenSkyViewPci);
	
	auto skyGenSunLuminancePci = vuk::ComputePipelineBaseCreateInfo();
	skyGenSunLuminancePci.add_spirv(std::vector<u32>{
#include "spv/skyGenSunLuminance.comp.spv"
	}, "skyGenSunLuminance.comp");
	_ptc.ctx.create_named_pipeline("sky_gen_sun_luminance", skyGenSunLuminancePci);
	
	auto skyDrawPci = vuk::ComputePipelineBaseCreateInfo();
	skyDrawPci.add_spirv(std::vector<u32>{
#include "spv/skyDraw.comp.spv"
	}, "skyDraw.comp");
	_ptc.ctx.create_named_pipeline("sky_draw", skyDrawPci);
	
	auto skyDrawCubemapPci = vuk::ComputePipelineBaseCreateInfo();
	skyDrawCubemapPci.add_spirv(std::vector<u32>{
#include "spv/skyDrawCubemap.comp.spv"
	}, "skyDrawCubemap.comp");
	_ptc.ctx.create_named_pipeline("sky_draw_cubemap", skyDrawCubemapPci);
	
	auto skyAerialPerspectivePci = vuk::ComputePipelineBaseCreateInfo();
	skyAerialPerspectivePci.add_spirv(std::vector<u32>{
#include "spv/skyGenAerialPerspective.comp.spv"
	}, "skyGenAerialPerspectivePci.comp");
	_ptc.ctx.create_named_pipeline("sky_gen_aerial_perspective", skyAerialPerspectivePci);
	
}

auto Sky::createView(Pool& _pool, Frame& _frame, vuk::Name _name,
	vec3 _probePos, Atmosphere _atmo) -> Texture2D {
	
	auto view = Texture2D::make(_pool, _name,
		ViewSize, ViewFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	view.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "gen"),
		.resources = {
			view.resource(vuk::eComputeWrite) },
		.execute = [view, &_frame, _probePos, _atmo](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				vec3 probePosition;
				f32 pad0;
				uvec2 skyViewSize;
			};
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_uniform_buffer(0, 1, _atmo.params)
			   .bind_sampled_image(0, 2, _atmo.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, _atmo.multiScattering, LinearClamp)
			   .bind_storage_image(0, 4, view)
			   .bind_compute_pipeline("sky_gen_sky_view");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _probePos);
			cmd.specialize_constants(0, u32Fromu16(view.size()));
			
			cmd.dispatch_invocations(view.size().x(), view.size().y(), 1);
			
		}});
	
	return view;
	
}

auto Sky::createAerialPerspective(Pool& _pool, Frame& _frame, vuk::Name _name,
	vec3 _probePos, mat4 _invViewProj, Atmosphere _atmo) -> Texture3D {
	
	auto aerialPerspective = Texture3D::make(_pool, _name,
		AerialPerspectiveSize, AerialPerspectiveFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	aerialPerspective.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "gen"),
		.resources = {
			aerialPerspective.resource(vuk::eComputeWrite) },
		.execute = [aerialPerspective, &_frame, _atmo, _invViewProj, _probePos](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				mat4 invViewProj;
				vec3 probePosition;
			};
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_uniform_buffer(0, 1, _atmo.params)
			   .bind_sampled_image(0, 2, _atmo.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, _atmo.multiScattering, LinearClamp)
			   .bind_storage_image(0, 4, aerialPerspective)
			   .bind_compute_pipeline("sky_gen_aerial_perspective");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, PushConstants{
				   .invViewProj = _invViewProj,
				   .probePosition = _probePos });
			cmd.specialize_constants(0, u32Fromu16({aerialPerspective.size().x(), aerialPerspective.size().y()}));
			cmd.specialize_constants(1, aerialPerspective.size().z());
			
			cmd.dispatch_invocations(aerialPerspective.size().x(), aerialPerspective.size().y(), aerialPerspective.size().z());
			
		}});
	
	return aerialPerspective;
	
}

auto Sky::createSunLuminance(Pool& _pool, Frame& _frame, vuk::Name _name,
	vec3 _probePos, Atmosphere _atmo) -> Buffer<vec3> {
	
	auto sunLuminance = Buffer<vec3>::make(_pool, _name,
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eUniformBuffer);
	
	sunLuminance.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "gen"),
		.resources = {
			sunLuminance.resource(vuk::eComputeWrite) },
		.execute = [sunLuminance,&_frame, _atmo, _probePos](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_uniform_buffer(0, 1, _atmo.params)
			   .bind_sampled_image(0, 2, _atmo.transmittance, LinearClamp)
			   .bind_storage_buffer(0, 3, sunLuminance)
			   .bind_compute_pipeline("sky_gen_sun_luminance");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _probePos);
			
			cmd.dispatch_invocations(1);
			
		}});
	
	return sunLuminance;
	
}

void Sky::draw(Frame& _frame, QuadBuffer& _quadbuf, Worklist _worklist,
	Texture2D _skyView, Atmosphere _atmo) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_quadbuf.name, "sky Quad"),
		.resources = {
			_skyView.resource(vuk::eComputeSampled),
			_worklist.counts.resource(vuk::eIndirectRead),
			_worklist.lists.resource(vuk::eComputeRead),
			_quadbuf.visbuf.resource(vuk::eComputeSampled),
			_quadbuf.offset.resource(vuk::eComputeSampled),
			_quadbuf.clusterOut.resource(vuk::eComputeWrite) },
		.execute = [_worklist, _skyView, _atmo, _quadbuf, &_frame,
			tileCount=_worklist.counts.offsetView(+MaterialType::None)](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_uniform_buffer(0, 1, _atmo.params)
			   .bind_sampled_image(0, 2, _atmo.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, _skyView, LinearClamp)
			   .bind_storage_buffer(0, 4, _worklist.lists)
			   .bind_sampled_image(0, 5, _quadbuf.visbuf, NearestClamp)
			   .bind_sampled_image(0, 6, _quadbuf.offset, NearestClamp)
			   .bind_storage_image(0, 7, _quadbuf.clusterOut)
			   .bind_compute_pipeline("sky_draw");
			
			cmd.specialize_constants(0, u32Fromu16(_skyView.size()));
			cmd.specialize_constants(1, u32Fromu16(_quadbuf.clusterOut.size()));
			cmd.specialize_constants(2, _worklist.tileDimensions.x() * _worklist.tileDimensions.y() * +MaterialType::None);
			
			cmd.dispatch_indirect(tileCount);
			
		}});
	
}

void Sky::draw(Frame& _frame, Cubemap _target, vec3 _probePos, Texture2D _skyView, Atmosphere _atmo) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_target.name, "sky"),
		.resources = {
			_skyView.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_target, _skyView, _atmo, &_frame, _probePos](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				vec3 probePosition;
				u32 cubemapSize;
				uvec2 skyViewSize;
			};
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_uniform_buffer(0, 1, _atmo.params)
			   .bind_sampled_image(0, 2, _atmo.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, _skyView, LinearClamp)
			   .bind_storage_image(0, 4, _target)
			   .bind_compute_pipeline("sky_draw_cubemap");
			
			auto* sides = cmd.map_scratch_uniform_binding<array<mat4, 6>>(0, 5);
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
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _probePos);
			
			cmd.specialize_constants(0, u32Fromu16(_skyView.size()));
			cmd.specialize_constants(1, _target.size().x());
			
			cmd.dispatch_invocations(_target.size().x(), _target.size().y(), 6);
			
		}});
	
}

}
