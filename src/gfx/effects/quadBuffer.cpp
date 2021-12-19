#include "gfx/effects/quadBuffer.hpp"

#include "base/containers/array.hpp"
#include "gfx/effects/clear.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

void QuadBuffer::compile(vuk::PerThreadContext& _ptc) {
	
	auto quadClusterizePci = vuk::ComputePipelineBaseCreateInfo();
	quadClusterizePci.add_spirv(std::vector<u32>{
#include "spv/quadClusterize.comp.spv"
	}, "quadClusterize.comp");
	_ptc.ctx.create_named_pipeline("quadClusterize", quadClusterizePci);
	
	auto quadGenBuffersPci = vuk::ComputePipelineBaseCreateInfo();
	quadGenBuffersPci.add_spirv(std::vector<u32>{
#include "spv/quadGenBuffers.comp.spv"
	}, "quadGenBuffers.comp");
	_ptc.ctx.create_named_pipeline("quadGenBuffers", quadGenBuffersPci);
	
	auto quadResolvePci = vuk::ComputePipelineBaseCreateInfo();
	quadResolvePci.add_spirv(std::vector<u32>{
#include "spv/quadResolve.comp.spv"
	}, "quadResolve.comp");
	_ptc.ctx.create_named_pipeline("quadResolve", quadResolvePci);
	
}

auto QuadBuffer::create(Pool& _pool, Frame& _frame,
	vuk::Name _name, uvec2 _size, bool _flushTemporal) -> QuadBuffer {
	
	auto result = QuadBuffer();
	result.name = _name;
	
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
	
	result.offset = Texture2D::make(_pool, nameAppend(_name, "offset"),
		_size, vuk::Format::eR8G8Unorm,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	result.velocity = Texture2D::make(_pool, nameAppend(_name, "velocity"),
		_size, vuk::Format::eR16G16Sfloat,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	result.normal = Texture2D::make(_pool, nameAppend(_name, "normals"),
		_size, vuk::Format::eR32Uint,
		vuk::ImageUsageFlagBits::eStorage |
		vuk::ImageUsageFlagBits::eSampled);
	
	result.clusterDef.attach(_frame.rg, vuk::eNone, vuk::eComputeRead);
	result.jitterMap.attach(_frame.rg, vuk::eNone, vuk::eComputeRead);
	result.clusterOut.attach(_frame.rg, vuk::eNone, vuk::eComputeRead);
	result.output.attach(_frame.rg, vuk::eNone, vuk::eTransferSrc);
	if (_flushTemporal) {
		
		result.clusterDefPrev.attach(_frame.rg, vuk::eNone, vuk::eNone);
		result.jitterMapPrev.attach(_frame.rg, vuk::eNone, vuk::eNone);
		result.clusterOutPrev.attach(_frame.rg, vuk::eNone, vuk::eNone);
		result.outputPrev.attach(_frame.rg, vuk::eNone, vuk::eNone);
		
	} else {
		
		result.clusterDefPrev.attach(_frame.rg, vuk::eComputeRead, vuk::eNone);
		result.jitterMapPrev.attach(_frame.rg, vuk::eComputeRead, vuk::eNone);
		result.clusterOutPrev.attach(_frame.rg, vuk::eComputeRead, vuk::eNone);
		result.outputPrev.attach(_frame.rg, vuk::eTransferSrc, vuk::eNone);
		
	}
	
	result.offset.attach(_frame.rg, vuk::eNone, vuk::eNone);
	result.velocity.attach(_frame.rg, vuk::eNone, vuk::eNone);
	result.normal.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	Clear::apply(_frame, result.jitterMap, vuk::ClearColor(0u, 0u, 0u, 0u));
	if (_flushTemporal) {
		
		Clear::apply(_frame, result.clusterDefPrev, vuk::ClearColor(0u, 0u, 0u, 0u));
		Clear::apply(_frame, result.clusterOutPrev, vuk::ClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		Clear::apply(_frame, result.outputPrev, vuk::ClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		
	}
	
	return result;
	
}

void QuadBuffer::clusterize(Frame& _frame, QuadBuffer& _quadbuf, Texture2DMS _visbuf) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_quadbuf.name, "clusterize"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_quadbuf.clusterDef.resource(vuk::eComputeWrite),
			_quadbuf.jitterMap.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, _visbuf, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_sampled_image(0, 1, _visbuf, NearestClamp)
			   .bind_storage_image(0, 2, _quadbuf.clusterDef)
			   .bind_storage_image(0, 3, _quadbuf.jitterMap)
			   .bind_compute_pipeline("quadClusterize");
			
			auto invocationCount = _visbuf.size() / 2u + _visbuf.size() % 2u;
			cmd.dispatch_invocations(invocationCount.x(), invocationCount.y());
			
	}});
	
}

void QuadBuffer::genBuffers(Frame& _frame, QuadBuffer& _quadbuf, DrawableInstanceList _instances) {
	
	_frame.rg.add_pass({
		
		.name = nameAppend(_quadbuf.name, "genBuffers"),
		.resources = {
			_instances.instances.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			_quadbuf.clusterDef.resource(vuk::eComputeSampled),
			_quadbuf.offset.resource(vuk::eComputeWrite),
			_quadbuf.normal.resource(vuk::eComputeWrite),
			_quadbuf.velocity.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, &_frame, _instances](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_storage_buffer(0, 1, _frame.models.meshes)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_storage_buffer(0, 4, _frame.models.indices)
			   .bind_storage_buffer(0, 5, _frame.models.vertices)
			   .bind_storage_buffer(0, 6, _frame.models.normals)
			   .bind_sampled_image(0, 7, _quadbuf.clusterDef, NearestClamp)
			   .bind_storage_image(0, 8, _quadbuf.offset)
			   .bind_storage_image(0, 9, _quadbuf.normal)
			   .bind_storage_image(0, 10, _quadbuf.velocity)
			   .bind_compute_pipeline("quadGenBuffers");
			
			cmd.specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32Fromu16(_quadbuf.clusterDef.size()));
			
			cmd.dispatch_invocations(_quadbuf.clusterDef.size().x(), _quadbuf.clusterDef.size().y());
			
		}});
	
}

void QuadBuffer::resolve(Frame& _frame, QuadBuffer& _quadbuf, Texture2D _output) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_quadbuf.name, "resolve"),
		.resources = {
			_quadbuf.clusterDef.resource(vuk::eComputeSampled),
			_quadbuf.jitterMap.resource(vuk::eComputeSampled),
			_quadbuf.clusterOut.resource(vuk::eComputeSampled),
			_quadbuf.outputPrev.resource(vuk::eComputeSampled),
			_quadbuf.clusterDefPrev.resource(vuk::eComputeSampled),
			_quadbuf.clusterOutPrev.resource(vuk::eComputeSampled),
			_quadbuf.jitterMapPrev.resource(vuk::eComputeSampled),
			_quadbuf.velocity.resource(vuk::eComputeSampled),
			_quadbuf.output.resource(vuk::eComputeWrite) },
		.execute = [_quadbuf, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_sampled_image(0, 1, _quadbuf.clusterDef, NearestClamp)
			   .bind_sampled_image(0, 2, _quadbuf.jitterMap, NearestClamp)
			   .bind_sampled_image(0, 3, _quadbuf.clusterOut, NearestClamp)
			   .bind_sampled_image(0, 4, _quadbuf.outputPrev, LinearClamp)
			   .bind_sampled_image(0, 5, _quadbuf.clusterDefPrev, NearestClamp)
			   .bind_sampled_image(0, 6, _quadbuf.clusterOutPrev, NearestClamp)
			   .bind_sampled_image(0, 7, _quadbuf.jitterMapPrev, NearestClamp)
			   .bind_sampled_image(0, 8, _quadbuf.velocity, LinearClamp)
			   .bind_storage_image(0, 9, _quadbuf.output)
			   .specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32Fromu16(_quadbuf.output.size()))
			   .bind_compute_pipeline("quadResolve");
			
			auto invocationCount = _quadbuf.output.size() / 2u + _quadbuf.output.size() % 2u;
			cmd.dispatch_invocations(invocationCount.x(), invocationCount.y());
			
	}});
	
	_frame.rg.add_pass({
		.name = nameAppend(_quadbuf.name, "copy"),
		.resources = {
			_quadbuf.output.resource(vuk::eTransferSrc),
			_output.resource(vuk::eTransferDst) },
		.execute = [_quadbuf, _output](vuk::CommandBuffer& cmd) {
			
			cmd.blit_image(_quadbuf.output.name, _output.name, vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(_output.size().x()), i32(_output.size().y()), 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{i32(_output.size().x()), i32(_output.size().y()), 1}} },
				vuk::Filter::eNearest);
			
	}});
	
}

}
