/**
 * Statically allocated variable-size array
 * @file
 * Keeps track of the number of elements. Uses a standard array as a backing
 * store.
 */

#pragma once

#include <cstdlib>
#include <array>
#include "base/util.hpp"

namespace minote {

template<typename T, std::size_t N>
struct varray {

	using Element = T;
	static constexpr std::size_t Capacity = N;

	std::array<Element, Capacity> buffer = {}; ///< Array containing the elements
	std::size_t size = 0; ///< Number of elements currently in #data

	/**
	 * Add a new element at the end and return a pointer to it.
	 * @return pointer to newly added element, or nullptr if backing store full
	 */
	auto produce() -> optref<Element>;

	/**
	 * Remove an element at a given index. Other elements are shifted to fill
     * the gap, which makes this an O(n) operation that should be avoided
     * for any large arrays.
	 * @param index Index of the element to remove
	 */
	void remove(std::size_t index);

	/**
	 * Remove an element at a given index and put the last element in its place.
     * This removal is an O(1) operation, at the cost of changing the ordering
     * of remaining elements.
	 * @param index
	 */
	void removeSwap(std::size_t index);

	/**
	 * Clear the array by setting the number of elements to zero.
	 */
	void clear();

	auto data() -> Element*;
	auto data() const -> const Element*;

	auto operator[](std::size_t index) -> Element&;
	auto operator[](std::size_t index) const -> const Element&;

};

}

#include "varray.tpp"
