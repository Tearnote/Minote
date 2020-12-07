// Minote - base/hash.hpp
// Hashmap implementation of choice

#pragma once

#include "robin-hood-hashing/robin_hood.h"

namespace minote {

template<typename Key, typename T>
using hashmap = robin_hood::unordered_flat_map<Key, T>;

}
