#include "gfx/effects/quadBuffer.hpp"

#include "base/containers/array.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void QuadBuffer::compile(vuk::PerThreadContext& _ptc) {
	
	;
	
}

auto QuadBuffer::create(vuk::RenderGraph& _rg, Pool& _pool,
	vuk::Name _name, uvec2 _size, bool _flushTemporal) -> QuadBuffer {
	
	auto result = QuadBuffer();
	
	auto oddFrame = _pool.ptc().ctx.frame_counter.load() % 2;
	
	auto clusterDefs = to_array({
		Texture2D::make(_pool, nameAppend(_name, "clusterDef0"),
			_size, vuk::Format::eR32G32Uint,
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferDst),
		Texture2D::make(_pool, nameAppend(_name, "clusterDef1"),
			_size, vuk::Format::eR32G32Uint,
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferDst) });
	result.clusterDef = clusterDefs[oddFrame];
	result.clusterDefPrev = clusterDefs[!oddFrame];
	
	auto jitterMaps = to_array({
		Texture2D::make(_pool, nameAppend(_name, "jitterMap0"),
			divRoundUp(_size, 8u), vuk::Format::eR16Uint,
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eTransferDst),
		Texture2D::make(_pool, nameAppend(_name, "jitterMap1"),
			divRoundUp(_size, 8u), vuk::Format::eR16Uint,
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eTransferDst) });
	result.jitterMap = jitterMaps[oddFrame];
	result.jitterMapPrev = jitterMaps[!oddFrame];
	
	auto clusterOuts = to_array({
		Texture2D::make(_pool, nameAppend(_name, "clusterOut0"),
			_size, vuk::Format::eR16G16B16A16Sfloat,
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eTransferDst),
		Texture2D::make(_pool, nameAppend(_name, "clusterOut1"),
			_size, vuk::Format::eR16G16B16A16Sfloat,
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eTransferDst) });
	result.clusterOut = clusterOuts[oddFrame];
	result.clusterOutPrev = clusterOuts[!oddFrame];
	
	auto outputs = to_array({
		Texture2D::make(_pool, nameAppend(_name, "output0"),
			_size, vuk::Format::eR16G16B16A16Sfloat,
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eTransferSrc |
			vuk::ImageUsageFlagBits::eTransferDst),
		Texture2D::make(_pool, nameAppend(_name, "output1"),
			_size, vuk::Format::eR16G16B16A16Sfloat,
			vuk::ImageUsageFlagBits::eSampled |
			vuk::ImageUsageFlagBits::eStorage |
			vuk::ImageUsageFlagBits::eTransferSrc |
			vuk::ImageUsageFlagBits::eTransferDst) });
	result.output = outputs[oddFrame];
	result.outputPrev = outputs[!oddFrame];
	
	result.velocity = Texture2D::make(_pool, nameAppend(_name, "velocity"),
		_size, vuk::Format::eR16G16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled |
		vuk::ImageUsageFlagBits::eTransferDst);
	
	result.clusterDef.attach(_rg, vuk::eNone, vuk::eComputeRead);
	result.jitterMap.attach(_rg, vuk::eNone, vuk::eComputeRead);
	result.clusterOut.attach(_rg, vuk::eNone, vuk::eComputeRead);
	result.output.attach(_rg, vuk::eNone, vuk::eTransferSrc);
	if (_flushTemporal) {
		
		result.clusterDefPrev.attach(_rg, vuk::eNone, vuk::eNone);
		result.jitterMapPrev.attach(_rg, vuk::eNone, vuk::eNone);
		result.clusterOutPrev.attach(_rg, vuk::eNone, vuk::eNone);
		result.outputPrev.attach(_rg, vuk::eNone, vuk::eNone);
		
	} else {
		
		result.clusterDefPrev.attach(_rg, vuk::eComputeRead, vuk::eNone);
		result.jitterMapPrev.attach(_rg, vuk::eComputeRead, vuk::eNone);
		result.clusterOutPrev.attach(_rg, vuk::eComputeRead, vuk::eNone);
		result.outputPrev.attach(_rg, vuk::eTransferSrc, vuk::eNone);
		
	}
	
	result.velocity.attach(_rg, vuk::eNone, vuk::eNone);
	
	return result;
	
}

}
