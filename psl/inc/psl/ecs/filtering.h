#pragma once
#include "selectors.h"
#include "details/component_key.h"
#include "psl/array.h"
#include "psl/template_utils.h"
#include "psl/ecs/pack.h"

namespace psl::ecs
{
	class state;
	namespace details
	{
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

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<order_by<Ts...>>) noexcept
			{}

			template <typename... Ts>
			constexpr void selector(psl::templates::type_container<on_condition<Ts...>>) noexcept
			{}
			friend class ::psl::ecs::state;
			filter_group() = default;
			filter_group(psl::array<component_key_t> filters_arr, psl::array<component_key_t> on_add_arr,
						 psl::array<component_key_t> on_remove_arr, psl::array<component_key_t> except_arr,
						 psl::array<component_key_t> on_combine_arr, psl::array<component_key_t> on_break_arr)
				: filters(filters_arr), on_add(on_add_arr), on_remove(on_remove_arr), except(except_arr),
				  on_combine(on_combine_arr), on_break(on_break_arr)
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
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_add), std::end(on_add),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_remove), std::end(on_remove),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(except), std::end(except),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_combine), std::end(on_combine),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_break), std::end(on_break),
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
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_add), std::end(on_add),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_remove), std::end(on_remove),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(except), std::end(except),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_combine), std::end(on_combine),
									std::back_inserter(filters));

				cpy = filters;
				filters.clear();
				std::set_difference(std::begin(cpy), std::end(cpy), std::begin(on_break), std::end(on_break),
									std::back_inserter(filters));
			}


			// Is this fully containable in the other
			bool is_subset_of(const filter_group& other) const noexcept
			{
				return std::includes(std::begin(other.filters), std::end(other.filters), std::begin(filters),
									 std::end(filters)) &&
					   std::includes(std::begin(other.on_add), std::end(other.on_add), std::begin(on_add),
									 std::end(on_add)) &&
					   std::includes(std::begin(other.on_remove), std::end(other.on_remove), std::begin(on_remove),
									 std::end(on_remove)) &&
					   std::includes(std::begin(other.except), std::end(other.except), std::begin(except),
									 std::end(except)) &&
					   std::includes(std::begin(other.on_combine), std::end(other.on_combine), std::begin(on_combine),
									 std::end(on_combine)) &&
					   std::includes(std::begin(other.on_break), std::end(other.on_break), std::begin(on_break),
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

		  private:
			friend class ::psl::ecs::state;
			using component_key_t = psl::ecs::details::component_key_t;

			psl::array<component_key_t> filters;
			psl::array<component_key_t> on_add;
			psl::array<component_key_t> on_remove;
			psl::array<component_key_t> except;
			psl::array<component_key_t> on_combine;
			psl::array<component_key_t> on_break;
		};

		template <typename... Ts>
		filter_group make_filter_group(psl::templates::type_container<psl::ecs::pack<Ts...>>)
		{
			return filter_group{psl::templates::type_container<Ts>{}...};
		}

		// template <typename... Ts>
		// auto make_filter_group(std::tuple<Ts...>)
		//{
		//	if constexpr (sizeof...(Ts) == 0)
		//	{

		//	}
		//	if constexpr(sizeof...(Ts) == 1)
		//	{
		//		//return filter_group{ psl::templates::type_container<Ts...>{} };
		//		return make_filter_group(psl::templates::type_container<Ts>{}...);
		//	}
		//	else
		//	{
		//		psl::array<filter_group> groups;
		//		(void(groups.emplace_back(make_filter_group(psl::templates::type_container<Ts>{}))), ...);
		//		return groups;
		//	}
		//}

		template <typename T>
		auto make_filter_group(T)
		{
			static_assert(false);
		}

		template <typename... Ts>
		auto make_filter_group(psl::templates::type_container<std::tuple<Ts...>>)
		{
			if constexpr(sizeof...(Ts) == 1)
			{
				return make_filter_group(psl::templates::type_container<Ts>{}...);
			}
			else
			{
				psl::array<filter_group> groups;
				(void(groups.emplace_back(make_filter_group(psl::templates::type_container<Ts>{}))), ...);
				return groups;
			}
		}
	} // namespace details
} // namespace psl::ecs