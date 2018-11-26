#pragma once
#include "stdafx.h"
#include "ecs/entity.h"
#include "ecs/range.h"


namespace core::ecs
{
	template<typename T>
	struct on_add {};
	template<typename T>
	struct on_remove {};

	/// \brief specialized tag of `on_add`
	///
	/// Will filter components based on when the given combination first appears.
	/// This provides a way to not care if component X, or component Y was added last
	/// and instead only cares if they both are used in a combination for the first time.
	/// This is ideal for systems that need to do something based on the creation of certain
	/// components. Take core::ecs::components::renderable and core::ecs::components::transform 
	/// as example, who, when combined, create a draw call. The system that creates them should
	/// not need to care if the transform was added first or last.
	template<typename... Ts>
	struct on_combine {};

	/// \brief specialized tag of `on_remove`
	///
	/// Similarly to the `on_combine` tag, the on_break that denotes a combination group, but
	/// instead of containing all recently combined components, it instead contains all those
	/// who recently broke connection. This can be ideal to use in a system (such as a render
	/// system), to erase draw calls.
	template<typename... Ts>
	struct on_break {};

	/// \brief tag that disallows a certain component to be present.
	///
	/// Sometimes you want to filter on all items, except a subgroup. This tag can aid in this.
	/// For example, if you had a debug system that would log an error for all entities that are
	/// renderable, but lacked a transform, then you could use the `except` tag to denote the filter
	/// what to do as a hint.
	template<typename T>
	struct except {};


	namespace details
	{
		template<bool has_entities, typename... Ts>
		struct pack_tuple
		{
			using range_t = std::tuple<psl::array_view<const entity>, psl::array_view<Ts>...>;
			using range_element_t = std::tuple<entity, Ts...>;
		};

		template<typename... Ts>
		struct pack_tuple<false, Ts...>
		{
			using range_t = std::tuple<psl::array_view<Ts>...>;
			using range_element_t = std::tuple<Ts...>;
		};

		template <typename T, typename Tuple>
		struct has_type;

		template <typename T>
		struct has_type<T, std::tuple<>> : std::false_type {};

		template <typename T, typename U, typename... Ts>
		struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>> {};

		template <typename T, typename... Ts>
		struct has_type<T, std::tuple<T, Ts...>> : std::true_type {};

		template <typename T, typename Tuple>
		using tuple_contains_type = typename has_type<T, Tuple>::type;
	}


	template <typename... Ts>
	class pack
	{
		using range_t = std::tuple<psl::array_view<Ts>...>;
		using range_element_t = std::tuple<Ts...>;
		using iterator_element_t = std::tuple<typename psl::array_view<Ts>::iterator...>;

	public:
		class iterator
		{
			template <std::size_t...Is, typename T>
			auto init_impl(std::index_sequence<Is...>, const T& t)
			{
				return std::make_tuple(std::begin(std::get<Is>(t))...);
			}

			template <std::size_t...Is>
			auto pack_impl(std::index_sequence<Is...>, const iterator_element_t& t)
			{
				return std::make_tuple(*std::get<Is>(t)...);
			}

			template <std::size_t...Is>
			const auto cpack_impl(std::index_sequence<Is...>, const iterator_element_t& t) const
			{
				return std::make_tuple(*std::get<Is>(t)...);
			}

			template <std::size_t...Is>
			auto increment(std::index_sequence<Is...>)
			{
				return (++std::get<Is>(data), ...);
			}
		public:
			constexpr iterator(const range_t& range) noexcept : data(init_impl(std::index_sequence_for<Ts...>{}, range))
			{

			};
			constexpr iterator(iterator_element_t data) noexcept : data(data) {};

			constexpr iterator operator++() const noexcept
			{
				auto next = iterator(data);
				++next;
				return next;
			}
			constexpr iterator& operator++() noexcept
			{
				increment(std::index_sequence_for<Ts...>{});
				return *this;
			}

			bool operator!=(const iterator &other) const noexcept { return std::get<0>(data) != std::get<0>(other.data); }

			bool operator==(const iterator &other) const noexcept { return std::get<0>(data) == std::get<0>(other.data); }

			auto operator*() -> decltype(pack_impl(std::index_sequence_for<Ts...>{}, std::declval<iterator_element_t>()))
			{
				return pack_impl(std::index_sequence_for<Ts...>{}, data);
			}
			auto operator*() const  -> decltype(cpack_impl(std::index_sequence_for<Ts...>{}, std::declval<iterator_element_t>()))
			{
				return cpack_impl(std::index_sequence_for<Ts...>{}, data);
			}

			template<typename T>
			auto get()	-> decltype(*std::get<typename psl::array_view<T>::iterator>(std::declval<iterator_element_t>()))
			{
				static_assert(details::tuple_contains_type<psl::array_view<T>, range_t>::value, "the requested component type does not exist in the pack");
				return *std::get<typename psl::array_view<T>::iterator>(data);
			}

			template <size_t N>
			auto get() const noexcept	-> decltype(*std::get<N>(std::declval<iterator_element_t>()))
			{
				static_assert(N < std::tuple_size<range_t>::value, "you requested a component outside of the range of the pack");
				return *std::get<N>(data);
			}
		private:
			iterator_element_t data;
		};
		range_t read() { return m_Pack; }

		template <typename T>
		psl::array_view<T> get() const noexcept
		{
			static_assert(details::tuple_contains_type<psl::array_view<T>, range_t>::value, "the requested component type does not exist in the pack");
			return std::get<psl::array_view<T>>(m_Pack);
		}

		template <size_t N>
		auto get() const noexcept	-> decltype(std::get<N>(std::declval<range_t>()))
		{
			static_assert(N < std::tuple_size<range_t>::value, "you requested a component outside of the range of the pack");
			return std::get<N>(m_Pack);
		}

		iterator begin() const noexcept { return iterator{m_Pack}; }

		iterator end() const noexcept { return iterator{m_Pack}; }
		constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }

	private:
		range_t m_Pack;
	};
}