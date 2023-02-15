#pragma once

#include <array>
#include "gfx/objects.hpp"

namespace minote::game {

// A complex scene of a battle between humans and skeletons
class BattleScene {

public:

	// Create a new scene
	BattleScene(Transform t = {});
	~BattleScene();
	
	// Not copyable, not moveable
	BattleScene(BattleScene const&) = delete;
	auto operator=(BattleScene const&) -> BattleScene& = delete;
	
private:
	
	ObjectID m_id;
	
};

// A bunch of cubes and spheres
class SimpleScene {

public:
	
	// Create a new scene
	SimpleScene(Transform t = {});
	~SimpleScene();
	
	// Spin the cube in the middle
	void update();

	// Not copyable, not moveable
	SimpleScene(SimpleScene const&) = delete;
	auto operator=(SimpleScene const&) -> SimpleScene& = delete;
	
private:
	
	ObjectID m_spinny;
	std::array<ObjectID, 6> m_blocks;
	std::array<ObjectID, 9> m_spheresLeft;
	std::array<ObjectID, 9> m_spheresRight;
	
};

}
