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
#include <variant>

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

void Frame::draw(Texture2D _target, ObjectPool& _objects, AntialiasingType _aa, bool _flush) {
	
	// Upload resources
	
	auto world = cpu_world.upload(framePool, "world");
	auto basicInstances = BasicInstanceList::upload(framePool, *this, "basicInstances", _objects);
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
	constexpr auto IblProbePosition = vec3{0_m, 0_m, 10_m};
	
	auto color = Texture2D::make(swapchainPool, "color",
		viewport, vuk::Format::eR16G16B16A16Sfloat,
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eTransferDst);
	color.attach(rg, vuk::eNone, vuk::eNone);
	
	auto v_visbuf = std::variant<std::monostate, Texture2D, Texture2DMS>();
	auto v_depth = std::variant<std::monostate, Texture2D, Texture2DMS>();
	auto quadbuf = QuadBuffer();
	if (_aa == AntialiasingType::None) {
		
		v_visbuf = Texture2D::make(swapchainPool, "visbuf",
			viewport, vuk::Format::eR32Uint,
			vuk::ImageUsageFlagBits::eColorAttachment |
			vuk::ImageUsageFlagBits::eSampled);
		std::get<Texture2D>(v_visbuf).attach(rg, vuk::eClear, vuk::eNone, vuk::ClearColor(-1u, -1u, -1u, -1u));
		
		v_depth = Texture2D::make(swapchainPool, "depth",
			viewport, vuk::Format::eD32Sfloat,
			vuk::ImageUsageFlagBits::eDepthStencilAttachment |
			vuk::ImageUsageFlagBits::eSampled);
		std::get<Texture2D>(v_depth).attach(rg, vuk::eClear, vuk::eNone, vuk::ClearDepthStencil(0.0f, 0));
		
	} else {
		
		v_visbuf = Texture2DMS::make(swapchainPool, "visbufMS",
			viewport, vuk::Format::eR32Uint,
			vuk::ImageUsageFlagBits::eColorAttachment |
			vuk::ImageUsageFlagBits::eSampled,
			vuk::Samples::e8);
		std::get<Texture2DMS>(v_visbuf).attach(rg, vuk::eClear, vuk::eNone, vuk::ClearColor(-1u, -1u, -1u, -1u));
		
		v_depth = Texture2DMS::make(swapchainPool, "depthMS",
			viewport, vuk::Format::eD32Sfloat,
			vuk::ImageUsageFlagBits::eDepthStencilAttachment |
			vuk::ImageUsageFlagBits::eSampled,
			vuk::Samples::e8);
		std::get<Texture2DMS>(v_depth).attach(rg, vuk::eClear, vuk::eNone, vuk::ClearDepthStencil(0.0f, 0));
		
		quadbuf = QuadBuffer::create(rg, swapchainPool, "quadbuf", viewport, _flush);
		
	}
	
	// Create rendering passes
	
	// Instance list processing
	auto instances = InstanceList::fromBasic(framePool, rg, "instances", std::move(basicInstances), models);
	auto drawables = DrawableInstanceList::fromUnsorted(framePool, rg, "drawables", instances, models);
	auto culledDrawables = DrawableInstanceList::frustumCull(framePool, rg, "culledDrawables", drawables,
		models, cpu_world.view, cpu_world.projection);
	
	// Sky generation
	auto cameraSky = Sky::createView(permPool, rg, "cameraSky", cpu_world.cameraPos, atmosphere, world);
	auto cubeSky = Sky::createView(permPool, rg, "cubeSky", IblProbePosition, atmosphere, world);
	auto aerialPerspective = Sky::createAerialPerspective(permPool, rg, "aerialPerspective",
		cpu_world.cameraPos, cpu_world.viewProjectionInverse, atmosphere, world);
	auto sunLuminance = Sky::createSunLuminance(permPool, rg, "sunLuminance", cpu_world.cameraPos, atmosphere, world);
	
	// IBL generation
	Sky::draw(rg, iblUnfiltered, IblProbePosition, cubeSky, atmosphere, world);
	CubeFilter::apply(rg, iblUnfiltered, iblFiltered);
	
	// Scene drawing
	if (_aa == AntialiasingType::None) {
		
		auto visbuf = std::get<Texture2D>(v_visbuf);
		auto depth = std::get<Texture2D>(v_depth);
		
		Clear::apply(rg, color, vuk::ClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		Visibility::apply(rg, visbuf, depth, world, culledDrawables, models);
		auto worklist = Worklist::create(framePool, rg, "worklist", visbuf,
			culledDrawables, models);
		PBR::apply(rg, color, visbuf, worklist, world, models,
			culledDrawables, iblFiltered, sunLuminance, aerialPerspective);
		Sky::draw(rg, color, worklist, cameraSky, atmosphere, world);
		
	} else {
		
		auto visbuf = std::get<Texture2DMS>(v_visbuf);
		auto depth = std::get<Texture2DMS>(v_depth);
		
		Visibility::applyMS(rg, visbuf, depth, world, culledDrawables, models);
		QuadBuffer::clusterize(rg, quadbuf, visbuf, world);
		QuadBuffer::genBuffers(rg, quadbuf, models, culledDrawables, world);
		auto worklistMS = Worklist::create(framePool, rg, "worklist_ms", quadbuf.clusterDef,
			culledDrawables, models);
		PBR::applyQuad(rg, quadbuf, worklistMS, world, models,
			culledDrawables, iblFiltered, sunLuminance, aerialPerspective);
		Sky::drawQuad(rg, quadbuf, worklistMS, cameraSky, atmosphere, world);
		QuadBuffer::resolve(rg, quadbuf, color, world);
		
	}
	
	// Postprocessing
	Bloom::apply(rg, swapchainPool, color);
	Tonemap::apply(rg, color, _target);
	
}

}
