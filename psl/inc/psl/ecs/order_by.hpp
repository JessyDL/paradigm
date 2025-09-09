#pragma once
#include "details/execution.hpp"
#include "psl/ecs/details/system_information.hpp"
#include "psl/ecs/state.hpp"
#include "selectors.hpp"
#include <future>

namespace psl::ecs::details {
template <typename T>
struct get_pair_type {
	using pair_t = std::pair<T, entity_t>;
	static constexpr auto is_big {false};

	static psl::array<pair_t> make_array(const psl::ecs::state_t& state,
										 psl::array<entity_t>::iterator begin,
										 psl::array<entity_t>::iterator end) {
		auto const count = (size_t)std::distance(begin, end);
		psl::array_view<entity_t> view(begin, end);
		auto data = state.get<T>(view);
		psl::array<pair_t> result {};
		result.reserve(std::distance(begin, end));
		for(auto i = size_t {0}; i < count; ++i, ++begin) {
			result.emplace_back(std::move(data[i]), *begin);
		}
		return result;
	}
};

template <typename T>
	requires(sizeof(T) > sizeof(void*))
struct get_pair_type<T> {
	using pair_t = std::pair<T*, entity_t>;
	static constexpr auto is_big {true};

	static psl::array<pair_t> make_array(const psl::ecs::state_t& state,
										 psl::array<entity_t>::iterator begin,
										 psl::array<entity_t>::iterator end) {
		auto const count = (size_t)std::distance(begin, end);
		psl::array_view<entity_t> view(begin, end);
		auto data = state.try_get_component<T>(view);
		psl::array<pair_t> result {};
		result.reserve(std::distance(begin, end));
		for(auto i = size_t {0}; i < count; ++i, ++begin) {
			result.emplace_back(data[i], *begin);
		}
		return result;
	}
};

template <typename Pred, typename T>
static inline void order_by(psl::ecs::execution::no_exec,
							const psl::ecs::state_t& state,
							psl::array<entity_t>::iterator begin,
							psl::array<entity_t>::iterator end) noexcept {
	auto sortable = get_pair_type<T>::make_array(state, begin, end);
	order_by<Pred, T>(psl::ecs::execution::no_exec {}, state, sortable.begin(), sortable.end());
	for(size_t i = 0; i < sortable.size(); ++i) {
		*(begin + i) = sortable[i].second;
	}
}

template <typename Pred, typename T>
static inline void order_by(psl::ecs::execution::sequenced_policy,
							const psl::ecs::state_t& state,
							psl::array<entity_t>::iterator begin,
							psl::array<entity_t>::iterator end) noexcept {
	auto sortable = get_pair_type<T>::make_array(state, begin, end);
	order_by<Pred, T>(psl::ecs::execution::sequenced_policy {}, state, sortable.begin(), sortable.end());
	for(size_t i = 0; i < sortable.size(); ++i) {
		*(begin + i) = sortable[i].second;
	}
}

template <typename Pred, typename T>
static inline void order_by(psl::ecs::execution::no_exec,
							const psl::ecs::state_t& state,
							typename psl::array<typename get_pair_type<T>::pair_t>::iterator begin,
							typename psl::array<typename get_pair_type<T>::pair_t>::iterator end) noexcept {
	std::sort(begin, end, [](auto& lhs, auto& rhs) -> bool {
		if constexpr(get_pair_type<T>::is_big) {
			return Pred {}(*lhs.first, *rhs.first);
		} else {
			return Pred {}(lhs.first, rhs.first);
		}
	});
}

template <typename Pred, typename T>
static inline void order_by(psl::ecs::execution::sequenced_policy,
							const psl::ecs::state_t& state,
							typename psl::array<typename get_pair_type<T>::pair_t>::iterator begin,
							typename psl::array<typename get_pair_type<T>::pair_t>::iterator end) noexcept {
	std::sort(psl::ecs::execution::seq, begin, end, [](auto& lhs, auto& rhs) -> bool {
		if constexpr(get_pair_type<T>::is_big) {
			return Pred {}(*lhs.first, *rhs.first);
		} else {
			return Pred {}(lhs.first, rhs.first);
		}
	});
}

template <typename Pred, typename T>
static inline void order_by(psl::ecs::execution::parallel_policy,
							const psl::ecs::state_t& state,
							typename psl::array<typename get_pair_type<T>::pair_t>::iterator begin,
							typename psl::array<typename get_pair_type<T>::pair_t>::iterator end,
							size_t max) noexcept {
	auto size = std::distance(begin, end);
	if(size == 0) {
		return;
	}

	if(size <= static_cast<decltype(size)>(max)) {
		psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::seq, state, begin, end);
	} else {
		auto middle = std::next(begin, size / 2);

		auto future = std::async(
		  [&state](auto begin, auto middle, auto max) {
			  psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, begin, middle, max);
		  },
		  begin,
		  middle,
		  max);

		psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, middle, end, max);

		future.wait();
		if constexpr(std::is_same_v<psl::ecs::execution::parallel_unsequenced_policy, psl::ecs::execution::no_exec>) {
			std::inplace_merge(begin, middle, end, [&state](auto const& lhs, auto const& rhs) -> bool {
				if constexpr(get_pair_type<T>::is_big) {
					return Pred {}(*lhs.first, *rhs.first);
				} else {
					return Pred {}(lhs.first, rhs.first);
				}
			});
		} else {
			std::inplace_merge(
			  psl::ecs::execution::par_unseq, begin, middle, end, [&state](auto const& lhs, auto const& rhs) -> bool {
				  if constexpr(get_pair_type<T>::is_big) {
					  return Pred {}(*lhs.first, *rhs.first);
				  } else {
					  return Pred {}(lhs.first, rhs.first);
				  }
			  });
		}
	}
}

template <typename Pred, typename T>
static inline void order_by(psl::ecs::execution::parallel_policy,
							const psl::ecs::state_t& state,
							psl::array<entity_t>::iterator begin,
							psl::array<entity_t>::iterator end,
							size_t max) noexcept {
	auto size = std::distance(begin, end);
	if(size == 0) {
		return;
	}
	auto sortable = get_pair_type<T>::make_array(state, begin, end);

	order_by<Pred, T>(psl::ecs::execution::parallel_policy {}, state, sortable.begin(), sortable.end(), max);

	for(size_t i = 0; i < sortable.size(); ++i) {
		*(begin + i) = sortable[i].second;
	}
}

template <typename Pred, typename T>
static inline void order_by(psl::ecs::execution::parallel_policy,
							const psl::ecs::state_t& state,
							psl::array<entity_t>::iterator begin,
							psl::array<entity_t>::iterator end) noexcept {
	auto size = std::distance(begin, end);
	if(size == 0) {
		return;
	}
	auto thread_size = std::max<size_t>(1u, std::min<size_t>(std::thread::hardware_concurrency(), size % (1 << 12)));
	size /= thread_size;


	psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, begin, end, size);
}

template <typename Pred, typename T>
static inline void order_by(const psl::ecs::state_t& state,
							psl::array<entity_t>::iterator begin,
							psl::array<entity_t>::iterator end) noexcept {
	psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::seq, state, begin, end);
}

template <typename Pred, typename... Ts>
void dependency_pack::select_ordering_impl(std::pair<Pred, std::tuple<Ts...>>) {
	static_assert(sizeof...(Ts) == 1, "due to a bug in MSVC we cannot have deeper nested template packs");
	orderby =
	  [](psl::array<entity_t>::iterator begin, psl::array<entity_t>::iterator end, const psl::ecs::state_t& state) {
		  psl::ecs::details::order_by<Pred, Ts...>(psl::ecs::execution::par, state, begin, end);
	  };
}

template <typename Pred, typename T>
void transform_group::selector(psl::type_pack_t<psl::ecs::order_by<Pred, T>>) noexcept {
	order_by = [](psl::array<entity_t>::iterator begin, psl::array<entity_t>::iterator end, const auto& state) {
		psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, begin, end);
	};
}
}	 // namespace psl::ecs::details
