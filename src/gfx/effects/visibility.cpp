#include "gfx/effects/visibility.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/containers/string.hpp"
#include "base/util.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

void Visibility::compile(vuk::PerThreadContext& _ptc) {
	
	auto visibilityPci = vuk::PipelineBaseCreateInfo();
	visibilityPci.add_spirv(std::vector<u32>{
#include "spv/visibility.vert.spv"
	}, "visibility.vert");
	visibilityPci.add_spirv(std::vector<u32>{
#include "spv/visibility.frag.spv"
	}, "visibility.frag");
	visibilityPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	visibilityPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eGreater;
	_ptc.ctx.create_named_pipeline("visibility", visibilityPci);
	
	auto visibilityMSPci = vuk::PipelineBaseCreateInfo();
	visibilityMSPci.add_spirv(std::vector<u32>{
#include "spv/visibility.vert.spv"
	}, "visibility.vert");
	visibilityMSPci.add_spirv(std::vector<u32>{
#include "spv/visibility.frag.spv"
	}, "visibility.frag");
	visibilityMSPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	visibilityMSPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eGreater;
	_ptc.ctx.create_named_pipeline("visibility_ms", visibilityMSPci);
	
}

void Visibility::apply(Frame& _frame, Texture2DMS _visbuf, Texture2DMS _depth,
	DrawableInstanceList _instances) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_visbuf.name, "visibility_ms"),
		.resources = {
			_instances.commands.resource(vuk::eIndirectRead),
			_instances.instances.resource(vuk::eVertexRead),
			_instances.transforms.resource(vuk::eVertexRead),
			_visbuf.resource(vuk::eColorWrite),
			_depth.resource(vuk::eDepthStencilRW) },
		.execute = [_visbuf, _instances, &_frame](vuk::CommandBuffer& cmd) {
			
			cmdSetViewportScissor(cmd, _visbuf.size());
			cmd.bind_index_buffer(_frame.models.indices, vuk::IndexType::eUint32)
			   .bind_uniform_buffer(0, 0, _frame.world)
			   .bind_storage_buffer(0, 1, _frame.models.vertices)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_graphics_pipeline("visibility_ms");
			
			cmd.draw_indexed_indirect(_instances.commands.length(), _instances.commands);
			
		}});
	
}

void Worklist::compile(vuk::PerThreadContext& _ptc) {
	
	auto worklistPci = vuk::ComputePipelineBaseCreateInfo();
	worklistPci.add_spirv(std::vector<u32>{
#include "spv/worklist.comp.spv"
	}, "worklist.comp");
	_ptc.ctx.create_named_pipeline("worklist", worklistPci);
	
	auto worklistMSPci = vuk::ComputePipelineBaseCreateInfo();
	worklistMSPci.add_spirv(std::vector<u32>{
#include "spv/worklistMS.comp.spv"
	}, "worklistMS.comp");
	_ptc.ctx.create_named_pipeline("worklist_ms", worklistMSPci);
	
}

auto Worklist::create(Pool& _pool, Frame& _frame, vuk::Name _name,
	Texture2D _visbuf, DrawableInstanceList _instances) -> Worklist {
	
	auto result = Worklist();
	
	result.tileDimensions = uvec2{
		u32(ceil(f32(_visbuf.size().x()) / f32(TileSize.x()))),
		u32(ceil(f32(_visbuf.size().y()) / f32(TileSize.y()))) };
	
	// Create buffers
	
	constexpr auto InitialCount = uvec4{0, 1, 1, 0};
	auto initialCounts = array<uvec4, ListCount>();
	initialCounts.fill(InitialCount);
	
	result.counts = Buffer<uvec4>::make(_frame.framePool, nameAppend(_name, "counts"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		initialCounts);
	
	result.lists = Buffer<u32>::make(_pool, nameAppend(_name, "tiles"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		usize(result.tileDimensions.x() * result.tileDimensions.y()) * ListCount);
	
	result.counts.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.lists.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	// Generate worklists
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "gen"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_instances.instances.resource(vuk::eComputeRead),
			result.counts.resource(vuk::eComputeRW),
			result.lists.resource(vuk::eComputeWrite) },
		.execute = [result, _visbuf, _instances, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _visbuf, NearestClamp)
			   .bind_storage_buffer(0, 1, _instances.instances)
			   .bind_storage_buffer(0, 2, _frame.models.meshes)
			   .bind_storage_buffer(0, 3, _frame.models.materials)
			   .bind_storage_buffer(0, 4, result.counts)
			   .bind_storage_buffer(0, 5, result.lists)
			   .bind_compute_pipeline("worklist");
			
			cmd.specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32Fromu16(_visbuf.size()));
			cmd.specialization_constants(1, vuk::ShaderStageFlagBits::eCompute, ListCount);
			
			cmd.dispatch_invocations(_visbuf.size().x(), _visbuf.size().y());
			
		}});
	
	return result;
	
}

}
