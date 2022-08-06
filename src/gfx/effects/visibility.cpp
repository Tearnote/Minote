#include "gfx/effects/visibility.hpp"

#include "vuk/CommandBuffer.hpp"
#include "util/string.hpp"
#include "util/util.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote {

void Visibility::compile(vuk::PerThreadContext& _ptc) {
	
	auto visibilityPci = vuk::PipelineBaseCreateInfo();
	visibilityPci.add_spirv(std::vector<uint>{
#include "spv/visibility/visbuf.vert.spv"
	}, "visibility/visbuf.vert");
	visibilityPci.add_spirv(std::vector<uint>{
#include "spv/visibility/visbuf.frag.spv"
	}, "visibility/visbuf.frag");
	_ptc.ctx.create_named_pipeline("visibility/visbuf", visibilityPci);
	
}

void Visibility::apply(Frame& _frame, Texture2DMS _visbuf, Texture2DMS _depth,
	TriangleList _triangles) {
	
	_frame.rg.add_pass({
		.name = nameAppend(_visbuf.name, "visibility/visbuf"),
		.resources = {
			_triangles.command.resource(vuk::eIndirectRead),
			_triangles.indices.resource(vuk::eIndexRead),
			_triangles.instances.resource(vuk::eVertexRead),
			_triangles.transforms.resource(vuk::eVertexRead),
			_visbuf.resource(vuk::eColorWrite),
			_depth.resource(vuk::eDepthStencilRW) },
		.execute = [_visbuf, _triangles, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.set_viewport(0, vuk::Rect2D::framebuffer());
			cmd.set_scissor(0, vuk::Rect2D::framebuffer());
			cmd.set_color_blend(_visbuf.name, vuk::BlendPreset::eOff);
			cmd.set_rasterization(vuk::PipelineRasterizationStateCreateInfo{
				// .cullMode = vuk::CullModeFlagBits::eNone });
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
			   .bind_storage_buffer(0, 4, _triangles.instances)
			   .bind_storage_buffer(0, 5, _triangles.transforms)
			   .bind_graphics_pipeline("visibility/visbuf");
			
			cmd.draw_indexed_indirect(1, _triangles.command);
			
		}});
	
}

void Worklist::compile(vuk::PerThreadContext& _ptc) {
	
	auto worklistPci = vuk::ComputePipelineBaseCreateInfo();
	worklistPci.add_spirv(std::vector<uint>{
#include "spv/visibility/worklist.comp.spv"
	}, "visibility/worklist.comp");
	_ptc.ctx.create_named_pipeline("visibility/worklist", worklistPci);
	
}

auto Worklist::create(Pool& _pool, Frame& _frame, vuk::Name _name,
	Texture2D _visbuf, TriangleList _triangles) -> Worklist {
	
	auto result = Worklist();
	
	result.tileDimensions = uint2{
		uint(ceil(float(_visbuf.size().x()) / float(TileSize.x()))),
		uint(ceil(float(_visbuf.size().y()) / float(TileSize.y()))) };
	
	// Create buffers
	
	constexpr auto InitialCount = uint4{0, 1, 1, 0};
	auto initialCounts = array<uint4, ListCount>();
	initialCounts.fill(InitialCount);
	
	result.counts = Buffer<uint4>::make(_frame.framePool, nameAppend(_name, "counts"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		initialCounts);
	
	result.lists = Buffer<uint>::make(_pool, nameAppend(_name, "tiles"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		usize(result.tileDimensions.x() * result.tileDimensions.y()) * ListCount);
	
	result.counts.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.lists.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	// Generate worklists
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "visibility/worklist"),
		.resources = {
			_visbuf.resource(vuk::eComputeSampled),
			_triangles.instances.resource(vuk::eComputeRead),
			result.counts.resource(vuk::eComputeRW),
			result.lists.resource(vuk::eComputeWrite) },
		.execute = [result, _visbuf, _triangles, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_sampled_image(0, 0, _visbuf, NearestClamp)
			   .bind_storage_buffer(0, 1, _triangles.indices)
			   .bind_storage_buffer(0, 2, _triangles.instances)
			   .bind_storage_buffer(0, 3, _frame.models.meshlets)
			   .bind_storage_buffer(0, 4, _frame.models.materials)
			   .bind_storage_buffer(0, 5, result.counts)
			   .bind_storage_buffer(0, 6, result.lists)
			   .bind_compute_pipeline("visibility/worklist");
			
			cmd.specialize_constants(0, uintFromuint16(_visbuf.size()));
			cmd.specialize_constants(1, ListCount);
			
			cmd.dispatch_invocations(_visbuf.size().x(), _visbuf.size().y());
			
		}});
	
	return result;
	
}

}
