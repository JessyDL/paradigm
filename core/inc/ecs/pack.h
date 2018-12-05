#pragma once
#include "stdafx.h"
#include "ecs/entity.h"
#include "ecs/range.h"


namespace core::ecs
{
	template <typename T>
	struct on_add
	{};
	template <typename T>
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
	template <typename... Ts>
	struct on_combine
	{};

	/// \brief specialized tag of `on_remove`
	///
	/// Similarly to the `on_combine` tag, the on_break that denotes a combination group, but
	/// instead of containing all recently combined components, it instead contains all those
	/// who recently broke connection. This can be ideal to use in a system (such as a render
	/// system), to erase draw calls.
	template <typename... Ts>
	struct on_break
	{};

	/// \brief tag that disallows a certain component to be present on the given entity.
	///
	/// Sometimes you want to filter on all items, except a subgroup. This tag can aid in this.
	/// For example, if you had a debug system that would log an error for all entities that are
	/// renderable, but lacked a transform, then you could use the `except` tag to denote the filter
	/// what to do as a hint.
	template <typename T>
	struct except
	{};


	namespace details
	{
		template <bool has_entities, typename... Ts>
		struct pack_tuple
		{
			using range_t		  = std::tuple<psl::array_view<const entity>, psl::array_view<Ts>...>;
			using range_element_t = std::tuple<entity, Ts...>;
		};

		template <typename... Ts>
		struct pack_tuple<false, Ts...>
		{
			using range_t		  = std::tuple<psl::array_view<Ts>...>;
			using range_element_t = std::tuple<Ts...>;
		};

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
		struct extract_add
		{
			using type = std::tuple<>;
		};

		template <typename T>
		struct extract_add<on_add<T>>
		{
			using type = std::tuple<T>;
		};

		template <typename T>
		struct extract_remove
		{
			using type = std::tuple<>;
		};

		template <typename T>
		struct extract_remove<on_remove<T>>
		{
			using type = std::tuple<T>;
		};

		template <typename T>
		struct extract_except
		{
			using type = std::tuple<>;
		};

		template <typename T>
		struct extract_except<except<T>>
		{
			using type = std::tuple<T>;
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

		template <typename T>
		struct decode_type<on_add<T>>
		{
			using type = std::tuple<T>;
		};

		template <typename T>
		struct decode_type<on_remove<T>>
		{
			using type = std::tuple<T>;
		};

		template <typename T>
		struct decode_type<except<T>>
		{
			using type = std::tuple<T>;
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

		template <typename... Tx, typename... Ty>
		auto merge_tuple(const std::tuple<Tx...>& tx, const std::tuple<Ty...>& ty)
		{
			return std::tuple<Tx..., Ty...>();
		}

		template <typename... Tx, typename... Ty>
		auto merge_tuple(const std::tuple<Tx...>& tx, const Ty&... ty)
		{
			if constexpr(sizeof...(ty) > 1)
			{
				return merge_tuple(tx, merge_tuple(ty...));
			}
			else
			{
				return merge_tuple(tx, ty...);
			}
		}

		template <typename... Ts>
		struct typelist_to_tuple
		{
			using type = decltype(details::merge_tuple(std::declval<typename details::decode_type<Ts>::type>()...));
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
		struct typelist_to_add_pack
		{
			using type = decltype(details::merge_tuple(std::declval<typename details::extract_add<Ts>::type>()...));
		};


		template <typename... Ts>
		struct typelist_to_combine_pack
		{
			using type = decltype(details::merge_tuple(std::declval<typename details::extract_combine<Ts>::type>()...));
		};

		template <typename... Ts>
		struct typelist_to_except_pack
		{
			using type = decltype(details::merge_tuple(std::declval<typename details::extract_except<Ts>::type>()...));
		};

		template <typename... Ts>
		struct typelist_to_break_pack
		{
			using type = decltype(details::merge_tuple(std::declval<typename details::extract_break<Ts>::type>()...));
		};

		template <typename... Ts>
		struct typelist_to_remove_pack
		{
			using type = decltype(details::merge_tuple(std::declval<typename details::extract_remove<Ts>::type>()...));
		};
	} // namespace details

	template <typename... Ts>
	class component_pack
	{
	  public:
		using pack_t	= typename details::typelist_to_pack_view<Ts...>::type;
		using combine_t = typename details::typelist_to_combine_pack<Ts...>::type;
		using break_t   = typename details::typelist_to_break_pack<Ts...>::type;
		using add_t		= typename details::typelist_to_add_pack<Ts...>::type;
		using remove_t  = typename details::typelist_to_remove_pack<Ts...>::type;
		using except_t  = typename details::typelist_to_except_pack<Ts...>::type;

	  public:
	  private:
		pack_t m_Pack;
	};

	template <typename... Ts>
	class pack
	{
		template <std::size_t... Is, typename T>
		static auto iterator_begin(std::index_sequence<Is...>, const T& t)
		{
			return std::make_tuple(std::begin(std::get<Is>(t))...);
		}
		template <std::size_t... Is, typename T>
		static auto iterator_end(std::index_sequence<Is...>, const T& t)
		{
			return std::make_tuple(std::end(std::get<Is>(t))...);
		}

	  public:
		using range_t			 = std::tuple<psl::array_view<Ts>...>;
		using range_element_t	= std::tuple<Ts...>;
		using iterator_element_t = std::tuple<typename psl::array_view<Ts>::iterator...>;

		class iterator
		{
			template <std::size_t... Is>
			auto pack_impl(std::index_sequence<Is...>)
			{
				return std::forward_as_tuple((std::get<Is>(data).value())...);
			}

			template <std::size_t... Is>
			const auto cpack_impl(std::index_sequence<Is...>) const
			{
				return std::forward_as_tuple((std::get<Is>(data).cvalue())...);
			}

			template <std::size_t... Is>
			auto increment(std::index_sequence<Is...>)
			{
				return (++std::get<Is>(data), ...);
			}

		  public:
			constexpr iterator(const range_t& range) noexcept
				: data(iterator_begin(std::index_sequence_for<Ts...>{}, range)){

				  };
			constexpr iterator(iterator_element_t data) noexcept : data(data){};

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

			bool operator!=(const iterator& other) const noexcept
			{
				return std::get<0>(data) != std::get<0>(other.data);
			}

			bool operator==(const iterator& other) const noexcept
			{
				return std::get<0>(data) == std::get<0>(other.data);
			}

			auto operator*() { return pack_impl(std::index_sequence_for<Ts...>{}); }
			auto operator*() const { return cpack_impl(std::index_sequence_for<Ts...>{}); }

			template <typename T>
			auto get() ->
				typename decltype(*std::get<typename psl::array_view<T>::iterator>(std::declval<iterator_element_t>()))
			{
				static_assert(details::tuple_contains_type<psl::array_view<T>, range_t>::value,
							  "the requested component type does not exist in the pack");
				return *std::get<typename psl::array_view<T>::iterator>(data);
			}

			template <size_t N>
			auto get() const noexcept -> decltype(*std::get<N>(std::declval<iterator_element_t>()))
			{
				static_assert(N < std::tuple_size<range_t>::value,
							  "you requested a component outside of the range of the pack");
				return *std::get<N>(data);
			}

		  private:
			iterator_element_t data;
		};
		pack() : m_Pack() {};
		pack(psl::array_view<Ts>... views) : m_Pack(std::make_tuple(std::forward<psl::array_view<Ts>>(views)...)) {}

		range_t read() { return m_Pack; }

		template <typename T>
		psl::array_view<T> get() const noexcept
		{
			static_assert(details::tuple_contains_type<psl::array_view<T>, range_t>::value,
						  "the requested component type does not exist in the pack");
			return std::get<psl::array_view<T>>(m_Pack);
		}

		template <size_t N>
		auto get() const noexcept -> decltype(std::get<N>(std::declval<range_t>()))
		{
			static_assert(N < std::tuple_size<range_t>::value,
						  "you requested a component outside of the range of the pack");
			return std::get<N>(m_Pack);
		}

		iterator begin() const noexcept { return iterator{iterator_begin(std::index_sequence_for<Ts...>{}, m_Pack)}; }

		iterator end() const noexcept { return iterator{iterator_end(std::index_sequence_for<Ts...>{}, m_Pack)}; }
		constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }

	  private:
		  psl::array_view<entity> m_Entities;
		range_t m_Pack;
	};
} // namespace core::ecs