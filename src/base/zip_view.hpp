#pragma once

#include <iterator>
#include <utility>
#include <tuple>

namespace minote::base {

// Implementation of zip_view, which didn't make it into C++20
// See: https://gist.github.com/mortehu/373069390c75b02f98b655e3f7dbef9a
template<typename... T>
class zip_view {
public:
	class iterator
		: std::iterator<std::forward_iterator_tag,
			std::tuple<decltype(*std::declval<T>().begin())...>> {
	private:
		std::tuple<decltype(std::declval<T>().begin())...> iters_;

		template<std::size_t... I>
		auto deref(std::index_sequence<I...>) const {
			return typename iterator::value_type{*std::get<I>(iters_)...};
		}

		template<std::size_t... I>
		void increment(std::index_sequence<I...>) {
			[[maybe_unused]]
			auto l = {(++std::get<I>(iters_), 0)...};
		}

	public:
		explicit iterator(decltype(iters_) iters): iters_{std::move(iters)} {}

		iterator& operator++() {
			increment(std::index_sequence_for<T...>{});
			return *this;
		}

		iterator operator++(int) {
			auto saved{*this};
			increment(std::index_sequence_for<T...>{});
			return saved;
		}

		bool operator!=(const iterator& other) const {
			return iters_ != other.iters_;
		}

		auto operator*() const { return deref(std::index_sequence_for<T...>{}); }
	};

	zip_view(T&... seqs)
		: begin_{std::make_tuple(seqs.begin()...)},
		  end_{std::make_tuple(seqs.end()...)} {}

	iterator begin() const { return begin_; }
	iterator end() const { return end_; }

private:
	iterator begin_;
	iterator end_;
};

// Sequences must be the same length.
template<typename... T>
auto zip(T&&... seqs) {
	return zip_view<T...>{seqs...};
}

}
