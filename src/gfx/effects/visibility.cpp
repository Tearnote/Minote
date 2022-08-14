#include "gfx/effects/visibility.hpp"

#include "vuk/AllocatorHelpers.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Partials.hpp"
#include "gfx/shader.hpp"
#include "gfx/util.hpp"
#include "sys/vulkan.hpp"

namespace minote {

Visibility::Visibility(vuk::Allocator& _allocator, ModelBuffer& _models,
	ObjectBuffer& _objects, InstanceList& _instances, TriangleList& _triangles,
	uint2 _extent, float4x4 _viewProjection) {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("visibility");
	rg->attach_in("indices", _triangles.indices);
	rg->attach_in("vertIndices", _models.vertIndices);
	rg->attach_in("vertices", _models.vertices);
	rg->attach_in("meshlets", _models.meshlets);
	rg->attach_in("transforms", _objects.transforms);
	rg->attach_in("instances", _instances.instances);
	rg->attach_in("command", _triangles.command);
	rg->attach_and_clear_image("visibility", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(_extent.x(), _extent.y()),
		.format = vuk::Format::eR32Uint,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	}, vuk::ClearColor(-1u, -1u, -1u, -1u));
	rg->attach_and_clear_image("depth", vuk::ImageAttachment{
		.format = vuk::Format::eD32Sfloat,
		.level_count = 1,
		.layer_count = 1,
	}, vuk::ClearDepth(0.0f));
	
	rg->add_pass(vuk::Pass{
		.name = "visibility/draw",
		.resources = {
			"indices"_buffer >> vuk::eIndexRead,
			"vertIndices"_buffer >> vuk::eVertexRead,
			"vertices"_buffer >> vuk::eVertexRead,
			"meshlets"_buffer >> vuk::eVertexRead,
			"transforms"_buffer >> vuk::eVertexRead,
			"instances"_buffer >> vuk::eVertexRead,
			"command"_buffer >> vuk::eIndirectRead,
			"visibility"_image >> vuk::eColorWrite >> "visibility/final",
			"depth"_image >> vuk::eDepthStencilRW >> "depth/final",
		},
		.execute = [_viewProjection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_graphics_pipeline("visibility/draw")
			   .set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .broadcast_color_blend(vuk::BlendPreset::eOff)
			   .set_rasterization(vuk::PipelineRasterizationStateCreateInfo{
				   .cullMode = vuk::CullModeFlagBits::eBack,
			   })
			   .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo{
				   .depthTestEnable = true,
				   .depthWriteEnable = true,
				   .depthCompareOp = vuk::CompareOp::eGreater,
			   })
			   .bind_index_buffer(*cmd.get_resource_buffer("indices"), vuk::IndexType::eUint32)
			   .bind_buffer(0, 0, "vertIndices")
			   .bind_buffer(0, 1, "vertices")
			   .bind_buffer(0, 2, "meshlets")
			   .bind_buffer(0, 3, "transforms")
			   .bind_buffer(0, 4, "instances");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, _viewProjection);
			
			cmd.draw_indexed_indirect(1, *cmd.get_resource_buffer("command"));
			
		}});
	
	visibility = vuk::Future(rg, "visibility/final");
	depth = vuk::Future(rg, "depth/final");
	
}

GET_SHADER(visibility_draw_vs);
GET_SHADER(visibility_draw_ps);
void Visibility::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto drawPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(drawPci, visibility_draw_vs, "visibility/draw.vs.hlsl");
	ADD_SHADER(drawPci, visibility_draw_ps, "visibility/draw.ps.hlsl");
	ctx.create_named_pipeline("visibility/draw", drawPci);
	
	m_compiled = true;
	
}

Worklist::Worklist(vuk::Allocator& _allocator, ModelBuffer& _models,
	InstanceList& _instances, TriangleList& _triangles, Visibility& _visibility, uint2 _extent) {
	
	compile();
	
	tileArea = uint2{
		divRoundUp(_extent.x(), TileSize),
		divRoundUp(_extent.y(), TileSize),
	};
	
	auto rg = std::make_shared<vuk::RenderGraph>("worklist");
	rg->attach_in("meshlets", _models.meshlets);
	rg->attach_in("materials", _models.materials);
	rg->attach_in("instances", _instances.instances);
	rg->attach_in("indices", _triangles.indices);
	rg->attach_in("visibility", _visibility.visibility);
	
	constexpr auto InitialCount = uint4{0, 1, 1, 0};
	auto initialCounts = array<uint4, ListCount>();
	initialCounts.fill(InitialCount);
	auto countsBuf = vuk::create_buffer_cross_device<uint4>(_allocator, vuk::MemoryUsage::eCPUtoGPU, initialCounts).second;
	rg->attach_in("counts", countsBuf);
	
	auto listsBuf = *vuk::allocate_buffer_gpu(_allocator, vuk::BufferCreateInfo{
		.mem_usage = vuk::MemoryUsage::eGPUonly,
		.size = tileArea.x() * tileArea.y() * ListCount * sizeof(uint4),
	});
	rg->attach_buffer("lists", *listsBuf);
	
	rg->add_pass(vuk::Pass{
		.name = "visibility/worklist",
		.resources = {
			"meshlets"_buffer >> vuk::eComputeRead,
			"materials"_buffer >> vuk::eComputeRead,
			"instances"_buffer >> vuk::eComputeRead,
			"indices"_buffer >> vuk::eComputeRead,
			"visibility"_image >> vuk::eComputeSampled,
			"counts"_buffer >> vuk::eComputeRW >> "counts/final",
			"lists"_buffer >> vuk::eComputeWrite >> "lists/final",
		},
		.execute = [this, _extent](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("visibility/worklist")
			   .bind_buffer(0, 0, "meshlets")
			   .bind_buffer(0, 1, "materials")
			   .bind_buffer(0, 2, "instances")
			   .bind_buffer(0, 3, "indices")
			   .bind_image(0, 4, "visibility")
			   .bind_buffer(0, 5, "counts")
			   .bind_buffer(0, 6, "lists");
			
			cmd.specialize_constants(0, _extent.x());
			cmd.specialize_constants(1, _extent.y());
			cmd.specialize_constants(2, tileArea.x());
			cmd.specialize_constants(3, tileArea.y());
			cmd.specialize_constants(4, ListCount);
			
			cmd.dispatch_invocations(_extent.x(), _extent.y());
			
		},
	});
	
	counts = vuk::Future(rg, "counts/final");
	lists = vuk::Future(rg, "lists/final");
	
}

GET_SHADER(visibility_worklist_cs);
void Worklist::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto worklistPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(worklistPci, visibility_worklist_cs, "visibility/worklist.cs.hlsl");
	ctx.create_named_pipeline("visibility/worklist", worklistPci);
	
	m_compiled = true;
	
}

}
