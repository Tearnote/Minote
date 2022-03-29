#pragma once

#include <utility>
#include "robin_hood.h"

namespace minote::base {

// Unordered hash map. Pointers are stable.
template<typename Key, typename T>
using hashmap = robin_hood::unordered_node_map<Key, T>;

}
