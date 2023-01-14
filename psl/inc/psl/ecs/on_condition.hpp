#pragma once
#include <algorithm>
#include <tuple>
#include <type_traits>
#include <utility>

#include "psl/array.hpp"
#include "psl/ecs/details/execution.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/selectors.hpp"
#include "psl/ecs/state.hpp"

namespace psl::ecs::details {
template <typename Pred, typename T>
static inline psl::array<entity_t>::iterator on_condition(const psl::ecs::state_t& state,
														  psl::array<entity_t>::iterator begin,
														  psl::array<entity_t>::iterator end) noexcept {
	auto pred = Pred {};
	if constexpr(std::is_same_v<psl::ecs::execution::parallel_unsequenced_policy, psl::ecs::execution::no_exec>) {
		return std::remove_if(begin, end, [&state, &pred](entity_t lhs) -> bool { return !pred(state.get<T>(lhs)); });
	} else {
		return std::remove_if(psl::ecs::execution::par_unseq, begin, end, [&state, &pred](entity_t lhs) -> bool {
			return !pred(state.get<T>(lhs));
		});
	}
}

template <typename T, typename Pred>
psl::array<entity_t>::iterator static inline on_condition(const psl::ecs::state_t& state,
														  psl::array<entity_t>::iterator begin,
														  psl::array<entity_t>::iterator end,
														  Pred&& pred) noexcept {
	if constexpr(std::is_same_v<psl::ecs::execution::parallel_unsequenced_policy, psl::ecs::execution::no_exec>) {
		return std::remove_if(begin, end, [&state, &pred](entity_t lhs) -> bool { return !pred(state.get<T>(lhs)); });
	} else {
		return std::remove_if(psl::ecs::execution::par_unseq, begin, end, [&state, &pred](entity_t lhs) -> bool {
			return !pred(state.get<T>(lhs));
		});
	}
}

template <typename Pred, typename... Ts>
void dependency_pack::select_condition_impl(std::pair<Pred, std::tuple<Ts...>>) {
	on_condition.push_back([](psl::array<entity_t>::iterator begin,
							  psl::array<entity_t>::iterator end,
							  const psl::ecs::state_t& state) -> psl::array<entity_t>::iterator {
		return psl::ecs::details::on_condition<Pred, Ts...>(state, begin, end);
	});
}

template <typename Pred, typename T>
void transform_group::selector(psl::type_pack_t<psl::ecs::on_condition<Pred, T>>) noexcept {
	on_condition.emplace_back([](psl::array<entity_t>::iterator begin,
								 psl::array<entity_t>::iterator end,
								 const auto& state) -> psl::array<entity_t>::iterator {
		return psl::ecs::details::on_condition<Pred, T>(state, begin, end);
	});
}
}	 // namespace psl::ecs::details
