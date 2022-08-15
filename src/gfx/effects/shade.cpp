#include "gfx/effects/shade.hpp"

#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "gfx/shader.hpp"
#include "sys/vulkan.hpp"

namespace minote {

auto Shade::flat(Worklist& _worklist, ModelBuffer& _models, InstanceList& _instances,
	Visibility& _visibility, TriangleList& _triangles, Texture2D<float4> _target)
	-> Texture2D<float4> {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("shadeFlat");
	rg->attach_in("materials", _models.materials);
	rg->attach_in("meshlets", _models.meshlets);
	rg->attach_in("instances", _instances.instances);
	rg->attach_in("indices", _triangles.indices);
	rg->attach_in("lists", _worklist.lists);
	rg->attach_in("visibility", _visibility.visibility);
	rg->attach_in("counts", _worklist.counts);
	rg->attach_in("target", _target);
	
	rg->add_pass(vuk::Pass{
		.name = "shade/flat",
		.resources = {
			"materials"_buffer >> vuk::eComputeRead,
			"meshlets"_buffer >> vuk::eComputeRead,
			"instances"_buffer >> vuk::eComputeRead,
			"indices"_buffer >> vuk::eComputeRead,
			"lists"_buffer >> vuk::eComputeRead,
			"visibility"_image >> vuk::eComputeSampled,
			"counts"_buffer >> vuk::eIndirectRead,
			"target"_image >> vuk::eComputeWrite >> "target/final",
		},
		.execute = [&_worklist](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("shade/flat")
			   .bind_buffer(0, 0, "materials")
			   .bind_buffer(0, 1, "meshlets")
			   .bind_buffer(0, 2, "instances")
			   .bind_buffer(0, 3, "indices")
			   .bind_buffer(0, 4, "lists")
			   .bind_image(0, 5, "visibility")
			   .bind_image(0, 6, "target");
			
			auto target = *cmd.get_resource_image_attachment("target");
			auto targetSize = target.extent.extent;
			cmd.specialize_constants(0, targetSize.width);
			cmd.specialize_constants(1, targetSize.height);
			cmd.specialize_constants(2, _worklist.listsOffset(Material::Type::Flat));
			
			cmd.dispatch_indirect(cmd.get_resource_buffer("counts")->add_offset(_worklist.countsOffset(Material::Type::Flat)));
			
		},
	});
	
	return vuk::Future(rg, "target/final");
	
}

GET_SHADER(shadeFlat_cs);
void Shade::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto flatPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(flatPci, shadeFlat_cs, "shadeFlat.cs.hlsl");
	ctx.create_named_pipeline("shade/flat", flatPci);
	
	m_compiled = true;
	
}

}
