// Minote - base/ring.hpp
// STL-style ring buffer. The storage capacity is bounded and placed on stack - any allocation
// that goes over the capacity limit will throw a std::out_of_range.

#pragma once

#include <initializer_list>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <memory>
#include <limits>
#include <array>

namespace minote {

template<typename T, std::size_t Capacity>
struct ring {

	template<typename U>
	struct iter;

	using value_type = T;
	using reference = value_type&;
	using const_reference = value_type const&;
	using iterator = iter<value_type>;
	using const_iterator = iter<value_type const>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;

	ring() = default;
	ring(ring const&) = default;
	explicit ring(size_type num);
	ring(size_type num, value_type const& val);
	template<typename InputIt>
	ring(InputIt first, InputIt last);
	ring(std::initializer_list<value_type>);
	~ring() { clear(); };
	void swap(ring& other);

	constexpr auto size() const { return length; }
	constexpr auto empty() const { return length == 0; }
	constexpr auto full() const { return length == Capacity; } // Non-standard extension
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
	template<typename... Args>
	auto emplace_back(Args&&... args) -> reference;
	template<typename... Args>
	auto emplace_front(Args&&... args) -> reference;
	void pop_front();
	void pop_back();
	void clear() { std::destroy_n(begin(), size()); length = 0; }

	auto operator=(ring const&) -> ring& = default;
	auto operator=(std::initializer_list<value_type>) -> ring&;
	auto operator[](size_type const index) -> reference { return at(index); }
	auto operator[](size_type const index) const -> const_reference { return at(index); }
	auto operator==(ring const& other) const { return std::equal(begin(), end(), other.begin()); }
	auto operator!=(ring const&) const -> bool = default;

private:

	std::array<typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type, Capacity> buffer;
	size_type offset{0};
	size_type length{0};

};

template<typename T, std::size_t Capacity>
template<typename U>
struct ring<T, Capacity>::iter {

	static_assert(std::is_same_v<T, std::remove_cv_t<U>>);

	using parent_type = ring<T, Capacity>;

	using iterator_category = std::random_access_iterator_tag;
	using value_type = U;
	using difference_type = parent_type::difference_type;
	using pointer = value_type*;
	using reference = value_type&;

	constexpr iter(parent_type& p, size_type i): parent{p}, index{i} {}
	constexpr iter(iter const&) = default;

	constexpr auto operator<=>(iter const& other) const { return index <=> other.index; }
	constexpr auto operator==(iter const& other) const { return index == other.index; }
	constexpr auto operator!=(iter const& other) const -> bool = default;
	constexpr auto operator-(iter const& other) const {
		return static_cast<difference_type>(index) - static_cast<difference_type>(other.index);
	}

	auto operator*() -> reference { return parent[index]; }
	auto operator[](difference_type const i) -> reference { return parent[index + i]; }

	constexpr auto operator++() -> iter& { index += 1; return *this; }
	constexpr auto operator++(int) -> iter { index += 1; return iter{parent, index - 1}; }
	constexpr auto operator--() -> iter& { index -= 1; return *this; }
	constexpr auto operator--(int) -> iter { index -= 1; return iter{parent, index + 1}; }
	constexpr auto operator+=(difference_type const i) -> iter& { index += i; return *this; }
	constexpr auto operator-=(difference_type const i) -> iter& { index -= i; return *this; }
	friend constexpr auto operator+(iter self, iter const& other) -> iter { self += other; return self; }
	friend constexpr auto operator-(iter self, iter const& other) -> iter { self -= other; return self; }

private:

	parent_type& parent;
	size_type index;

};

}

#include "ring.tpp"
