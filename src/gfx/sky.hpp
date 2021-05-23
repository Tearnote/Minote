#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

struct Sky {
	
	constexpr static auto TransmittanceFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto TransmittanceWidth = 256u;
	constexpr static auto TransmittanceHeight = 64u;
	
	constexpr static auto MultiScatteringFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MultiScatteringWidth = 32u;
	constexpr static auto MultiScatteringHeight = 32u;
	
	constexpr static auto SkyViewFormat = vuk::Format::eB10G11R11UfloatPack32;
	constexpr static auto SkyViewWidth = 192u;
	constexpr static auto SkyViewHeight = 108u;
	
	constexpr static auto AerialPerspectiveFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto AerialPerspectiveWidth = 32u;
	constexpr static auto AerialPerspectiveHeight = 32u;
	constexpr static auto AerialPerspectiveDepth = 32u;
	
	constexpr static auto CubemapHeight = 0.2f;
	
	struct AtmosphereParams {
		
		float BottomRadius; // Radius of the planet (center to ground)
		float TopRadius; // Maximum considered atmosphere height (center to atmosphere top)
		
		float RayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
		float pad0;
		vec3 RayleighScattering; // Rayleigh scattering coefficients
		
		float MieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
		vec3 MieScattering; // Mie scattering coefficients
		float pad1;
		vec3 MieExtinction; // Mie extinction coefficients
		float pad2;
		vec3 MieAbsorption; // Mie absorption coefficients
		float MiePhaseG; // Mie phase function excentricity
		
		// Another medium type in the atmosphere
		float AbsorptionDensity0LayerWidth;
		float AbsorptionDensity0ConstantTerm;
		float AbsorptionDensity0LinearTerm;
		float AbsorptionDensity1ConstantTerm;
		float AbsorptionDensity1LinearTerm;
		float pad3;
		float pad4;
		float pad5;
		vec3 AbsorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
		float pad6;
		
		vec3 GroundAlbedo; // The albedo of the ground.
		
	};
	
	vuk::Texture transmittance;
	vuk::Texture multiScattering;
	vuk::Texture skyView;
	vuk::Texture skyCubemapView;
	vuk::Texture aerialPerspective;
	
	explicit Sky(vuk::Context&);
	
	auto generateAtmosphereModel(AtmosphereParams const&, vuk::Buffer world, vuk::PerThreadContext&) -> vuk::RenderGraph;
	
	auto draw(AtmosphereParams const&, vuk::Name targetColor, vuk::Name targetDepth, vuk::Buffer world, vuk::PerThreadContext&) -> vuk::RenderGraph;
	
	auto drawCubemap(AtmosphereParams const&, vuk::Name target, u32 targetSize, vuk::Buffer world, vuk::PerThreadContext&) -> vuk::RenderGraph;
	
};

}
