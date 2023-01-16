#pragma once
#include "details/component_key.hpp"
#include "details/execution.hpp"
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
			psl_assert(target_key == target_container->id(), "ID did not match");
		};

		constexpr cached_container_entry_t(cached_container_entry_t const&)			   = default;
		constexpr cached_container_entry_t(cached_container_entry_t&&)				   = default;
		constexpr cached_container_entry_t& operator=(cached_container_entry_t const&) = default;
		constexpr cached_container_entry_t& operator=(cached_container_entry_t&&)	   = default;

		constexpr operator details::component_key_t const&() const noexcept { return key; }
		constexpr operator details::component_key_t&() noexcept { return key; }

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

		operator bool() const noexcept { return order_by || on_condition.size() > 0; }

	  private:
		friend class ::psl::ecs::state_t;
		void add_debug_system_name(psl::string_view name) { m_SystemsDebugNames.emplace_back(name); }
		std::function<ordering_pred_t> order_by;

		psl::array<std::function<conditional_pred_t>> on_condition;
		psl::array<psl::string_view> m_SystemsDebugNames;
	};


	class filter_group {
		template <typename T, typename Fn>
		constexpr void selector(psl::type_pack_t<T>, Fn&& query) noexcept {
			if constexpr(!std::is_same_v<entity_t, T> && !IsPolicy<T> && !IsAccessType<T>)
				filters.emplace_back(details::component_key_t::generate<T>(), query.template operator()<T>());
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<filter<Ts...>>, Fn&& query) noexcept {
			(selector(psl::type_pack_t<Ts>(), query), ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_combine<Ts...>>, Fn&& query) noexcept {
			(void(on_combine.emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_break<Ts...>>, Fn&& query) noexcept {
			(void(on_break.emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<except<Ts...>>, Fn&& query) noexcept {
			(void(except.emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())), ...);
		}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_add<Ts...>>, Fn&& query) noexcept {
			(void(on_add.emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())), ...);
		}


		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_remove<Ts...>>, Fn&& query) noexcept {
			(void(on_remove.emplace_back(details::component_key_t::generate<Ts>(), query.template operator()<Ts>())),
			 ...);
		}

		template <typename Pred, typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<order_by<Pred, Ts...>>, Fn&&) noexcept {}

		template <typename... Ts, typename Fn>
		constexpr void selector(psl::type_pack_t<on_condition<Ts...>>, Fn&&) noexcept {}

		friend class ::psl::ecs::state_t;
		filter_group() = default;
		filter_group(psl::array<cached_container_entry_t> filters_arr,
					 psl::array<cached_container_entry_t> on_add_arr,
					 psl::array<cached_container_entry_t> on_remove_arr,
					 psl::array<cached_container_entry_t> except_arr,
					 psl::array<cached_container_entry_t> on_combine_arr,
					 psl::array<cached_container_entry_t> on_break_arr)
			: filters(filters_arr.begin(), filters_arr.end()), on_add(on_add_arr.begin(), on_add_arr.end()),
			  on_remove(on_remove_arr.begin(), on_remove_arr.end()), except(except_arr.begin(), except_arr.end()),
			  on_combine(on_combine_arr.begin(), on_combine_arr.end()),
			  on_break(on_break_arr.begin(), on_break_arr.end()) {
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
			std::set_difference(
			  std::begin(cpy), std::end(cpy), std::begin(on_remove), std::end(on_remove), std::back_inserter(filters));

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
			std::set_difference(
			  std::begin(cpy), std::end(cpy), std::begin(on_break), std::end(on_break), std::back_inserter(filters));
		};

	  public:
		template <typename... Ts, typename Fn>
		filter_group(psl::type_pack_t<Ts...>, Fn&& query) {
			(void(selector(psl::type_pack_t<Ts>(), query)), ...);
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
			std::set_difference(
			  std::begin(cpy), std::end(cpy), std::begin(on_remove), std::end(on_remove), std::back_inserter(filters));

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
			std::set_difference(
			  std::begin(cpy), std::end(cpy), std::begin(on_break), std::end(on_break), std::back_inserter(filters));
		}

		// Is this fully containable in the other
		bool is_subset_of(const filter_group& other) const noexcept {
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
				   std::includes(
					 std::begin(other.on_break), std::end(other.on_break), std::begin(on_break), std::end(on_break));
		}

		// inverse of subset, does this fully contain the other
		bool is_superset_of(const filter_group& other) const noexcept { return other.is_subset_of(*this); }

		// neither superset or subset, but partial match
		bool is_divergent(const filter_group& other) const noexcept { return false; }

		bool clear_every_frame() const noexcept {
			return on_remove.size() > 0 || on_break.size() > 0 || on_combine.size() > 0 || on_add.size() > 0;
		}

		bool operator==(const filter_group& other) const noexcept {
			return std::equal(
					 std::begin(filters), std::end(filters), std::begin(other.filters), std::end(other.filters)) &&
				   std::equal(std::begin(on_add), std::end(on_add), std::begin(other.on_add), std::end(other.on_add)) &&
				   std::equal(std::begin(on_remove),
							  std::end(on_remove),
							  std::begin(other.on_remove),
							  std::end(other.on_remove)) &&
				   std::equal(std::begin(except), std::end(except), std::begin(other.except), std::end(other.except)) &&
				   std::equal(std::begin(on_combine),
							  std::end(on_combine),
							  std::begin(other.on_combine),
							  std::end(other.on_combine)) &&
				   std::equal(
					 std::begin(on_break), std::end(on_break), std::begin(other.on_break), std::end(other.on_break));
		}

	  private:
		friend class ::psl::ecs::state_t;

		void add_debug_system_name(psl::string_view name) { m_SystemsDebugNames.emplace_back(name); }

		psl::array<cached_container_entry_t> filters;
		psl::array<cached_container_entry_t> on_add;
		psl::array<cached_container_entry_t> on_remove;
		psl::array<cached_container_entry_t> except;
		psl::array<cached_container_entry_t> on_combine;
		psl::array<cached_container_entry_t> on_break;
		psl::array<psl::string_view> m_SystemsDebugNames;
	};
}	 // namespace details
}	 // namespace psl::ecs
