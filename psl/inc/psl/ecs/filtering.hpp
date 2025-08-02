#pragma once
#include "details/component_key.hpp"
#include "details/execution.hpp"
#include "details/mutate_instruction.hpp"
#include "psl/array.hpp"
#include "psl/ecs/pack.hpp"
#include "psl/template_utils.hpp"
#include "psl/ustring.hpp"
#include "selectors.hpp"

namespace psl::ecs {
class state_t;
namespace details {
	struct cached_container_entry_t {
		constexpr cached_container_entry_t(const details::component_key_t& target) : key(target), container(nullptr) {};
		constexpr cached_container_entry_t(details::component_container_t* target)
			: key(target->id()), container(target) {};
		constexpr cached_container_entry_t(const details::component_key_t& target_key,
										   details::component_container_t* target_container)
			: key(target_key), container(target_container) {
			psl_assert(target_container == nullptr || target_key == target_container->id(),
					   "ID did not match. Was expecting '{}' but got '{}'",
					   target_key.name(),
					   target_container->id().name());
		};
		// specialized form for the on_mutate, as it masquerades as the container of another type.
		constexpr cached_container_entry_t(const details::component_key_t& target_key,
										   const details::component_key_t& container_key,
										   details::component_container_t* target_container)
			: key(target_key), container(target_container) {
			psl_assert(target_container == nullptr || container_key == target_container->id(),
					   "ID did not match. Was expecting '{}' but got '{}'",
					   container_key.name(),
					   target_container->id().name());
		};

		constexpr cached_container_entry_t(cached_container_entry_t const&)			   = default;
		constexpr cached_container_entry_t(cached_container_entry_t&&)				   = default;
		constexpr cached_container_entry_t& operator=(cached_container_entry_t const&) = default;
		constexpr cached_container_entry_t& operator=(cached_container_entry_t&&)	   = default;

		constexpr operator details::component_key_t const&() const noexcept {
			return key;
		}
		constexpr operator details::component_key_t&() noexcept {
			return key;
		}

		friend constexpr bool operator==(cached_container_entry_t const& lhs,
										 cached_container_entry_t const& rhs) noexcept {
			return lhs.key == rhs.key;
		}
		friend constexpr bool operator!=(cached_container_entry_t const& lhs,
										 cached_container_entry_t const& rhs) noexcept {
			return lhs.key != rhs.key;
		}
		friend constexpr bool operator<=(cached_container_entry_t const& lhs,
										 cached_container_entry_t const& rhs) noexcept {
			return lhs.key <= rhs.key;
		}
		friend constexpr bool operator>=(cached_container_entry_t const& lhs,
										 cached_container_entry_t const& rhs) noexcept {
			return lhs.key >= rhs.key;
		}
		friend constexpr bool operator<(cached_container_entry_t const& lhs,
										cached_container_entry_t const& rhs) noexcept {
			return lhs.key < rhs.key;
		}
		friend constexpr bool operator>(cached_container_entry_t const& lhs,
										cached_container_entry_t const& rhs) noexcept {
			return lhs.key > rhs.key;
		}

		friend constexpr bool operator==(cached_container_entry_t const& lhs,
										 details::component_key_t const& rhs) noexcept {
			return lhs.key == rhs;
		}
		friend constexpr bool operator!=(cached_container_entry_t const& lhs,
										 details::component_key_t const& rhs) noexcept {
			return lhs.key != rhs;
		}
		friend constexpr bool operator<=(cached_container_entry_t const& lhs,
										 details::component_key_t const& rhs) noexcept {
			return lhs.key <= rhs;
		}
		friend constexpr bool operator>=(cached_container_entry_t const& lhs,
										 details::component_key_t const& rhs) noexcept {
			return lhs.key >= rhs;
		}
		friend constexpr bool operator<(cached_container_entry_t const& lhs,
										details::component_key_t const& rhs) noexcept {
			return lhs.key < rhs;
		}
		friend constexpr bool operator>(cached_container_entry_t const& lhs,
										details::component_key_t const& rhs) noexcept {
			return lhs.key > rhs;
		}

		details::component_key_t key {};
		details::component_container_t* container {nullptr};
	};

	// unlike filter_groups, transform groups are dynamic operations on every element of a filtered list
	class transform_group {
		using ordering_pred_t	 = void(psl::array<entity_t>::iterator,
										psl::array<entity_t>::iterator,
										const psl::ecs::state_t&);
		using conditional_pred_t = psl::array<entity_t>::iterator(psl::array<entity_t>::iterator,
																  psl::array<entity_t>::iterator,
																  const psl::ecs::state_t&);
		template <typename T>
		constexpr void selector(psl::type_pack_t<T>) noexcept {}

		// implementation lives in `psl/ecs/order_by.hpp`
		template <typename Pred, typename T>
		void selector(psl::type_pack_t<order_by<Pred, T>>) noexcept;

		// implementation lives in `psl/ecs/on_condition.hpp`
		template <typename Pred, typename T>
		void selector(psl::type_pack_t<on_condition<Pred, T>>) noexcept;

	  public:
		template <typename... Ts>
		transform_group(psl::type_pack_t<Ts...>) {
			(void(selector(psl::type_pack_t<Ts>())), ...);
		};
		~transform_group() = default;

		transform_group(const transform_group& other)				 = default;
		transform_group(transform_group&& other) noexcept			 = default;
		transform_group& operator=(const transform_group& other)	 = default;
		transform_group& operator=(transform_group&& other) noexcept = default;

		psl::array<entity_t>::iterator transform(psl::array<entity_t>::iterator begin,
												 psl::array<entity_t>::iterator end,
												 const state_t& state) const noexcept {
			for(const auto& condition : on_condition) end = condition(begin, end, state);

			if(order_by)
				order_by(begin, end, state);
			return end;
		}

		operator bool() const noexcept {
			return order_by || on_condition.size() > 0;
		}

	  private:
		friend class ::psl::ecs::state_t;
		void add_debug_system_name(psl::string_view name) {
			m_SystemsDebugNames.emplace_back(name);
		}
		std::function<ordering_pred_t> order_by;

		psl::array<std::function<conditional_pred_t>> on_condition;
		psl::array<psl::string_view> m_SystemsDebugNames;
	};


	class filter_group {
		friend class ::psl::ecs::state_t;

		struct filter_group_container_t {
			filter_group_container_t() = default;
			filter_group_container_t(const psl::array<cached_container_entry_t>& other) : group(other) {};

			void sort() noexcept {
				std::sort(std::begin(group), std::end(group));
			}

			void unique() noexcept {
				group.erase(std::unique(std::begin(group), std::end(group)), std::end(group));
			}

			void clear() noexcept {
				group.clear();
			}

			void set_difference(const filter_group_container_t& other) noexcept {
				psl::array<cached_container_entry_t> cpy = group;
				group.clear();
				std::set_difference(std::begin(cpy),
									std::end(cpy),
									std::begin(other.group),
									std::end(other.group),
									std::back_inserter(group));
			}

			bool includes(const filter_group_container_t& other) const noexcept {
				return std::includes(
				  std::begin(group), std::end(group), std::begin(other.group), std::end(other.group));
			}

			bool operator==(const filter_group_container_t& other) const noexcept {
				return std::equal(std::begin(group), std::end(group), std::begin(other.group), std::end(other.group));
			}

			size_t size() const noexcept {
				return group.size();
			}

			operator psl::array<cached_container_entry_t>&() noexcept {
				return group;
			}
			operator const psl::array<cached_container_entry_t>&() const noexcept {
				return group;
			}

			auto begin() noexcept {
				return std::begin(group);
			}

			auto end() noexcept {
				return std::end(group);
			}

			auto begin() const noexcept {
				return std::begin(group);
			}

			auto end() const noexcept {
				return std::end(group);
			}

			auto operator->() noexcept {
				return &group;
			}

			auto operator->() const noexcept {
				return &group;
			}

			psl::array<cached_container_entry_t> group {};
		};

		template <typename T, typename Fn>
		constexpr void selector(psl::type_pack_t<T>, Fn&& query) noexcept {
			if constexpr(!std::is_same_v<entity_t, T> && !IsPolicy<T> && !IsAccessType<T>) {
				static_assert(IsUnrestrictedMutable<T> || std::is_const_v<T>,
							  "Only mutable components (default) or read-only access is allowed.");
				filters->emplace_back(details::component_key_t::generate<T>(), query.template operator()<T>());
			}
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<filter<Ts...>>, Fn&& query) noexcept {
			(selector(psl::type_pack_t<Ts>(), query), ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_combine<Ts...>>, Fn&& query) noexcept {
			(void(on_combine->emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_break<Ts...>>, Fn&& query) noexcept {
			(void(on_break->emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<except<Ts...>>, Fn&& query) noexcept {
			(void(except->emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_add<Ts...>>, Fn&& query) noexcept {
			(void(on_add->emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}


		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_remove<Ts...>>, Fn&& query) noexcept {
			(void(on_remove->emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}

		template <typename T, typename Fn>
		constexpr void selector(psl::type_pack_t<on_mutate<T>>, Fn&& query) noexcept {
			on_mutate->emplace_back(details::component_key_t::generate<details::mutate_instruction_t<T>>(),
									details::component_key_t::generate<T>(),
									query.template operator()<details::mutate_instruction_t<T>>());
		}

		template <typename Pred, typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<order_by<Pred, Ts...>>, Fn&&) noexcept {}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_condition<Ts...>>, Fn&&) noexcept {}

		filter_group() = default;
		filter_group(psl::array<cached_container_entry_t> filters_arr,
					 psl::array<cached_container_entry_t> on_add_arr,
					 psl::array<cached_container_entry_t> on_remove_arr,
					 psl::array<cached_container_entry_t> except_arr,
					 psl::array<cached_container_entry_t> on_combine_arr,
					 psl::array<cached_container_entry_t> on_break_arr)
			: filters(filters_arr), on_add(on_add_arr), on_remove(on_remove_arr), except(except_arr),
			  on_combine(on_combine_arr), on_break(on_break_arr) {
			post_init();
		};

		void post_init() {
			filters.sort();
			on_add.sort();
			on_remove.sort();
			except.sort();
			on_combine.sort();
			on_break.sort();
			on_mutate.sort();

			filters.unique();

			filters.set_difference(on_add);
			filters.set_difference(on_remove);
			filters.set_difference(except);
			filters.set_difference(on_combine);
			filters.set_difference(on_break);
			filters.set_difference(on_mutate);
		}

	  public:
		template <typename... Ts, typename Fn>
		filter_group(psl::type_pack_t<Ts...>, Fn&& query) {
			(void(selector(psl::type_pack_t<Ts>(), query)), ...);
			post_init();
		}

		// Is this fully containable in the other
		bool is_subset_of(const filter_group& other) const noexcept {
			return other.filters.includes(filters) && other.on_add.includes(on_add) &&
				   other.on_remove.includes(on_remove) && other.except.includes(except) &&
				   other.on_combine.includes(on_combine) && other.on_break.includes(on_break) &&
				   other.on_mutate.includes(on_mutate);
		}

		// inverse of subset, does this fully contain the other
		bool is_superset_of(const filter_group& other) const noexcept {
			return other.is_subset_of(*this);
		}

		// neither superset or subset, but partial match
		bool is_divergent(const filter_group& other) const noexcept {
			return false;
		}

		bool clear_every_frame() const noexcept {
			return on_remove.size() > 0 || on_break.size() > 0 || on_combine.size() > 0 || on_add.size() > 0 ||
				   on_mutate.size() > 0;
		}

		bool operator==(const filter_group& other) const noexcept {
			return filters == other.filters && on_add == other.on_add && on_remove == other.on_remove &&
				   except == other.except && on_combine == other.on_combine && on_break == other.on_break &&
				   on_mutate == other.on_mutate;
		}

		// returns true if this filter is a basic filter, meaning it only filters on components, not special events
		// such as on_add, on_remove, on_combine, on_break
		bool is_basic_filter() const noexcept {
			return on_add.size() == 0 && on_remove.size() == 0 && on_combine.size() == 0 && on_break.size() == 0;
		}

	  private:
		friend class ::psl::ecs::state_t;

		void add_debug_system_name(psl::string_view name) {
			m_SystemsDebugNames.emplace_back(name);
		}

		filter_group_container_t filters;
		filter_group_container_t on_add;
		filter_group_container_t on_remove;
		filter_group_container_t except;
		filter_group_container_t on_combine;
		filter_group_container_t on_break;
		filter_group_container_t on_mutate;
		psl::array<psl::string_view> m_SystemsDebugNames;
	};
}	 // namespace details
}	 // namespace psl::ecs
