#pragma once

#include "vuk/Context.hpp"
#include "util/math.hpp"
#include "gfx/resources/texture3d.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/quadBuffer.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/camera.hpp"
#include "gfx/frame.hpp"

namespace minote {

// Precalculated representation of a planet's atmosphere. Once created, it can
// be used repeatedly to sample the sky at any elevation and sun position.
struct Atmosphere {
	
	constexpr static auto TransmittanceFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto TransmittanceSize = uvec2{256u, 64u};
	
	constexpr static auto MultiScatteringFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MultiScatteringSize = uvec2{32u, 32u};
	
	struct Params {
	
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
		
		vec3 GroundAlbedo;
		
		// Return params that model Earth's atmosphere
		static auto earth() -> Params;
		
	};
	
	Texture2D transmittance;
	Texture2D multiScattering;
	Buffer<Params> params;
	
	// Build required shaders.
	static void compile(vuk::PerThreadContext&);
	
	static auto create(Pool&, Frame&, vuk::Name, Params const&) -> Atmosphere;
	
};

// Effect for rendering sky backgrounds, IBL cubemaps and other position-dependent
// lookup tables.
struct Sky {
	
	constexpr static auto ViewFormat = vuk::Format::eB10G11R11UfloatPack32;
	constexpr static auto ViewSize = uvec2{192u, 108u};
	
	constexpr static auto AerialPerspectiveFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto AerialPerspectiveSize = uvec3{32u, 32u, 32u};
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	static auto createView(Pool&, Frame&, vuk::Name, vec3 probePos, Atmosphere) -> Texture2D;
	
	static auto createAerialPerspective(Pool&, Frame&, vuk::Name,
		vec3 probePos, mat4 invViewProj, Atmosphere) -> Texture3D;
	
	static auto createSunLuminance(Pool&, Frame&, vuk::Name,
		vec3 probePos, Atmosphere) -> Buffer<vec3>;
	
	static void draw(Frame&, QuadBuffer&, Worklist, Texture2D skyView, Atmosphere);
	
	// Draw the sky into an existing cubemap. Target is the mip 0 of provided image.
	static void draw(Frame&, Cubemap target, vec3 probePos, Texture2D skyView, Atmosphere);
	
};

}
