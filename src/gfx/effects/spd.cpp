#include "gfx/effects/spd.hpp"

#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "gfx/samplers.hpp"
#include "gfx/shader.hpp"
#include "gfx/util.hpp"
#include "sys/vulkan.hpp"
#include "util/array.hpp"

namespace minote {

auto SPD::apply(Texture2D<float> _source, ReductionType _type) -> Texture2D<float> {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("spd");
	rg->attach_in("source", _source);
	
	rg->add_pass(vuk::Pass{
		.name = "spd/apply",
		.resources = {
			"source"_image >> vuk::eComputeRW >> "source/final",
			"source"_image >> vuk::eComputeSampled, // declare an extra usage
		},
		.execute = [_type](auto& cmd) {
			
			auto source = *cmd.get_resource_image_attachment("source");
			auto sourceSize = source.extent.extent;
			auto mipCount = source.level_count;
			ASSERT(mipCount <= 13);
			auto sourceMips = array<vuk::ImageAttachment, 13>();
			for (auto i: iota(0u, mipCount)) {
				auto mip = source;
				mip.base_level = i;
				mip.level_count = 1;
				sourceMips[i] = mip;
			}
			auto dispatchSize = uint2{
				((sourceSize.width + 63) >> 6),
				((sourceSize.height + 63) >> 6),
			};
			auto sampler = [_type](){
				switch (_type) {
					case ReductionType::Avg: return LinearClamp;
					case ReductionType::Min: return MinClamp;
					default /*Max*/: return MaxClamp;
				}
			}();
			
			// Prepare initial mip for read
			cmd.image_barrier("source", vuk::eComputeRW, vuk::eComputeSampled, 0, 1);
			
			cmd.bind_compute_pipeline("spd/apply")
			   .bind_image(0, 0, sourceMips[0], vuk::ImageLayout::eGeneral).bind_sampler(0, 0, sampler);
			*cmd.map_scratch_buffer<uint>(0, 1) = 0;
			for (auto i: iota(1u, 13u))
				cmd.bind_image(0, i+1, sourceMips[min(i, mipCount-1)], vuk::ImageLayout::eGeneral);
			
			cmd.specialize_constants(0, mipCount - 1);
			cmd.specialize_constants(1, dispatchSize.x() * dispatchSize.y());
			cmd.specialize_constants(2, sourceSize.width);
			cmd.specialize_constants(3, sourceSize.height);
			cmd.specialize_constants(4,
				sourceSize.width == sourceSize.height &&
				(sourceSize.width & (sourceSize.width - 1)) == 0 // Clever bitwise power-of-two check
				? 1u : 0u);
			cmd.specialize_constants(5, +_type);
			cmd.specialize_constants(6, vuk::is_format_srgb(source.format)? 1u : 0u);
			
			cmd.dispatch(dispatchSize.x(), dispatchSize.y());
			
			// Converge image to return to a consistent state
			cmd.image_barrier("source", vuk::eComputeSampled, vuk::eComputeRW, 0, 1);
			
		},
	});
	
	return vuk::Future(rg, "source/final");
	
}

GET_SHADER(spd_cs);
void SPD::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto applyPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(applyPci, spd_cs, "spd.cs.hlsl");
	ctx.create_named_pipeline("spd/apply", applyPci);
	
	m_compiled = true;
	
}

}
