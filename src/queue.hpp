/**
 * Statically allocated FIFO queue
 * @file
 * Uses a ring buffer as a backing store. All methods are O(1).
 */

#pragma once

#include <cstdlib>
#include <array>

namespace minote {

template<typename T, std::size_t N>
struct queue {

	using Element = T;
	static constexpr std::size_t Capacity{N};

	std::array<Element, Capacity> data{}; ///< Ring buffer of elements
	std::size_t head{0}; ///< Index of the first empty space to enqueue into
	std::size_t tail{0}; ///< Index of the next element to dequeue

	/**
	 * Add an element to the back of the queue. If there is no space, nothing
	 * happens.
	 * @param e Element to add
	 * @return true if added, false if out of space
	 */
	auto enqueue(const Element& e) -> bool;

	/**
	 * Remove and return an element from the front of the queue. If the queue is
     * empty, nothing happens.
	 * @return Removed element, or nullptr if queue is empty
	 */
	auto dequeue() -> Element*;

	/**
	 * Return the element from the front of the queue without removing it.
	 * If the queue is empty, nothing happens.
	 * @return Peeked element, or nullptr if queue is empty
	 */
	auto peek() -> Element*;

	/**
	 * Check whether the queue is empty.
	 * @return true is empty, false if not empty
	 */
	auto isEmpty() -> bool;

	/**
	 * Check whether the queue is full.
	 * @return true if full, false if not full
	 */
	auto isFull() -> bool;

	/**
	 * Clear the queue, setting the number of elements to zero.
	 */
	void clear();

};

}

#include "queue.tpp"
