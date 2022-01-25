#pragma once
#include "details/component_key.hpp"
#include "psl/array.hpp"
#include "psl/ecs/pack.hpp"
#include "psl/template_utils.hpp"
#include "selectors.hpp"
#include <execution>

namespace psl::ecs
{
	class state_t;
	namespace details
	{
		// unlike filter_groups, transform groups are dynamic operations on every element of a filtered list
		class transform_group
		{
			using ordering_pred_t	 = void(psl::array<entity>::iterator,
											psl::array<entity>::iterator,
											const psl::ecs::state_t&);
			using conditional_pred_t = psl::array<entity>::iterator(psl::array<entity>::iterator,
																	psl::array<entity>::iterator,
																	const psl::ecs::state_t&);
			template <typename T>
			constexpr void selector(psl::templates::type_container<T>) noexcept
			{}

			template <typename Pred, typename T>
			constexpr void selector(psl::templates::type_container<order_by<Pred, T>>) noexcept
			{
				order_by = [](psl::array<entity>::iterator begin, psl::array<entity>::iterator end, const auto& state) {
					state.template order_by<Pred, T>(std::execution::par, begin, end);
				};
			}


			template <typename Pred, typename T>
			constexpr void selector(psl::templates::type_container<on_condition<Pred, T>>) noexcept
			{
				on_condition.emplace_back([](psl::array<entity>::iterator begin,
											 psl::array<entity>::iterator end,
											 const auto& state) -> psl::array<entity>::iterator {
					return state.template on_condition<Pred, T>(begin, end);
				});
			}

		  public:
			template <typename... Ts>
			transform_group(psl::templates::type_container<Ts>...)
			{
				(void(selector(psl::templates::type_container<Ts>())), ...);
			};
			~transform_group() = default;

			transform_group(const transform_group& other)	  = default;
			transform_group(transform_group&& other) noexcept = default;
			transform_group& operator=(const transform_group& other) = default;
			transform_group& operator=(transform_group&& other) noexcept = default;

			psl::array<entity>::iterator transform(psl::array<entity>::iterator begin,
												   psl::array<entity>::iterator end,
												   const state_t& state) const noexcept
			{
				for(const auto& condition : on_condition) end = condition(begin, end, state);

				if(order_by) order_by(begin, end, state);
				return end;
			}

			operator bool() const noexcept { return order_by || on_condition.size() > 0; }

		  private:
			friend class ::psl::ecs::state_t;
			void add_debug_system_name(psl::string_view name) { m_SystemsDebugNames.emplace_back(name); }
			std::function<ordering_pred_t> order_by;

			psl::array<std::function<conditional_pred_t>> on_condition;
			psl::array<psl::string_view> m_SystemsDebugNames;
		};


		class filter_group
		{
			template <typename T>
			constexpr void selector(psl::templates::type_container<T>) noexcept
			{
				if constexpr(!std::is_same_v<entity, T> && !std::is_same_v<psl::ecs::partial, T> &&
							 !std::is_same_v<psl::ecs::full, T>)
					filters.emplace_back(details::key_for<T>());
			}

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<filter<Ts...>>) noexcept
			{
				(selector(psl::templates::type_container<Ts>()), ...);
			}

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<on_combine<Ts...>>) noexcept
			{
				(void(on_combine.emplace_back(details::key_for<Ts>())), ...);
			}

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<on_break<Ts...>>) noexcept
			{
				(void(on_break.emplace_back(details::key_for<Ts>())), ...);
			}

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<except<Ts...>>) noexcept
			{
				(void(except.emplace_back(details::key_for<Ts>())), ...);
			}

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<on_add<Ts...>>) noexcept
			{
				(void(on_add.emplace_back(details::key_for<Ts>())), ...);
			}


			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<on_remove<Ts...>>) noexcept
			{
				(void(on_remove.emplace_back(details::key_for<Ts>())), ...);
			}

			template <typename Pred, typename... Ts>
			constexpr void selector(psl::templates::type_container<order_by<Pred, Ts...>>) noexcept
			{}

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<on_condition<Ts...>>) noexcept
			{}
			friend class ::psl::ecs::state_t;
			filter_group() = default;
			filter_group(psl::array<component_key_t> filters_arr,
						 psl::array<component_key_t> on_add_arr,
						 psl::array<component_key_t> on_remove_arr,
						 psl::array<component_key_t> except_arr,
						 psl::array<component_key_t> on_combine_arr,
						 psl::array<component_key_t> on_break_arr) :
				filters(filters_arr),
				on_add(on_add_arr), on_remove(on_remove_arr), except(except_arr), on_combine(on_combine_arr),
				on_break(on_break_arr)
			{
				std::sort(std::begin(filters), std::end(filters));
				std::sort(std::begin(on_add), std::end(on_add));
				std::sort(std::begin(on_remove), std::end(on_remove));
				std::sort(std::begin(except), std::end(except));
				std::sort(std::begin(on_combine), std::end(on_combine));
				std::sort(std::begin(on_break), std::end(on_break));

				filters.erase(std::unique(std::begin(filters), std::end(filters)), std::end(filters));
				auto cpy = filters;
				filters.clear();
				std::set_difference(
				  std::begin(cpy), std::end(cpy), std::begin(on_add), std::end(on_add), std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy),
									std::end(cpy),
									std::begin(on_remove),
									std::end(on_remove),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(
				  std::begin(cpy), std::end(cpy), std::begin(except), std::end(except), std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy),
									std::end(cpy),
									std::begin(on_combine),
									std::end(on_combine),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy),
									std::end(cpy),
									std::begin(on_break),
									std::end(on_break),
									std::back_inserter(filters));
			};

		  public:
			template <typename... Ts>
			filter_group(psl::templates::type_container<Ts>...)
			{
				(void(selector(psl::templates::type_container<Ts>())), ...);
				std::sort(std::begin(filters), std::end(filters));
				std::sort(std::begin(on_add), std::end(on_add));
				std::sort(std::begin(on_remove), std::end(on_remove));
				std::sort(std::begin(except), std::end(except));
				std::sort(std::begin(on_combine), std::end(on_combine));
				std::sort(std::begin(on_break), std::end(on_break));


				filters.erase(std::unique(std::begin(filters), std::end(filters)), std::end(filters));
				auto cpy = filters;
				filters.clear();
				std::set_difference(
				  std::begin(cpy), std::end(cpy), std::begin(on_add), std::end(on_add), std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy),
									std::end(cpy),
									std::begin(on_remove),
									std::end(on_remove),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(
				  std::begin(cpy), std::end(cpy), std::begin(except), std::end(except), std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy),
									std::end(cpy),
									std::begin(on_combine),
									std::end(on_combine),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy),
									std::end(cpy),
									std::begin(on_break),
									std::end(on_break),
									std::back_inserter(filters));
			}


			// Is this fully containable in the other
			bool is_subset_of(const filter_group& other) const noexcept
			{
				return std::includes(
						 std::begin(other.filters), std::end(other.filters), std::begin(filters), std::end(filters)) &&
					   std::includes(
						 std::begin(other.on_add), std::end(other.on_add), std::begin(on_add), std::end(on_add)) &&
					   std::includes(std::begin(other.on_remove),
									 std::end(other.on_remove),
									 std::begin(on_remove),
									 std::end(on_remove)) &&
					   std::includes(
						 std::begin(other.except), std::end(other.except), std::begin(except), std::end(except)) &&
					   std::includes(std::begin(other.on_combine),
									 std::end(other.on_combine),
									 std::begin(on_combine),
									 std::end(on_combine)) &&
					   std::includes(std::begin(other.on_break),
									 std::end(other.on_break),
									 std::begin(on_break),
									 std::end(on_break));
			}

			// inverse of subset, does this fully contain the other
			bool is_superset_of(const filter_group& other) const noexcept { return other.is_subset_of(*this); }

			// neither superset or subset, but partial match
			bool is_divergent(const filter_group& other) const noexcept { return false; }

			bool clear_every_frame() const noexcept
			{
				return on_remove.size() > 0 || on_break.size() > 0 || on_combine.size() > 0 || on_add.size() > 0;
			}

			bool operator==(const filter_group& other) const noexcept
			{
				return std::equal(
						 std::begin(filters), std::end(filters), std::begin(other.filters), std::end(other.filters)) &&
					   std::equal(
						 std::begin(on_add), std::end(on_add), std::begin(other.on_add), std::end(other.on_add)) &&
					   std::equal(std::begin(on_remove),
								  std::end(on_remove),
								  std::begin(other.on_remove),
								  std::end(other.on_remove)) &&
					   std::equal(
						 std::begin(except), std::end(except), std::begin(other.except), std::end(other.except)) &&
					   std::equal(std::begin(on_combine),
								  std::end(on_combine),
								  std::begin(other.on_combine),
								  std::end(other.on_combine)) &&
					   std::equal(std::begin(on_break),
								  std::end(on_break),
								  std::begin(other.on_break),
								  std::end(other.on_break));
			}

		  private:
			friend class ::psl::ecs::state_t;

			void add_debug_system_name(psl::string_view name) { m_SystemsDebugNames.emplace_back(name); }

			psl::array<details::component_key_t> filters;
			psl::array<details::component_key_t> on_add;
			psl::array<details::component_key_t> on_remove;
			psl::array<details::component_key_t> except;
			psl::array<details::component_key_t> on_combine;
			psl::array<details::component_key_t> on_break;
			psl::array<psl::string_view> m_SystemsDebugNames;
		};

		template <typename... Ts>
		auto make_filter_group(psl::templates::type_container<psl::ecs::pack<Ts...>>)
		{
			return filter_group {psl::templates::type_container<Ts> {}...};
		}

		template <typename... Ts>
		auto make_filter_group(psl::templates::type_container<std::tuple<Ts...>>)
		{
			psl::array<filter_group> groups;
			groups.reserve(sizeof...(Ts));
			if constexpr(sizeof...(Ts) == 1)
			{
				groups.emplace_back(make_filter_group(psl::templates::type_container<Ts> {}...));
			}
			else
			{
				(void(groups.emplace_back(make_filter_group(psl::templates::type_container<Ts> {}))), ...);
			}
			return groups;
		}

		template <typename... Ts>
		auto make_transform_group(psl::templates::type_container<psl::ecs::pack<Ts...>>)
		{
			return transform_group {psl::templates::type_container<Ts> {}...};
		}

		template <typename... Ts>
		auto make_transform_group(psl::templates::type_container<std::tuple<Ts...>>)
		{
			psl::array<transform_group> groups;
			groups.reserve(sizeof...(Ts));
			if constexpr(sizeof...(Ts) == 1)
			{
				groups.emplace_back(make_transform_group(psl::templates::type_container<Ts> {}...));
			}
			else
			{
				(void(groups.emplace_back(make_transform_group(psl::templates::type_container<Ts> {}))), ...);
			}
			return groups;
		}
	}	 // namespace details
}	 // namespace psl::ecs