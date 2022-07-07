#pragma once

#include <utility>
#include "robin_hood.h"

namespace minote {

// Unordered hash map. Pointers are stable.
template<typename Key, typename T>
using hashmap = robin_hood::unordered_node_map<Key, T>;

}
