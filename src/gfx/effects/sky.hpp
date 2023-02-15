#pragma once

#include <string_view>
#include "vuk/Future.hpp"
#include "gfx/resource.hpp"
#include "gfx/renderer.hpp"
#include "gfx/camera.hpp"
#include "util/math.hpp"

namespace minote {

// Precalculated representation of a planet's atmosphere. Once created, it can
// be used repeatedly to sample the sky at any elevation and sun position
struct Atmosphere {
	
	constexpr static auto TransmittanceFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto TransmittanceSize = uint2{256u, 64u};
	
	constexpr static auto MultiScatteringFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MultiScatteringSize = uint2{32u, 32u};
	
	struct Params {
	
		float bottomRadius; // Radius of the planet (center to ground)
		float topRadius; // Maximum considered atmosphere height (center to atmosphere top)
		
		float rayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
		float _pad0;
		float3 rayleighScattering; // Rayleigh scattering coefficients
		
		float mieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
		float3 mieScattering; // Mie scattering coefficients
		float _pad1;
		float3 mieExtinction; // Mie extinction coefficients
		float _pad2;
		float3 mieAbsorption; // Mie absorption coefficients
		float miePhaseG; // Mie phase function excentricity
		
		// Another medium type in the atmosphere
		float absorptionDensity0LayerWidth;
		float absorptionDensity0ConstantTerm;
		float absorptionDensity0LinearTerm;
		float absorptionDensity1ConstantTerm;
		float absorptionDensity1LinearTerm;
		float _pad3;
		float _pad4;
		float _pad5;
		float3 absorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
		float _pad6;
		
		float3 groundAlbedo;
		
		// Return params that model Earth's atmosphere
		static auto earth() -> Params;
		
	};
	
	Texture2D<float4> transmittance;
	Texture2D<float4> multiScattering;
	Buffer<Params> params;
	
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
	constexpr static auto ViewSize = uint2{192u, 108u};
	
	constexpr static auto AerialPerspectiveFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto AerialPerspectiveSize = uint3{32u, 32u, 32u};
	
	float3 sunDirection = {-0.435286462, 0.818654716, 0.374606609};
	float3 sunIlluminance = {8.0f, 8.0f, 8.0f};
	
	// Create a 360-degree view of the sky at the specified world position
	auto createView(Atmosphere&, float3 probePos) -> Texture2D<float3>;
	
	// Draw the sky into a texture at camera position
	auto draw(Texture2D<float4> target, Atmosphere&, Texture2D<float3> skyView, Camera const&) -> Texture2D<float4>;
	
	// Build required shaders; optional
	static void compile();
	
	// Draw debug controls for this instance
	void drawImguiDebug(std::string_view name);
	
	// Compute direction from pitch and yaw
	auto getSunDirection(float pitch, float yaw) -> float3;
	
private:
	
	static inline bool m_compiled = false;
	
	// Imgui controls
	float m_sunPitch = 22_deg;
	float m_sunYaw = 118_deg;
	/*
	static auto createAerialPerspective(Pool&, Frame&, vuk::Name,
		float3 probePos, float4x4 invViewProj, Atmosphere) -> Texture3D;
	
	static auto createSunLuminance(Pool&, Frame&, vuk::Name,
		float3 probePos, Atmosphere) -> Buffer<float3>;
	
	// Draw the sky into an existing cubemap. Target is the mip 0 of provided image.
	static void draw(Frame&, Cubemap target, float3 probePos, Texture2D skyView, Atmosphere);
	*/
};

}
