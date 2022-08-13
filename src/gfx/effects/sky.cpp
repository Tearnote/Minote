#include "gfx/effects/sky.hpp"

#include "imgui.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Partials.hpp"
#include "util/string.hpp"
#include "util/span.hpp"
#include "util/math.hpp"
#include "gfx/samplers.hpp"
#include "gfx/shader.hpp"
#include "sys/vulkan.hpp"

namespace minote {

auto Atmosphere::Params::earth() -> Params {
	
	constexpr auto EarthRayleighScaleHeight = 8.0f;
	constexpr auto EarthMieScaleHeight = 1.2f;
	constexpr auto MieScattering = float3{0.003996f, 0.003996f, 0.003996f};
	constexpr auto MieExtinction = float3{0.004440f, 0.004440f, 0.004440f};
	
	return Params{
		.bottomRadius = 6360.0f,
		.topRadius = 6460.0f,
		.rayleighDensityExpScale = -1.0f / EarthRayleighScaleHeight,
		.rayleighScattering = {0.005802f, 0.013558f, 0.033100f},
		.mieDensityExpScale = -1.0f / EarthMieScaleHeight,
		.mieScattering = MieScattering,
		.mieExtinction = MieExtinction,
		.mieAbsorption = max(MieExtinction - MieScattering, float3{0.0f, 0.0f, 0.0f}),
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

GET_SHADER(sky_genTransmittance_cs);
GET_SHADER(sky_genMultiScattering_cs);
void Atmosphere::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto genTransmittancePci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(genTransmittancePci, sky_genTransmittance_cs, "sky/genTransmittance.cs.hlsl");
	ctx.create_named_pipeline("sky/genTransmittance", genTransmittancePci);
	
	auto genMultiScatteringPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(genMultiScatteringPci, sky_genMultiScattering_cs, "sky/genMultiScattering.cs.hlsl");
	ctx.create_named_pipeline("sky/genMultiScattering", genMultiScatteringPci);
	
	m_compiled = true;
	
}

Atmosphere::Atmosphere(vuk::Allocator& _allocator, Params const& _params) {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("atmosphere");
	rg->attach_image("transmittance", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(TransmittanceSize.x(), TransmittanceSize.y()),
		.format = TransmittanceFormat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("multiScattering", vuk::ImageAttachment{
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
			"transmittance"_image >> vuk::eComputeWrite >> "transmittance/final",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("sky/genTransmittance");
			cmd.bind_buffer(0, 0, "params");
			cmd.bind_image(0, 1, "transmittance");
			
			auto transmittance = *cmd.get_resource_image_attachment("transmittance");
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
			"transmittance/final"_image >> vuk::eComputeSampled,
			"multiScattering"_image >> vuk::eComputeWrite >> "multiScattering/final",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("sky/genMultiScattering")
			   .bind_buffer(0, 0, "params")
			   .bind_image(0, 1, "transmittance/final").bind_sampler(0, 1, LinearClamp)
			   .bind_image(0, 2, "multiScattering");
			
			auto multiScattering = *cmd.get_resource_image_attachment("multiScattering");
			auto multiScatteringSize = multiScattering.extent.extent;
			cmd.specialize_constants(0, multiScatteringSize.width);
			cmd.specialize_constants(1, multiScatteringSize.height);
			
			cmd.dispatch_invocations(multiScatteringSize.width, multiScatteringSize.height);
			
		},
	});
	
	params = vuk::Future(rg, "params");
	transmittance = vuk::Future(rg, "transmittance/final");
	multiScattering = vuk::Future(rg, "multiScattering/final");
	
}

GET_SHADER(sky_genView_cs);
GET_SHADER(sky_draw_cs);
void Sky::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto genViewPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(genViewPci, sky_genView_cs, "sky/genView.cs.hlsl");
	ctx.create_named_pipeline("sky/genView", genViewPci);
	
	auto drawPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(drawPci, sky_draw_cs, "sky/draw.cs.hlsl");
	ctx.create_named_pipeline("sky/draw", drawPci);
	/*
	auto skyGenSunLuminancePci = vuk::ComputePipelineBaseCreateInfo();
	skyGenSunLuminancePci.add_spirv(std::vector<uint>{
#include "spv/sky/genSunLuminance.comp.spv"
	}, "sky/genSunLuminance.comp");
	_ptc.ctx.create_named_pipeline("sky/genSunLuminance", skyGenSunLuminancePci);
	
	auto skyDrawCubemapPci = vuk::ComputePipelineBaseCreateInfo();
	skyDrawCubemapPci.add_spirv(std::vector<uint>{
#include "spv/sky/drawCubemap.comp.spv"
	}, "sky/drawCubemap.comp");
	_ptc.ctx.create_named_pipeline("sky/drawCubemap", skyDrawCubemapPci);
	
	auto skyAerialPerspectivePci = vuk::ComputePipelineBaseCreateInfo();
	skyAerialPerspectivePci.add_spirv(std::vector<uint>{
#include "spv/sky/genAerialPerspective.comp.spv"
	}, "sky/genAerialPerspective.comp");
	_ptc.ctx.create_named_pipeline("sky/genAerialPerspective", skyAerialPerspectivePci);
	*/
	m_compiled = true;
	
}

auto Sky::createView(Atmosphere& _atmo, float3 _probePos) -> Texture2D<float3> {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("sky");
	rg->attach_image("view", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(ViewSize.x(), ViewSize.y()),
		.format = ViewFormat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_in("params", _atmo.params);
	rg->attach_in("transmittance", _atmo.transmittance);
	rg->attach_in("multiScattering", _atmo.multiScattering);
	
	rg->add_pass(vuk::Pass{
		.name = "sky/genView",
		.resources = {
			"params"_buffer >> vuk::eComputeRead,
			"transmittance"_image >> vuk::eComputeSampled,
			"multiScattering"_image >> vuk::eComputeSampled,
			"view"_image >> vuk::eComputeWrite >> "view/final",
		},
		.execute = [this, _probePos](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("sky/genView")
			   .bind_buffer(0, 0, "params")
			   .bind_image(0, 1, "transmittance").bind_sampler(0, 1, LinearClamp)
			   .bind_image(0, 2, "multiScattering").bind_sampler(0, 2, LinearClamp)
			   .bind_image(0, 3, "view");
			
			struct Constants {
				float3 probePos;
				float _pad0;
				float3 sunDirection;
				float _pad1;
				float3 sunIlluminance;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.probePos = _probePos,
				.sunDirection = sunDirection,
				.sunIlluminance = sunIlluminance,
			});
			
			auto view = *cmd.get_resource_image_attachment("view");
			auto viewSize = view.extent.extent;
			cmd.specialize_constants(0, viewSize.width);
			cmd.specialize_constants(1, viewSize.height);
			
			cmd.dispatch_invocations(viewSize.width, viewSize.height);
			
		},
	});
	
	return vuk::Future(rg, "view/final");
	
}

auto Sky::draw(Texture2D<float4> _target, Atmosphere& _atmo,
	Texture2D<float3> _skyView, Camera const& _camera) -> Texture2D<float4> {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("sky");
	rg->attach_in("params", _atmo.params);
	rg->attach_in("transmittance", _atmo.transmittance);
	rg->attach_in("view", _skyView);
	rg->attach_in("target", _target);
	
	rg->add_pass(vuk::Pass{
		.name = "sky/draw",
		.resources = {
			"params"_buffer >> vuk::eComputeRead,
			"transmittance"_image >> vuk::eComputeSampled,
			"view"_image >> vuk::eComputeSampled,
			"target"_image >> vuk::eComputeWrite >> "target/final",
		},
		.execute = [this, &_camera](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("sky/draw")
			   .bind_buffer(0, 0, "params")
			   .bind_image(0, 1, "transmittance").bind_sampler(0, 1, LinearClamp)
			   .bind_image(0, 2, "view").bind_sampler(0, 2, LinearClamp)
			   .bind_image(0, 3, "target");
			
			struct Constants {
				float4x4 viewProjectionInv;
				float3 cameraPos;
				float _pad0;
				float3 sunDirection;
				float _pad1;
				float3 sunIlluminance;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.viewProjectionInv = inverse(_camera.viewProjection()),
				.cameraPos = _camera.position,
				.sunDirection = sunDirection,
				.sunIlluminance = sunIlluminance,
			});
			
			auto view = *cmd.get_resource_image_attachment("view");
			auto viewSize = view.extent.extent;
			cmd.specialize_constants(0, viewSize.width);
			cmd.specialize_constants(1, viewSize.height);
			auto target = *cmd.get_resource_image_attachment("target");
			auto targetSize = target.extent.extent;
			cmd.specialize_constants(2, targetSize.width);
			cmd.specialize_constants(3, targetSize.height);
			
			cmd.dispatch_invocations(targetSize.width, targetSize.height);
			
		},
	});
	
	return vuk::Future(rg, "target/final");
	
}

void Sky::drawImguiDebug(string_view _name) {
	
	ImGui::Begin(string(_name).c_str());
	ImGui::SliderAngle("Sun pitch", &m_sunPitch, -8.0f, 60.0f, "%.1f deg", ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderAngle("Sun yaw", &m_sunYaw, -180.0f, 180.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
	sunDirection = getSunDirection(m_sunPitch, m_sunYaw);
	ImGui::SliderFloat("Sun illuminance", &sunIlluminance.x(), 0.01f, 100.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
	sunIlluminance.y() = sunIlluminance.x();
	sunIlluminance.z() = sunIlluminance.x();
	ImGui::End();
	
}

auto Sky::getSunDirection(float _pitch, float _yaw) -> float3 {
	
	float3 vec = {1.0f, 0.0f, 0.0f};
	vec = mul(vec, float3x3::rotate({0.0f, -1.0f, 0.0f}, _pitch));
	vec = mul(vec, float3x3::rotate({0.0f, 0.0f, 1.0f}, _yaw));
	return vec;
	
}
/*
auto Sky::createAerialPerspective(Pool& _pool, Frame& _frame, vuk::Name _name,
	float3 _probePos, float4x4 _invViewProj, Atmosphere _atmo) -> Texture3D {
	
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
				float4x4 invViewProj;
				float3 probePosition;
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
			cmd.specialize_constants(0, uintFromuint16({aerialPerspective.size().x(), aerialPerspective.size().y()}));
			cmd.specialize_constants(1, aerialPerspective.size().z());
			
			cmd.dispatch_invocations(aerialPerspective.size().x(), aerialPerspective.size().y(), aerialPerspective.size().z());
			
		}});
	
	return aerialPerspective;
	
}

auto Sky::createSunLuminance(Pool& _pool, Frame& _frame, vuk::Name _name,
	float3 _probePos, Atmosphere _atmo) -> Buffer<float3> {
	
	auto sunLuminance = Buffer<float3>::make(_pool, _name,
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

void Sky::draw(Frame& _frame, Cubemap _target, float3 _probePos, Texture2D _skyView, Atmosphere _atmo) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_target.name, "sky/drawCubemap"),
		.resources = {
			_skyView.resource(vuk::eComputeSampled),
			_target.resource(vuk::eComputeWrite) },
		.execute = [_target, _skyView, _atmo, &_frame, _probePos](vuk::CommandBuffer& cmd) {
			
			struct PushConstants {
				float3 probePosition;
				uint cubemapSize;
				uint2 skyViewSize;
			};
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_uniform_buffer(0, 1, _atmo.params)
			   .bind_sampled_image(0, 2, _atmo.transmittance, LinearClamp)
			   .bind_sampled_image(0, 3, _skyView, LinearClamp)
			   .bind_storage_image(0, 4, _target)
			   .bind_compute_pipeline("sky/drawCubemap");
			
			auto* sides = cmd.map_scratch_uniform_binding<array<float4x4, 6>>(0, 5);
			*sides = to_array<float4x4>({
				float4x4(float3x3{
					0.0f, 0.0f, -1.0f,
					0.0f, -1.0f, 0.0f,
					1.0f, 0.0f, 0.0f}),
				float4x4(float3x3{
					0.0f, 0.0f, 1.0f,
					0.0f, -1.0f, 0.0f,
					-1.0f, 0.0f, 0.0f}),
				float4x4(float3x3{
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f}),
				float4x4(float3x3{
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, -1.0f,
					0.0f, -1.0f, 0.0f}),
				float4x4(float3x3{
					1.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 0.0f,
					0.0f, 0.0f, 1.0f}),
				float4x4(float3x3{
					-1.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 0.0f,
					0.0f, 0.0f, -1.0f})});
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _probePos);
			
			cmd.specialize_constants(0, uintFromuint16(_skyView.size()));
			cmd.specialize_constants(1, _target.size().x());
			
			cmd.dispatch_invocations(_target.size().x(), _target.size().y(), 6);
			
		}});
	
}
*/
}
