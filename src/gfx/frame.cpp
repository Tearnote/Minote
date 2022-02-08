#include "gfx/frame.hpp"

#include "gfx/effects/instanceList.hpp"
#include "gfx/effects/quadbuffer.hpp"
#include "gfx/effects/cubeFilter.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/effects/tonemap.hpp"
#include "gfx/effects/bloom.hpp"
#include "gfx/effects/clear.hpp"
#include "gfx/effects/pbr.hpp"
#include "gfx/effects/sky.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

Frame::Frame(Engine& _engine, vuk::RenderGraph& _rg):
	ptc(_engine.m_framePool.ptc()),
	rg(_rg),
	framePool(_engine.m_framePool),
	swapchainPool(_engine.m_swapchainPool),
	permPool(_engine.m_permPool),
	models(_engine.m_models),
	cpu_world(_engine.m_world) {}

void Frame::draw(Texture2D _target, ObjectPool& _objects, bool _flush) {
	
	// Upload resources
	
	world = cpu_world.upload(framePool, "world");
	auto instances = InstanceList::upload(framePool, *this, "instances", _objects);
	auto atmosphere = Atmosphere::create(permPool, *this, "earth", Atmosphere::Params::earth());
	auto viewport = _target.size();
	
	// Create textures
	
	auto iblUnfiltered = Cubemap::make(permPool, "iblUnfiltered",
		256, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferSrc);
	auto iblFiltered = Cubemap::make(permPool, "iblFiltered",
		256, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferDst);
	iblUnfiltered.attach(rg, vuk::eNone, vuk::eNone);
	iblFiltered.attach(rg, vuk::eNone, vuk::eNone);
	constexpr auto IblProbePosition = vec3{0_m, 0_m, 64_m};
	
	auto color = Texture2D::make(swapchainPool, "color",
		viewport, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eTransferDst);
	color.attach(rg, vuk::eNone, vuk::eNone);
	
	auto visbuf = Texture2DMS::make(swapchainPool, "visbuf",
		viewport, vuk::Format::eR32Uint,
		vuk::ImageUsageFlagBits::eColorAttachment |
		vuk::ImageUsageFlagBits::eSampled,
		vuk::Samples::e8);
	visbuf.attach(rg, vuk::eClear, vuk::eNone, vuk::ClearColor(-1u, -1u, -1u, -1u));
	
	auto depth = Texture2DMS::make(swapchainPool, "depth",
		viewport, vuk::Format::eD32Sfloat,
		vuk::ImageUsageFlagBits::eDepthStencilAttachment |
		vuk::ImageUsageFlagBits::eSampled,
		vuk::Samples::e8);
	depth.attach(rg, vuk::eClear, vuk::eNone, vuk::ClearDepthStencil(0.0f, 0));
	
	auto quadbuf = QuadBuffer::create(swapchainPool, *this, "quadbuf", viewport, _flush);
	
	// Create rendering passes
	
	// Instance list processing
	auto screenTriangles = TriangleList::fromInstances(instances, framePool, *this, "screenTriangles",
		cpu_world.view, cpu_world.projection);
	
	// Sky generation
	auto cameraSky = Sky::createView(permPool, *this, "cameraSky", cpu_world.cameraPos, atmosphere);
	auto cubeSky = Sky::createView(permPool, *this, "cubeSky", IblProbePosition, atmosphere);
	auto aerialPerspective = Sky::createAerialPerspective(permPool, *this, "aerialPerspective",
		cpu_world.cameraPos, cpu_world.viewProjectionInverse, atmosphere);
	auto sunLuminance = Sky::createSunLuminance(permPool, *this, "sunLuminance", cpu_world.cameraPos, atmosphere);
	
	// IBL generation
	Sky::draw(*this, iblUnfiltered, IblProbePosition, cubeSky, atmosphere);
	CubeFilter::apply(*this, iblUnfiltered, iblFiltered);
	
	// Drawing
	Visibility::apply(*this, visbuf, depth, screenTriangles);
	QuadBuffer::clusterize(*this, quadbuf, visbuf);
	QuadBuffer::genBuffers(*this, quadbuf, screenTriangles);
	auto worklist = Worklist::create(swapchainPool, *this, "worklist", quadbuf.visbuf, screenTriangles);
	PBR::apply(*this, quadbuf, worklist, screenTriangles,
		iblFiltered, sunLuminance, aerialPerspective);
	Sky::draw(*this, quadbuf, worklist, cameraSky, atmosphere);
	QuadBuffer::resolve(*this, quadbuf, color);
	
	// Postprocessing
	Bloom::apply(*this, swapchainPool, color);
	Tonemap::apply(*this, color, _target);
	
}

}
