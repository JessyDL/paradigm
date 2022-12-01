#pragma once
#include "details/execution.hpp"
#include "psl/ecs/details/system_information.hpp"
#include "psl/ecs/state.hpp"
#include <future>
#include "selectors.hpp"

namespace psl::ecs::details
{
	template <typename Pred, typename T>
	static inline void order_by(psl::ecs::execution::no_exec,
								const psl::ecs::state_t& state,
								psl::array<entity>::iterator begin,
								psl::array<entity>::iterator end) noexcept
	{
		const auto pred = Pred {};
		std::sort(begin, end, [&state, &pred](entity lhs, entity rhs) -> bool {
			return std::invoke(pred, state.get<T>(lhs), state.get<T>(rhs));
		});
	}

	template <typename Pred, typename T>
	static inline void order_by(psl::ecs::execution::sequenced_policy,
								const psl::ecs::state_t& state,
								psl::array<entity>::iterator begin,
								psl::array<entity>::iterator end) noexcept
	{
		const auto pred = Pred {};
		std::sort(psl::ecs::execution::seq, begin, end, [&state, &pred](entity lhs, entity rhs) -> bool {
			return std::invoke(pred, state.get<T>(lhs), state.get<T>(rhs));
		});
	}
	template <typename Pred, typename T>
	static inline void order_by(psl::ecs::execution::parallel_policy,
								const psl::ecs::state_t& state,
								psl::array<entity>::iterator begin,
								psl::array<entity>::iterator end,
								size_t max) noexcept
	{
		auto size = std::distance(begin, end);
		if(size <= static_cast<decltype(size)>(max))
		{
			psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::seq, state, begin, end);
		}
		else
		{
			auto middle = std::next(begin, size / 2);

			auto future = std::async(
			  [&state](auto begin, auto middle, auto max) {
				  psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, begin, middle, max);
			  },
			  begin,
			  middle,
			  max);

			psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, middle, end, max);

			const auto pred = Pred {};
			future.wait();
			if constexpr(std::is_same_v<psl::ecs::execution::parallel_unsequenced_policy, psl::ecs::execution::no_exec>)
			{
				std::inplace_merge(begin, middle, end, [&state, &pred](entity lhs, entity rhs) -> bool {
					return std::invoke(pred, state.get<T>(lhs), state.get<T>(rhs));
				});
			}
			else
			{
				std::inplace_merge(
				  psl::ecs::execution::par_unseq, begin, middle, end, [&state, &pred](entity lhs, entity rhs) -> bool {
					  return std::invoke(pred, state.get<T>(lhs), state.get<T>(rhs));
				  });
			}
		}
	}

	template <typename Pred, typename T>
	static inline void order_by(psl::ecs::execution::parallel_policy,
								const psl::ecs::state_t& state,
								psl::array<entity>::iterator begin,
								psl::array<entity>::iterator end) noexcept
	{
		auto size		 = std::distance(begin, end);
		auto thread_size = std::max<size_t>(1u, std::min<size_t>(std::thread::hardware_concurrency(), size % 1024u));
		size /= thread_size;


		psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, begin, end, size);
	}
    
	template <typename Pred, typename T>
	static inline void
	order_by(const psl::ecs::state_t& state, psl::array<entity>::iterator begin, psl::array<entity>::iterator end) noexcept
	{
		psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::seq, state, begin, end);
	}

	template <typename Pred, typename... Ts>
	void dependency_pack::select_ordering_impl(std::pair<Pred, std::tuple<Ts...>>)
	{
		static_assert(sizeof...(Ts) == 1, "due to a bug in MSVC we cannot have deeper nested template packs");
		orderby =
		  [](psl::array<entity>::iterator begin, psl::array<entity>::iterator end, const psl::ecs::state_t& state) {
			  psl::ecs::details::order_by<Pred, Ts...>(psl::ecs::execution::par, state, begin, end);
		  };
	}
    
    template <typename Pred, typename T>
    void transform_group::selector(psl::type_pack_t<psl::ecs::order_by<Pred, T>>) noexcept
    {
        order_by = [](psl::array<entity>::iterator begin, psl::array<entity>::iterator end, const auto& state) {
            psl::ecs::details::order_by<Pred, T>(psl::ecs::execution::par, state, begin, end);
        };
    }
}	 // namespace psl::ecs::details