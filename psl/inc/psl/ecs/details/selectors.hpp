#pragma once
#include "../selectors.hpp"
#include "psl/pack_view.hpp"

namespace psl::ecs::details
{
template <typename T>
struct is_selector : std::false_type
{};

template <typename... Ts>
struct is_selector<on_add<Ts...>> : std::true_type
{};

template <typename... Ts>
struct is_selector<on_remove<Ts...>> : std::true_type
{};

template <typename... Ts>
struct is_selector<filter<Ts...>> : std::true_type
{};

template <typename... Ts>
struct is_selector<except<Ts...>> : std::true_type
{};

template <typename... Ts>
struct is_selector<on_break<Ts...>> : std::true_type
{};

template <typename... Ts>
struct is_selector<on_combine<Ts...>> : std::true_type
{};

template <typename Pred, typename... Ts>
struct is_selector<on_condition<Pred, Ts...>> : std::true_type
{};

template <typename T>
struct is_exception : std::false_type
{};

template <typename... Ts>
struct is_exception<except<Ts...>> : std::true_type
{};

template <typename T>
struct extract
{
	using type = std::tuple<T>;
};

template <typename T>
struct extract_add
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_add<on_add<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename T>
struct extract_remove
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_remove<on_remove<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename T>
struct extract_except
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_except<except<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename Pred, typename... Ts>
struct extract_conditional
{
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct extract_conditional<on_condition<Pred, Ts...>>
{
	using type = std::tuple<std::pair<Pred, std::tuple<Ts...>>>;
};

template <typename Pred, typename... Ts>
struct extract_orderby
{
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct extract_orderby<order_by<Pred, Ts...>>
{
	using type = std::tuple<std::pair<Pred, std::tuple<Ts...>>>;
};

template <typename T>
struct extract_physical
{
	using type = std::tuple<T>;
};

template <typename Pred, typename... Ts>
struct extract_physical<on_condition<Pred, Ts...>>
{
	using type = std::tuple<>;
};

template <typename Pred, typename... Ts>
struct extract_physical<order_by<Pred, Ts...>>
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<filter<Ts...>>
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<on_add<Ts...>>
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<on_remove<Ts...>>
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<except<Ts...>>
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_physical<on_combine<Ts...>>
{
	using type = std::tuple<>;
};


template <typename... Ts>
struct extract_physical<on_break<Ts...>>
{
	using type = std::tuple<>;
};

template <typename T>
struct extract_combine
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_combine<on_combine<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename T>
struct extract_break
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct extract_break<on_break<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename T>
struct decode_type
{
	using type = std::tuple<T>;
};

template <typename... Ts>
struct decode_type<on_add<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<on_remove<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<except<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<on_combine<Ts...>>
{
	using type = std::tuple<Ts...>;
};


template <typename... Ts>
struct decode_type<on_break<Ts...>>
{
	using type = std::tuple<Ts...>;
};

template <typename... Ts>
struct decode_type<filter<Ts...>>
{
	using type = std::tuple<Ts...>;
};


template <typename Pred, typename... Ts>
struct decode_type<order_by<Pred, Ts...>>
{
	using type = std::tuple<>;
};


template <typename Pred, typename... Ts>
struct decode_type<on_condition<Pred, Ts...>>
{
	using type = std::tuple<>;
};

template <typename... Ts>
struct typelist_to_tuple
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_physical<Ts>::type>()...));
};

template <typename... Ts>
struct tuple_to_pack_view
{
	using type = void;
};


template <typename... Ts>
struct tuple_to_pack_view<std::tuple<Ts...>>
{
	using type = psl::pack_view<Ts...>;
};

template <typename... Ts>
struct typelist_to_pack_view
{
	using type = typename tuple_to_pack_view<typename typelist_to_tuple<Ts...>::type>::type;
};

template <typename... Ts>
struct typelist_to_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::decode_type<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_physical_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_physical<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_add_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_add<Ts>::type>()...));
};


template <typename... Ts>
struct typelist_to_combine_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_combine<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_except_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_except<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_conditional_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_conditional<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_orderby_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_orderby<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_break_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_break<Ts>::type>()...));
};

template <typename... Ts>
struct typelist_to_remove_pack
{
	using type = decltype(std::tuple_cat(std::declval<typename details::extract_remove<Ts>::type>()...));
};


template <typename... Ts>
struct wrap_with_array_view
{
	using type = std::tuple<psl::array_view<Ts>...>;
};

template <typename... Ts>
struct wrap_with_array_view<std::tuple<Ts...>>
{
	using type = std::tuple<psl::array_view<Ts>...>;
};
}	 // namespace psl::ecs::details