// Minote - base/queue.hpp
// Static FIFO queue with limited capacity. Uses a ring buffer as storage.
// All methods are O(1).

#pragma once

#include "base/array.hpp"
#include "base/util.hpp"

namespace minote {

template<typename T, size_t N>
struct queue {

	// Type of the queue element
	using Element = T;

	// Maximum number of elements in the queue plus one
	static constexpr size_t Capacity = N;

	// Ring buffer containing the elements
	array<Element, Capacity> buffer = {};

	// Index of the first empty slot to enqueue into
	size_t head = 0;

	// Index of the next element to dequeue
	size_t tail = 0;

	// Add an element to the back of the queue and return true. If there is
	// no space, the element is not added and false is returned.
	auto enqueue(Element const& e) -> bool;

	// Remove and return an element from the front of the queue. If the queue is
    // empty, nullptr is returned.
	auto dequeue() -> Element*;

	// Return the element from the front of the queue without removing it.
	// If the queue is empty, nullptr is returned.
	auto peek() -> Element*;
	auto peek() const -> Element const*;

	// Return true if the queue is empty, false otherwise.
	[[nodiscard]]
	auto isEmpty() const -> bool;

	// Return true if the queue is full, false otherwise.
	[[nodiscard]]
	auto isFull() const -> bool;

	// Clear the queue, setting the number of elements to zero.
	void clear();

};

}

#include "queue.tpp"
