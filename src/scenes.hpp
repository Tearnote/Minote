#pragma once

#include "util/array.hpp"
#include "gfx/objects.hpp"

namespace minote {

// A complex scene of a battle between humans and skeletons
struct BattleScene {
	
	BattleScene(Transform t = {});
	~BattleScene();
	
	BattleScene(BattleScene const&) = delete;
	auto operator=(BattleScene const&) -> BattleScene& = delete;
	
private:
	
	ObjectID m_id;
	
};

// A bunch of cubes and spheres
struct SimpleScene {
	
	SimpleScene(Transform t = {});
	~SimpleScene();
	
	void update();
	
	SimpleScene(SimpleScene const&) = delete;
	auto operator=(SimpleScene const&) -> SimpleScene& = delete;
	
private:
	
	ObjectID m_spinny;
	array<ObjectID, 6> m_blocks;
	array<ObjectID, 9> m_spheresLeft;
	array<ObjectID, 9> m_spheresRight;
	
};

}
