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
#include "spv/visibility/visbuf.vert.spv"
	}, "visibility/visbuf.vert");
	visibilityPci.add_spirv(std::vector<u32>{
#include "spv/visibility/visbuf.frag.spv"
	}, "visibility/visbuf.frag");
	_ptc.ctx.create_named_pipeline("visibility/visbuf", visibilityPci);
	
}

void Visibility::apply(Frame& _frame, Texture2DMS _visbuf, Texture2DMS _depth,
	InstanceList _instances, TriangleList _triangles) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_visbuf.name, "visibility/visbuf"),
		.resources = {
			_triangles.command.resource(vuk::eIndirectRead),
			_triangles.indices.resource(vuk::eIndexRead),
			_instances.instances.resource(vuk::eVertexRead),
			_instances.transforms.resource(vuk::eVertexRead),
			_visbuf.resource(vuk::eColorWrite),
			_depth.resource(vuk::eDepthStencilRW) },
		.execute = [_visbuf, _instances, _triangles, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.set_viewport(0, vuk::Rect2D::framebuffer());
			cmd.set_scissor(0, vuk::Rect2D::framebuffer());
			cmd.set_color_blend(_visbuf.name, vuk::BlendPreset::eOff);
			cmd.set_rasterization(vuk::PipelineRasterizationStateCreateInfo{
				.cullMode = vuk::CullModeFlagBits::eBack });
			cmd.set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo{
				.depthTestEnable = true,
				.depthWriteEnable = true,
				.depthCompareOp = vuk::CompareOp::eGreater });
			
			cmd.bind_index_buffer(_triangles.indices, vuk::IndexType::eUint32)
			   .bind_uniform_buffer(0, 0, _frame.world)
			   .bind_storage_buffer(0, 1, _frame.models.vertIndices)
			   .bind_storage_buffer(0, 2, _frame.models.vertices)
			   .bind_storage_buffer(0, 3, _frame.models.meshlets)
			   .bind_storage_buffer(0, 4, _instances.instances)
			   .bind_storage_buffer(0, 5, _instances.transforms)
			   .bind_graphics_pipeline("visibility/visbuf");
			
			cmd.draw_indexed_indirect(1, _triangles.command);
			
		}});
	
}

void Worklist::compile(vuk::PerThreadContext& _ptc) {
	
	auto worklistPci = vuk::ComputePipelineBaseCreateInfo();
	worklistPci.add_spirv(std::vector<u32>{
#include "spv/visibility/worklist.comp.spv"
	}, "visibility/worklist.comp");
	_ptc.ctx.create_named_pipeline("visibility/worklist", worklistPci);
	
}

auto Worklist::create(Pool& _pool, Frame& _frame, vuk::Name _name,
	Texture2D _visbuf, InstanceList _instances) -> Worklist {
	
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
		.name = nameAppend(_name, "visibility/worklist"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_instances.instances.resource(vuk::eComputeRead),
			result.counts.resource(vuk::eComputeRW),
			result.lists.resource(vuk::eComputeWrite) },
		.execute = [result, _visbuf, _instances, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _visbuf, NearestClamp)
			   .bind_storage_buffer(0, 1, _instances.instances)
			   .bind_storage_buffer(0, 2, _frame.models.meshlets)
			   .bind_storage_buffer(0, 3, _frame.models.materials)
			   .bind_storage_buffer(0, 4, result.counts)
			   .bind_storage_buffer(0, 5, result.lists)
			   .bind_compute_pipeline("visibility/worklist");
			
			cmd.specialize_constants(0, u32Fromu16(_visbuf.size()));
			cmd.specialize_constants(1, ListCount);
			
			cmd.dispatch_invocations(_visbuf.size().x(), _visbuf.size().y());
			
		}});
	
	return result;
	
}

}
