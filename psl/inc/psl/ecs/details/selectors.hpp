#pragma once
#include "../selectors.hpp"
#include "psl/ecs/details/mutate_instruction.hpp"
#include "psl/pack_view.hpp"

namespace psl::ecs::details {
template <typename T>
struct extract {
	using type = std::tuple<T>;
};

template <typename... Ts>
struct has_preseed : std::conditional_t<(std::is_same_v<Ts, preseed_tag> || ...), std::true_type, std::false_type> {};

template <typename... Ts>
struct has_preseed<on_add<Ts...>> : public has_preseed<Ts...> {};

template <typename... Ts>
struct has_preseed<on_combine<Ts...>> : public has_preseed<Ts...> {};

template <typename... Ts>
concept HasPreseedTag = has_preseed<Ts...>::value;

template <typename T>
concept IsPreseedTag = std::is_same_v<T, preseed_tag>;

template <typename T>
struct extract_add {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_add<on_add<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename T>
struct extract_remove {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_remove<on_remove<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename T>
struct extract_except {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_except<except<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename Pred, typename... Ts>
struct extract_conditional {
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct extract_conditional<on_condition<Pred, Ts...>> {
	using type = std::tuple<std::pair<Pred, std::tuple<Ts...>>>;
};

template <typename Pred, typename... Ts>
struct extract_orderby {
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct extract_orderby<order_by<Pred, Ts...>> {
	using type = std::tuple<std::pair<Pred, std::tuple<Ts...>>>;
};

template <typename T>
struct extract_on_mutate {
	using type = std::tuple<>;
};

template <typename T>
struct extract_on_mutate<on_mutate<T>> {
	using type = std::tuple<const details::mutate_instruction_t<T>>;
};

template <typename T>
struct extract_physical {
	using type = std::tuple<T>;
};

template <IsEntityFilteringOp T>
struct extract_physical<T> {
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct extract_physical<on_condition<Pred, Ts...>> {
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct extract_physical<order_by<Pred, Ts...>> {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<filter<Ts...>> {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<on_add<Ts...>> {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<on_remove<Ts...>> {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<except<Ts...>> {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<on_combine<Ts...>> {
	using type = std::tuple<>;
};


template <typename... Ts>
struct extract_physical<on_break<Ts...>> {
	using type = std::tuple<>;
};

template <typename T>
struct extract_physical<on_mutate<T>> {
	using type = std::tuple<const details::mutate_instruction_t<T>>;
};

template <typename T>
struct extract_combine {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_combine<on_combine<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename T>
struct extract_break {
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_break<on_break<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename T>
struct decode_type {
	using type = std::tuple<T>;
};

template <IsEntityFilteringOp T>
struct decode_type<T> {
	using type = std::tuple<>;
};

template <typename... Ts>
struct decode_type<on_add<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<on_remove<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<except<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<on_combine<Ts...>> {
	using type = std::tuple<Ts...>;
};


template <typename... Ts>
struct decode_type<on_break<Ts...>> {
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<filter<Ts...>> {
	using type = std::tuple<Ts...>;
};


template <typename Pred, typename... Ts>
struct decode_type<order_by<Pred, Ts...>> {
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct decode_type<on_condition<Pred, Ts...>> {
	using type = std::tuple<>;
};

template <typename T>
struct decode_type<on_mutate<T>> {
	using type = std::tuple<const details::mutate_instruction_t<T>>;
};

template <typename... Ts>
struct typelist_to_tuple {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_physical<Ts>::type>()...));
};

template <typename... Ts>
struct tuple_to_pack_view {
	using type = void;
};


template <typename... Ts>
struct tuple_to_pack_view<std::tuple<Ts...>> {
	using type = psl::pack_view<Ts...>;
};

template <typename... Ts>
struct typelist_to_pack_view {
	using type = typename tuple_to_pack_view<typename typelist_to_tuple<Ts...>::type>::type;
};

template <typename... Ts>
struct typelist_to_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::decode_type<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_physical_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_physical<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_add_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_add<Ts>::type>()...));
};


template <typename... Ts>
struct typelist_to_combine_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_combine<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_except_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_except<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_conditional_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_conditional<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_orderby_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_orderby<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_break_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_break<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_remove_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_remove<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_on_mutate_pack {
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_on_mutate<Ts>::type>()...));
};

template <typename... Ts>
struct wrap_with_array_view {
	using type = std::tuple<psl::array_view<Ts>...>;
};

template <typename... Ts>
struct wrap_with_array_view<std::tuple<Ts...>> {
	using type = std::tuple<psl::array_view<Ts>...>;
};
namespace {
	template <typename T>
	struct is_on_add_t : std::false_type {};
	template <typename... Ts>
	struct is_on_add_t<on_add<Ts...>> : std::true_type {};

	template <typename T>
	struct is_on_remove_t : std::false_type {};
	template <typename... Ts>
	struct is_on_remove_t<on_remove<Ts...>> : std::true_type {};

	template <typename T>
	struct is_on_combine_t : std::false_type {};
	template <typename... Ts>
	struct is_on_combine_t<on_combine<Ts...>> : std::true_type {};

	template <typename T>
	struct is_on_break_t : std::false_type {};
	template <typename... Ts>
	struct is_on_break_t<on_break<Ts...>> : std::true_type {};

	template <typename T>
	struct is_on_mutate_t : std::false_type {};
	template <typename T>
	struct is_on_mutate_t<on_mutate<T>> : std::true_type {};

	template <typename T>
	struct is_on_condition_t : std::false_type {};
	template <typename Pred, typename... Ts>
	struct is_on_condition_t<on_condition<Pred, Ts...>> : std::true_type {};

	template <typename T>
	struct is_order_by_t : std::false_type {};
	template <typename Pred, typename... Ts>
	struct is_order_by_t<order_by<Pred, Ts...>> : std::true_type {};

	template <typename T>
	struct is_get_relationship_t : std::false_type {};
	template <entity_relationship Relationship>
	struct is_get_relationship_t<get_relationship<Relationship>> : std::true_type {};

	template <typename T>
	struct is_hierarchy_change_t : std::false_type {};
	template <hierarchy_change_event Change, typename T>
	struct is_hierarchy_change_t<on_hierarchy_change<Change, T>> : std::true_type {};


	template <typename T>
	struct is_filter_t : std::true_type {};

	template <typename T>
		requires(!is_on_add_t<T>::value && !is_on_remove_t<T>::value && !is_on_combine_t<T>::value &&
				 !is_on_break_t<T>::value && !is_on_mutate_t<T>::value && !is_on_condition_t<T>::value &&
				 !is_order_by_t<T>::value && !is_get_relationship_t<T>::value && !is_hierarchy_change_t<T>::value)
	struct is_filter_t<T> : std::false_type {};
	template <typename... Ts>
	struct is_filter_t<filter<Ts...>> : std::true_type {};
}	 // namespace

template <typename T>
concept IsOnAdd = is_on_add_t<T>::value;
template <typename T>
concept IsOnRemove = is_on_remove_t<T>::value;
template <typename T>
concept IsOnCombine = is_on_combine_t<T>::value;
template <typename T>
concept IsOnBreak = is_on_break_t<T>::value;
template <typename T>
concept IsOnMutate = is_on_mutate_t<T>::value;
template <typename T>
concept IsOnCondition = is_on_condition_t<T>::value;
template <typename T>
concept IsOrderBy = is_order_by_t<T>::value;
template <typename T>
concept IsFilter = is_filter_t<T>::value;
template <typename T>
concept IsGetRelationship = is_get_relationship_t<T>::value;
template <typename T>
concept IsHierarchyChange = is_hierarchy_change_t<T>::value;

template <typename T>
concept IsSimpleFilteringOp = !IsOnAdd<T> && !IsOnRemove<T> && !IsOnCombine<T> && !IsOnBreak<T> && !IsOnMutate<T> &&
							  !IsOnCondition<T> && !IsHierarchyChange<T>;
}	 // namespace psl::ecs::details
