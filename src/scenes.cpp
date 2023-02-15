#include "scenes.hpp"

#include "util/util.hpp"
#include "math.hpp"
#include "stx/ranges.hpp"
#include "stx/time.hpp"
#include "sys/system.hpp"
#include "gfx/renderer.hpp"

namespace minote {

using namespace util::id_literals;

BattleScene::BattleScene(Transform _t) {
	
	m_id = s_renderer->objects().create();
	auto testscene = s_renderer->objects().get(m_id);
	testscene.modelID = "testscene"_id;
	testscene.transform = _t;
	
}

BattleScene::~BattleScene() {
	
	s_renderer->objects().destroy(m_id);
	
}

SimpleScene::SimpleScene(Transform _t) {
	
	m_spinny = s_renderer->objects().create();
	auto spinny = s_renderer->objects().get(m_spinny);
	spinny.modelID = "block"_id;
	spinny.color = {0.2f, 0.9f, 0.5f, 1.0f};
	spinny.transform = _t;
	spinny.transform.position += float3{0.0f, 0.0f, 2.5f} * _t.scale;
	spinny.transform.scale *= float3{1.5f, 1.5f, 1.5f};
	
	m_blocks[0] = s_renderer->objects().create();
	auto block0 = s_renderer->objects().get(m_blocks[0]);
	block0.modelID = "block"_id;
	block0.color = {0.9f, 0.9f, 1.0f, 1.0f};
	block0.transform = _t;
	block0.transform.scale *= float3{12.0f, 12.0f, 1.0f};
	
	m_blocks[1] = s_renderer->objects().create();
	auto block1 = s_renderer->objects().get(m_blocks[1]);
	block1.modelID = "block"_id;
	block1.color = {0.9f, 0.1f, 0.1f, 1.0f};
	block1.transform = _t;
	block1.transform.position += float3{-4.0f, -4.0f, 2.0f} * _t.scale;
	
	m_blocks[2] = s_renderer->objects().create();
	auto block2 = s_renderer->objects().get(m_blocks[2]);
	block2.modelID = "block"_id;
	block2.color = {0.9f, 0.1f, 0.1f, 1.0f};
	block2.transform = _t;
	block2.transform.position += float3{4.0f, -4.0f, 2.0f} * _t.scale;
	
	m_blocks[3] = s_renderer->objects().create();
	auto block3 = s_renderer->objects().get(m_blocks[3]);
	block3.modelID = "block"_id;
	block3.color = {0.9f, 0.1f, 0.1f, 1.0f};
	block3.transform = _t;
	block3.transform.position += float3{-4.0f, 4.0f, 2.0f} * _t.scale;
	
	m_blocks[4] = s_renderer->objects().create();
	auto block4 = s_renderer->objects().get(m_blocks[4]);
	block4.modelID = "block"_id;
	block4.color = {0.9f, 0.1f, 0.1f, 1.0f};
	block4.transform = _t;
	block4.transform.position += float3{4.0f, 4.0f, 2.0f} * _t.scale;
	
	m_blocks[5] = s_renderer->objects().create();
	auto block5 = s_renderer->objects().get(m_blocks[5]);
	block5.modelID = "block"_id;
	block5.color = {0.1f, 0.1f, 0.9f, 1.0f};
	block5.transform = _t;
	block5.transform.position += float3{7.0f, 0.0f, 2.0f} * _t.scale;
	
	for (auto i: stx::iota(0, 9)) {
		
		m_spheresLeft[i] = s_renderer->objects().create();
		auto sphereLeft = s_renderer->objects().get(m_spheresLeft[i]);
		sphereLeft.modelID = "sphere"_id;
		sphereLeft.transform = _t;
		sphereLeft.transform.position += float3{float(i - 4) * 2.0f, 8.0f, 2.0f} * _t.scale;
		
		m_spheresRight[i] = s_renderer->objects().create();
		auto sphereRight = s_renderer->objects().get(m_spheresRight[i]);
		sphereRight.modelID = "sphere"_id;
		sphereRight.transform = _t;
		sphereRight.transform.position += float3{float(i - 4) * 2.0f, -8.0f, 2.0f} * _t.scale;
		
	}
	
}

SimpleScene::~SimpleScene() {
	
	s_renderer->objects().destroy(m_spinny);
	for (auto block: m_blocks)
		s_renderer->objects().destroy(block);
	for (auto sphere: m_spheresLeft)
		s_renderer->objects().destroy(sphere);
	for (auto sphere: m_spheresRight)
		s_renderer->objects().destroy(sphere);
	
}

void SimpleScene::update() {
	
	auto rotateAnim = quat::angleAxis(radians(stx::ratio(s_system->getTime(), 20_ms)), {0.0f, 0.0f, 1.0f});
	s_renderer->objects().get(m_spinny).transform.rotation = rotateAnim;
	
}

}
