#include "playstate.hpp"

#include <algorithm>

namespace minote {

namespace ranges = std::ranges;

PlayState::PlayState() {
	// something, someday...
}

void PlayState::tick(std::span<Action const> actions) {
	std::srand(0);
	for (auto x: ranges::iota_view{0_zu, m_grid.Width})
		for (auto y: ranges::iota_view{0_zu, m_grid.Height}) {
			if (std::rand() % 4)
				m_grid.set({x, y}, Mino{static_cast<int>(x + y) % 7});
		}
}

void PlayState::draw(gfx::Engine& engine) {
	// Scene
	engine.enqueueDraw("scene"_id, "transparent"_id,
		std::to_array<gfx::Instance>({
			{.transform = glm::mat4{1.0f}}
		}),
		gfx::Material::Flat);

	// Blocks
	svector<gfx::Instance, 512> blockInstancesOpaque;
	svector<gfx::Instance, 512> blockInstancesTransparent;
	for (auto x: ranges::iota_view{0_zu, m_grid.Width})
		for (auto y: ranges::iota_view{0_zu, m_grid.Height}) {
			auto const minoOpt = m_grid.get({x, y});
			if (!minoOpt) continue;
			auto const mino = minoOpt.value();

			auto const translation = make_translate({
				static_cast<f32>(x) - m_grid.Width / 2,
				y,
				-1.0f,
			});
			auto color = minoColor(mino);
			if (y >= 20) color.a *= 0.9f;
			auto& instances = color.a < 1.0f? blockInstancesTransparent : blockInstancesOpaque;
			instances.emplace_back(gfx::Instance{
				.transform = translation,
				.tint = color,
			});
		}
	engine.enqueueDraw("block"_id, "opaque"_id, blockInstancesOpaque,
		gfx::Material::Phong, gfx::MaterialData{.phong = {
			.ambient = 0.2f,
			.diffuse = 0.9f,
			.specular = 0.4f,
			.shine = 24.0f,
		}}
	);
	engine.enqueueDraw("block"_id, "transparent"_id, blockInstancesTransparent,
		gfx::Material::Phong, gfx::MaterialData{.phong = {
			.ambient = 0.2f,
			.diffuse = 0.9f,
			.specular = 0.4f,
			.shine = 24.0f,
		}}
	);
}

}
