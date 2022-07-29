#include "gfx/effects/sky.hpp"

#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Partials.hpp"
#include "util/span.hpp"
#include "util/math.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"
#include "sys/vulkan.hpp"

#include "spv/sky/genTransmittance.cs.hpp"
#include "spv/sky/genMultiScattering.cs.hpp"

namespace minote {

auto Atmosphere::Params::earth() -> Params {
	
	constexpr auto EarthRayleighScaleHeight = 8.0f;
	constexpr auto EarthMieScaleHeight = 1.2f;
	constexpr auto MieScattering = vec3{0.003996f, 0.003996f, 0.003996f};
	constexpr auto MieExtinction = vec3{0.004440f, 0.004440f, 0.004440f};
	
	return Params{
		.bottomRadius = 6360.0f,
		.topRadius = 6460.0f,
		.rayleighDensityExpScale = -1.0f / EarthRayleighScaleHeight,
		.rayleighScattering = {0.005802f, 0.013558f, 0.033100f},
		.mieDensityExpScale = -1.0f / EarthMieScaleHeight,
		.mieScattering = MieScattering,
		.mieExtinction = MieExtinction,
		.mieAbsorption = max(MieExtinction - MieScattering, vec3(0.0f)),
		.miePhaseG = 0.8f,
		.absorptionDensity0LayerWidth = 25.0f,
		.absorptionDensity0ConstantTerm = -2.0f / 3.0f,
		.absorptionDensity0LinearTerm = 1.0f / 15.0f,
		.absorptionDensity1ConstantTerm = 8.0f / 3.0f,
		.absorptionDensity1LinearTerm = -1.0f / 15.0f,
		.absorptionExtinction = {0.000650f, 0.001881f, 0.000085f},
		.groundAlbedo = {0.0f, 0.0f, 0.0f},
	};
	
}

void Atmosphere::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto skyGenTransmittancePci = vuk::PipelineBaseCreateInfo();
	addSpirv(skyGenTransmittancePci, sky_genTransmittance_cs, "sky/genTransmittance.cs.hlsl");
	ctx.create_named_pipeline("sky/genTransmittance", skyGenTransmittancePci);

	auto skyGenMultiScatteringPci = vuk::PipelineBaseCreateInfo();
	addSpirv(skyGenMultiScatteringPci, sky_genMultiScattering_cs, "sky/genMultiScattering.cs.hlsl");
	ctx.create_named_pipeline("sky/genMultiScattering", skyGenMultiScatteringPci);
	
	m_compiled = true;
	
}

Atmosphere::Atmosphere(vuk::Allocator& _allocator, Params const& _params) {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>();
	rg->attach_image("transmittance/uninit", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(TransmittanceSize.x(), TransmittanceSize.y()),
		.format = TransmittanceFormat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("multiScattering/uninit", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(MultiScatteringSize.x(), MultiScatteringSize.y()),
		.format = MultiScatteringFormat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	auto paramsFut = vuk::create_buffer_gpu(_allocator,
		vuk::DomainFlagBits::eGraphicsQueue,
		span(&_params, 1)).second;
	rg->attach_in("params", paramsFut);
	
	rg->add_pass(vuk::Pass{
		.name = "sky/genTransmittance",
		.resources = {
			"params"_buffer >> vuk::eComputeRead,
			"transmittance/uninit"_image >> vuk::eComputeWrite >> "transmittance",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("sky/genTransmittance");
			cmd.bind_buffer(0, 0, "params");
			cmd.bind_image(0, 1, "transmittance/uninit");
			
			auto transmittance = *cmd.get_resource_image_attachment("transmittance/uninit");
			auto transmittanceSize = transmittance.extent.extent;
			cmd.specialize_constants(0, transmittanceSize.width);
			cmd.specialize_constants(1, transmittanceSize.height);
			
			cmd.dispatch_invocations(transmittanceSize.width, transmittanceSize.height);
			
		},
	});
	
	rg->add_pass({
		.name = "sky/genMultiScattering",
		.resources = {
			"params"_buffer >> vuk::eComputeRead,
			"transmittance"_image >> vuk::eComputeSampled,
			"multiScattering/uninit"_image >> vuk::eComputeWrite >> "multiScattering",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("sky/genMultiScattering")
			   .bind_buffer(0, 0, "params")
			   .bind_image(0, 1, "transmittance").bind_sampler(0, 1, LinearClamp)
			   .bind_image(0, 2, "multiScattering/uninit");
			
			auto multiScattering = *cmd.get_resource_image_attachment("multiScattering/uninit");
			auto multiScatteringSize = multiScattering.extent.extent;
			cmd.specialize_constants(0, multiScatteringSize.width);
			cmd.specialize_constants(1, multiScatteringSize.height);
			
			cmd.dispatch_invocations(multiScatteringSize.width, multiScatteringSize.height);
		},
	});
	
	params = vuk::Future(rg, "params");
	transmittance = vuk::Future(rg, "transmittance");
	multiScattering = vuk::Future(rg, "multiScattering");
	
}
/*
void Sky::compile(vuk::PerThreadContext& _ptc) {
	
	auto skyGenSkyViewPci = vuk::ComputePipelineBaseCreateInfo();
	skyGenSkyViewPci.add_spirv(std::vector<u32>{
#include "spv/sky/genSkyView.comp.spv"
	}, "sky/genSkyView.comp");
	_ptc.ctx.create_named_pipeline("sky/genSkyView", skyGenSkyViewPci);
	
	auto skyGenSunLuminancePci = vuk::ComputePipelineBaseCreateInfo();
	skyGenSunLuminancePci.add_spirv(std::vector<u32>{
#include "spv/sky/genSunLuminance.comp.spv"
	}, "sky/genSunLuminance.comp");
	_ptc.ctx.create_named_pipeline("sky/genSunLuminance", skyGenSunLuminancePci);
	
	auto skyDrawPci = vuk::ComputePipelineBaseCreateInfo();
	skyDrawPci.add_spirv(std::vector<u32>{
#include "spv/sky/draw.comp.spv"
	}, "sky/draw.comp");
	_ptc.ctx.create_named_pipeline("sky/draw", skyDrawPci);
	
	auto skyDrawCubemapPci = vuk::ComputePipelineBaseCreateInfo();
	skyDrawCubemapPci.add_spirv(std::vector<u32>{
#include "spv/sky/drawCubemap.comp.spv"
	}, "sky/drawCubemap.comp");
	_ptc.ctx.create_named_pipeline("sky/drawCubemap", skyDrawCubemapPci);
	
	auto skyAerialPerspectivePci = vuk::ComputePipelineBaseCreateInfo();
	skyAerialPerspectivePci.add_spirv(std::vector<u32>{
#include "spv/sky/genAerialPerspective.comp.spv"
	}, "sky/genAerialPerspective.comp");
	_ptc.ctx.create_named_pipeline("sky/genAerialPerspective", skyAerialPerspectivePci);
	
}

auto Sky::createView(Pool& _pool, Frame& _frame, vuk::Name _name,
	vec3 _probePos, Atmosphere _atmo) -> Texture2D {
	
	auto view = Texture2D::make(_pool, _name,
		ViewSize, ViewFormat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	view.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "sky/genSkyView"),
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
			   .bind_compute_pipeline("sky/genSkyView");
			
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
		.name = nameAppend(_name, "sky/genAerialPerspective"),
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
			   .bind_compute_pipeline("sky/genAerialPerspective");
			
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
		.name = nameAppend(_name, "sky/genSunLuminance"),
		.resources = {
			sunLuminance.resource(vuk::eComputeWrite) },
		.execute = [sunLuminance,&_frame, _atmo, _probePos](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_uniform_buffer(0, 1, _atmo.params)
			   .bind_sampled_image(0, 2, _atmo.transmittance, LinearClamp)
			   .bind_storage_buffer(0, 3, sunLuminance)
			   .bind_compute_pipeline("sky/genSunLuminance");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _probePos);
			
			cmd.dispatch_invocations(1);
			
		}});
	
	return sunLuminance;
	
}

void Sky::draw(Frame& _frame, QuadBuffer& _quadbuf, Worklist _worklist,
	Texture2D _skyView, Atmosphere _atmo) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_quadbuf.name, "sky/draw"),
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
			   .bind_compute_pipeline("sky/draw");
			
			cmd.specialize_constants(0, u32Fromu16(_skyView.size()));
			cmd.specialize_constants(1, u32Fromu16(_quadbuf.clusterOut.size()));
			cmd.specialize_constants(2, _worklist.tileDimensions.x() * _worklist.tileDimensions.y() * +MaterialType::None);
			
			cmd.dispatch_indirect(tileCount);
			
		}});
	
}

void Sky::draw(Frame& _frame, Cubemap _target, vec3 _probePos, Texture2D _skyView, Atmosphere _atmo) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_target.name, "sky/drawCubemap"),
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
			   .bind_compute_pipeline("sky/drawCubemap");
			
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
*/
}
