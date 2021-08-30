#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "base/math.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/resources/buffer.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

// Precalculated representation of a planet's atmosphere. Once created, it can
// be used repeatedly to sample the sky at any elevation and sun position.
struct Atmosphere {
	
	static constexpr auto Transmittance_n = "atmosphere_transmittance";
	static constexpr auto MultiScattering_n = "atmosphere_multiscattering";
	
	constexpr static auto TransmittanceFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto TransmittanceWidth = 256u;
	constexpr static auto TransmittanceHeight = 64u;
	
	constexpr static auto MultiScatteringFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MultiScatteringWidth = 32u;
	constexpr static auto MultiScatteringHeight = 32u;
	
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
	
	vuk::Texture transmittance;
	vuk::Texture multiScattering;
	Buffer<Params> params;
	
	// Build required shaders.
	static void compile(vuk::PerThreadContext&);
	
	// Construct the object. You may call upload() when ready.
	Atmosphere() = default;
	
	// Upload the atmospheric buffers to GPU as per provided parameters. Object
	// is not usable until all GPU transfers are finished.
	void upload(Pool&, vuk::Name, Params const&);
	
	// Fill the lookup tables. Requires upload() to be called first. Only needs
	// to be executed once.
	auto precalculate() -> vuk::RenderGraph;
	
};

// Effect for rendering sky backgrounds, IBL cubemaps and other position-dependent
// lookup tables.
struct Sky {
	
	static constexpr auto CameraView_n = "sky_camera_view";
	static constexpr auto CubemapView_n = "sky_cubemap_view";
	static constexpr auto AerialPerspective_n = "sky_aerial_perspective";
	static constexpr auto SunLuminance_n = "sky_sun_luminance";
	
	constexpr static auto ViewFormat = vuk::Format::eB10G11R11UfloatPack32;
	constexpr static auto ViewWidth = 192u;
	constexpr static auto ViewHeight = 108u;
	
	constexpr static auto AerialPerspectiveFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto AerialPerspectiveWidth = 32u;
	constexpr static auto AerialPerspectiveHeight = 32u;
	constexpr static auto AerialPerspectiveDepth = 32u;
	
	// World-space center of the camera for drawCubemap()
	constexpr static auto CubemapCamera = vec3{0_m, 0_m, 10_m};
	
	vuk::Texture cameraView;
	vuk::Texture cubemapView;
	vuk::Texture aerialPerspective;
	vuk::Unique<vuk::Buffer> sunLuminance;
	
	// Build the shader.
	static void compile(vuk::PerThreadContext&);
	
	Sky(vuk::PerThreadContext&, Atmosphere const&);
	
	// Fill lookup tables required for the two functions below.
	auto calculate(Buffer<World> const&, Camera const& camera) -> vuk::RenderGraph;
	
	// Draw the sky in the background of an image (where depth is 0.0).
	auto draw(Buffer<World> const&, vuk::Name targetColor,
		vuk::Name targetDepth, uvec2 targetSize) -> vuk::RenderGraph;
	
	// Draw the sky into an existing IBLMap. Target is the mip 0 of provided image.
	auto drawCubemap(Buffer<World> const&, Cubemap& dst) -> vuk::RenderGraph;
	
private:
	
	Atmosphere const& atmosphere;
	
};

}
