#pragma once

#include "vuk/Future.hpp"
#include "gfx/renderer.hpp"
#include "gfx/camera.hpp"
#include "util/math.hpp"

namespace minote {

// Precalculated representation of a planet's atmosphere. Once created, it can
// be used repeatedly to sample the sky at any elevation and sun position
struct Atmosphere {
	
	constexpr static auto TransmittanceFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto TransmittanceSize = uvec2{256u, 64u};
	
	constexpr static auto MultiScatteringFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MultiScatteringSize = uvec2{32u, 32u};
	
	struct Params {
	
		f32 bottomRadius; // Radius of the planet (center to ground)
		f32 topRadius; // Maximum considered atmosphere height (center to atmosphere top)
		
		f32 rayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
		f32 pad0;
		vec3 rayleighScattering; // Rayleigh scattering coefficients
		
		f32 mieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
		vec3 mieScattering; // Mie scattering coefficients
		f32 pad1;
		vec3 mieExtinction; // Mie extinction coefficients
		f32 pad2;
		vec3 mieAbsorption; // Mie absorption coefficients
		f32 miePhaseG; // Mie phase function excentricity
		
		// Another medium type in the atmosphere
		f32 absorptionDensity0LayerWidth;
		f32 absorptionDensity0ConstantTerm;
		f32 absorptionDensity0LinearTerm;
		f32 absorptionDensity1ConstantTerm;
		f32 absorptionDensity1LinearTerm;
		f32 pad3;
		f32 pad4;
		f32 pad5;
		vec3 absorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
		f32 pad6;
		
		vec3 groundAlbedo;
		
		// Return params that model Earth's atmosphere
		static auto earth() -> Params;
		
	};
	
	vuk::Future transmittance; // 2D texture
	vuk::Future multiScattering; // 2D texture
	vuk::Future params; // Buffer<Params>
	
	// Create and precalculate the atmosphere data
	Atmosphere(vuk::Allocator&, Params const&);
	
	// Build required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

// Rendering of the sky from atmosphere data. Sky views depend on camera position
struct Sky {
	
	constexpr static auto ViewFormat = vuk::Format::eB10G11R11UfloatPack32;
	constexpr static auto ViewSize = uvec2{192u, 108u};
	
	constexpr static auto AerialPerspectiveFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto AerialPerspectiveSize = uvec3{32u, 32u, 32u};
	
	vec3 sunDirection = {-0.435286462, 0.818654716, 0.374606609};
	vec3 sunIlluminance = vec3(8.0f);
	
	// Create a 360-degree view of the sky at the specified world position
	auto createView(Atmosphere&, vec3 probePos) -> vuk::Future;
	
	// Draw the sky into a texture at camera position
	auto draw(vuk::Future target, Atmosphere&, vuk::Future skyView, Camera const&) -> vuk::Future;
	
	// Build required shaders; optional
	static void compile();
	
	// Draw debug controls for this instance
	void drawImguiDebug(string_view name);
	
	// Compute direction from pitch and yaw
	auto getSunDirection(f32 pitch, f32 yaw) -> vec3;
	
private:
	
	static inline bool m_compiled = false;
	
	// Imgui controls
	f32 m_sunPitch = 22_deg;
	f32 m_sunYaw = 118_deg;
	/*
	static auto createAerialPerspective(Pool&, Frame&, vuk::Name,
		vec3 probePos, mat4 invViewProj, Atmosphere) -> Texture3D;
	
	static auto createSunLuminance(Pool&, Frame&, vuk::Name,
		vec3 probePos, Atmosphere) -> Buffer<vec3>;
	
	// Draw the sky into an existing cubemap. Target is the mip 0 of provided image.
	static void draw(Frame&, Cubemap target, vec3 probePos, Texture2D skyView, Atmosphere);
	*/
};

}
