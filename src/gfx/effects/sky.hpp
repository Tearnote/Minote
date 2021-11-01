#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "base/math.hpp"
#include "gfx/resources/texture3d.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

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
	
	static auto create(Pool&, vuk::RenderGraph&, vuk::Name, Params const&) -> Atmosphere;
	
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
	
	static auto createView(Pool&, vuk::RenderGraph&, vuk::Name, vec3 probePos,
		Atmosphere const&, Buffer<World>) -> Texture2D;
	
	static auto createAerialPerspective(Pool&, vuk::RenderGraph&, vuk::Name,
		vec3 probePos, mat4 invViewProj, Atmosphere const&, Buffer<World>) -> Texture3D;
	
	static auto createSunLuminance(Pool&, vuk::RenderGraph&, vuk::Name,
		vec3 probePos, Atmosphere const&, Buffer<World>) -> Buffer<vec3>;
	
	// Draw the sky in the background of an image (where visibility buffer is
	// empty).
	static void draw(vuk::RenderGraph&, Texture2D target,
		Worklist const&, Texture2D skyView, Atmosphere const&, Buffer<World>);
		
	static void drawQuad(vuk::RenderGraph&, Texture2D target, Texture2D quadbuf,
		Worklist const&, Texture2D skyView, Atmosphere const&, Buffer<World>);
	
	// Draw the sky into an existing cubemap. Target is the mip 0 of provided image.
	static void draw(vuk::RenderGraph&, Cubemap target,
		vec3 probePos, Texture2D skyView, Atmosphere const&, Buffer<World>);
	
};

}
