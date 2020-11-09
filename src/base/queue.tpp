/**
 * Template implementation of queue.hpp
 * @file
 */

#pragma once

namespace minote {

template<typename T, size_t N>
auto queue<T, N>::enqueue(Element const& e) -> bool
{
	if (isFull())
		return false;

	buffer[head] = e;
	head = (head + 1) % Capacity;
	return true;
}

template<typename T, size_t N>
auto queue<T, N>::dequeue() -> Element*
{
	if (isEmpty())
		return nullptr;

	auto const prevTail = tail;
	tail = (tail + 1) % Capacity;
	return &buffer[prevTail];
}

template<typename T, size_t N>
auto queue<T, N>::peek() -> Element*
{
	if (isEmpty())
		return nullptr;

	return &buffer[tail];
}

template<typename T, size_t N>
auto queue<T, N>::peek() const -> Element const*
{
	if (isEmpty())
		return nullptr;

	return &buffer[tail];
}

template<typename T, size_t N>
auto queue<T, N>::isEmpty() const -> bool
{
	return head == tail;
}

template<typename T, size_t N>
auto queue<T, N>::isFull() const -> bool
{
	return (head + 1) % Capacity == tail;
}

template<typename T, size_t N>
void queue<T, N>::clear()
{
	head = tail;
}

}
