#pragma once
#include "pack_view.h"

namespace psl::ecs
{
	/// \brief tag that allows you to select entities (and components) that have recently added
	/// the given component type
	///
	/// You can use this tag to listen to the event of a specific component type being added to
	/// an entity.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_add
	{};

	/// \brief tag that allows you to listen to the event of a component of the given type being
	/// removed.
	///
	/// You can use this tag to listen to the event of a component of the given type being removed
	/// to an entity.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_remove
	{};

	/// \brief specialized tag of `on_add`
	///
	/// Will filter components based on when the given combination first appears.
	/// This provides a way to not care if component X, or component Y was added last
	/// and instead only cares if they both are used in a combination for the first time.
	/// This is ideal for systems that need to do something based on the creation of certain
	/// components. Take core::ecs::components::renderable and core::ecs::components::transform
	/// as example, who, when combined, create a draw call. The system that creates them should
	/// not need to care if the transform was added first or last.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_combine
	{};

	/// \brief specialized tag of `on_remove`
	///
	/// Similarly to the `on_combine` tag, the on_break that denotes a combination group, but
	/// instead of containing all recently combined components, it instead contains all those
	/// who recently broke connection. This can be ideal to use in a system (such as a render
	/// system), to erase draw calls.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_break
	{};

	/// \brief tag that disallows a certain component to be present on the given entity.
	///
	/// Sometimes you want to filter on all items, except a subgroup. This tag can aid in this.
	/// For example, if you had a debug system that would log an error for all entities that are
	/// renderable, but lacked a transform, then you could use the `except` tag to denote the filter
	/// what to do as a hint.
	/// Except directives are always executed last, regardless of order in the parameter pack.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct except
	{};



	/// \brief tag allows you to filter on sets of components
	///
	/// When you just want to create a generic filtering behaviour, without getting the components
	/// themselves, then you can use this tag to mark those requested components.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct filter
	{};


	namespace details
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


		template <typename T>
		struct is_exception : std::false_type
		{};

		template <typename... Ts>
		struct is_exception<except<Ts...>> : std::true_type
		{};

		template <typename T, typename Tuple>
		struct has_type;

		template <typename T>
		struct has_type<T, std::tuple<>> : std::false_type
		{};

		template <typename T, typename U, typename... Ts>
		struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>>
		{};

		template <typename T, typename... Ts>
		struct has_type<T, std::tuple<T, Ts...>> : std::true_type
		{};

		template <typename T, typename Tuple>
		using tuple_contains_type = typename has_type<T, Tuple>::type;

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

		template <typename T>
		struct extract_physical
		{
			using type = std::tuple<T>;
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
	} // namespace details
} // namespace psl::ecs