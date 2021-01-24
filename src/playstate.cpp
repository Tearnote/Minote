#include "playstate.hpp"

#include "base/util.hpp"
#include "base/math.hpp"

namespace minote {

using namespace base;

PlayState::PlayState() {
	;
}

void PlayState::tick(std::span<Action const>) {
	;
}

void PlayState::draw(gfx::Engine& engine) {
	// Scene
	auto const stackHeight = m_grid.stackHeight();

	engine.enqueueDraw("scene_base"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = glm::mat4{1.0f},
			.tint = glm::vec4{1.2f, 1.2f, 1.2f, 1.0f},
		},
	}, gfx::Material::Flat);
	engine.enqueueDraw("scene_body"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = make_translate({0.0f, -0.1f, 0.0f}) * make_scale({1.0f, 0.1f + stackHeight, 1.0f}),
			.tint = glm::vec4{1.2f, 1.2f, 1.2f, 1.0f},
		},
	}, gfx::Material::Flat);
	engine.enqueueDraw("scene_top"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = make_translate({0.0f, stackHeight, 0.0f}) * make_scale({1.0f, 4.1f, 1.0f}),
		},
	}, gfx::Material::Flat);
	engine.enqueueDraw("scene_guide"_id, "transparent"_id, std::array{
		gfx::Instance{
			.transform = make_translate({0.0f, -0.1f, 0.0f}) * make_scale({1.0f, 4.1f + stackHeight, 1.0f}),
		},
	}, gfx::Material::Flat);

	// Blocks
	svector<gfx::Instance, 512> blockInstances;
	for (auto x: nrange(0_zu, m_grid.Width))
		for (auto y: nrange(0_zu, m_grid.Height)) {
			auto const minoOpt = m_grid.get({x, y});
			if (!minoOpt) continue;
			auto const mino = minoOpt.value();

			auto const translation = make_translate({
				static_cast<f32>(x) - m_grid.Width / 2,
				y,
				-1.0f,
			});
			blockInstances.emplace_back(gfx::Instance{
				.transform = translation,
				.tint = minoColor(mino),
			});
		}
	engine.enqueueDraw("block"_id, "opaque"_id, blockInstances,
		gfx::Material::Phong, gfx::MaterialData{.phong = {
			.ambient = 0.2f,
			.diffuse = 0.9f,
			.specular = 0.4f,
			.shine = 24.0f,
		}}
	);
}

}
