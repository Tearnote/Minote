#pragma once

#include <utility>
#include "absl/container/flat_hash_map.h"

namespace minote::base {

// Unordered hash map. References aren't stable.
template<typename Key, typename T>
using hashmap = absl::flat_hash_map<Key, T,
	absl::container_internal::hash_default_hash<Key>,
	absl::container_internal::hash_default_eq<Key>>;

}
