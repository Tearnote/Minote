#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace minote::gfx {

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

	struct Globals {

		glm::mat4 gSkyInvViewProjMat;
		glm::uvec2 gResolution;
		glm::vec2 RayMarchMinMaxSPP;
		glm::vec3 gSunIlluminance;
		float MultipleScatteringFactor;
		glm::vec3 sun_direction;
		float pad0;
		glm::vec3 camera;

	};

	struct AtmosphereParams {

		float BottomRadius; // Radius of the planet (center to ground)
		float TopRadius; // Maximum considered atmosphere height (center to atmosphere top)

		float RayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
		float pad0;
		glm::vec3 RayleighScattering; // Rayleigh scattering coefficients

		float MieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
		glm::vec3 MieScattering; // Mie scattering coefficients
		float pad1;
		glm::vec3 MieExtinction; // Mie extinction coefficients
		float pad2;
		glm::vec3 MieAbsorption; // Mie absorption coefficients
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
		glm::vec3 AbsorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
		float pad6;

		glm::vec3 GroundAlbedo; // The albedo of the ground.

	};

	vuk::Texture transmittance;
	vuk::Texture multiScattering;
	vuk::Texture skyView;

	explicit Sky(vuk::Context&);

	auto generateAtmosphereModel(AtmosphereParams const&, vuk::PerThreadContext&,
		glm::uvec2 resolution, glm::vec3 cameraPos, glm::mat4 viewProjection) -> vuk::RenderGraph;

};

}
