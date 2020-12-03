// Minote - base/queue.hpp
// Static FIFO queue with limited capacity. Uses a ring buffer as storage.
// All methods are O(1).

#pragma once

#include <type_traits>
#include <iterator>
#include <limits>

namespace minote {

template<typename T, std::size_t Capacity>
struct ring_buffer {

	using value_type = T;
	using reference = value_type&;
	using const_reference = value_type const&;
	struct iterator; // LegacyRandomAccessIterator
	using const_iterator = iterator const;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;

	constexpr ring_buffer(): offset{0}, length{0} {}
	constexpr ring_buffer(ring_buffer const&) = default;
	~ring_buffer() { clear(); };

	constexpr auto size() const { return length; }
	constexpr auto empty() const { return length == 0; }
	constexpr auto capacity() const { return Capacity; }
	constexpr auto max_size() const { return std::numeric_limits<size_type>::max(); }

	auto at(size_type i) -> reference;
	auto at(size_type i) const -> const_reference;
	auto front() -> reference { return at(0); }
	auto front() const -> const_reference { return at(0); }
	auto back() -> reference { return at(length - 1); }
	auto back() const -> const_reference { return at(length - 1); }

	auto begin() { return iterator{*this, 0}; }
	auto begin() const { return const_iterator{*this, 0}; }
	auto cbegin() const { return begin(); }
	auto end() { return iterator{*this, size()}; }
	auto end() const { return const_iterator{*this, size()}; }
	auto cend() const { return end(); }
	auto rbegin() { return reverse_iterator{end()}; }
	auto rbegin() const { return const_reverse_iterator{end()}; }
	auto crbegin() const { return rbegin(); }
	auto rend() { return reverse_iterator{begin()}; }
	auto rend() const { return const_reverse_iterator{begin()}; }
	auto crend() const { return rend(); }

	void push_back(const_reference value);
	void push_back(value_type&& value);
	void push_front(const_reference value);
	void push_front(value_type&& value);
	void pop_front();
	void pop_back();
	void clear() { while (!empty()) pop_back(); }

	constexpr auto operator=(ring_buffer const&) -> ring_buffer& = default;
	auto operator[](size_type const index) -> reference { return at(index); }
	auto operator[](size_type const index) const -> const_reference { return at(index); }

private:

	typename std::aligned_storage<sizeof(value_type), alignof(value_type)> buffer[Capacity];
	size_type offset;
	size_type length;

};

}

#include "queue.tpp"
