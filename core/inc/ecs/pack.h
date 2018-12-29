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


	template<typename... Ts>
	struct filter
	{};

	namespace details
	{
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
		struct extract_physical
		{
			using type = std::tuple<T>;
		};

		template <typename... Ts>
		struct extract_physical<filter<Ts...>>
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
		
		template <typename... Ts>
		struct decode_type<filter<Ts...>>
		{
			using type = std::tuple<Ts...>;
		};

		template <typename... Ts>
		struct typelist_to_tuple
		{
			using type = decltype(std::tuple_cat(std::declval<typename details::decode_type<Ts>::type>()...));
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


		template<typename... Ts>
		struct wrap_with_array_view
		{
			using type = std::tuple<psl::array_view<Ts>...>;
		};

		template<typename... Ts>
		struct wrap_with_array_view<std::tuple<Ts...>>
		{
			using type = std::tuple<psl::array_view<Ts>...>;
		};

		template <bool has_entities, typename... Ts>
		struct pack_tuple
		{
			using range_element_t = typename details::typelist_to_physical_pack<Ts..., core::ecs::entity>::type;
			using range_t		  = typename wrap_with_array_view<range_element_t>::type;
			using iterator_element_t = std::tuple<typename range_t::iterator...>;
		};

		template <typename... Ts>
		struct pack_tuple<false, Ts...>
		{
			using range_element_t = typename details::typelist_to_physical_pack<Ts...>::type;
			using range_t		  = typename wrap_with_array_view<range_element_t>::type;
			using iterator_element_t = std::tuple<typename range_t::iterator...>;
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
		friend class core::ecs::state;

		template<typename... Ts>
		struct to_iter{};

		template<typename... Ts>
		struct to_iter<std::tuple<Ts...>>
		{
			using type = std::tuple<typename Ts::iterator...>;
		};


	public:
		using range_element_t = typename details::typelist_to_physical_pack<Ts...>::type;
		using range_t		  = typename details::wrap_with_array_view<range_element_t>::type;
		using iterator_element_t = typename to_iter<range_t>::type;


		using filter_t = typename details::typelist_to_pack<Ts...>::type;
		using combine_t = typename details::typelist_to_combine_pack<Ts...>::type;
		using break_t   = typename details::typelist_to_break_pack<Ts...>::type;
		using add_t		= typename details::typelist_to_add_pack<Ts...>::type;
		using remove_t  = typename details::typelist_to_remove_pack<Ts...>::type;
		using except_t  = typename details::typelist_to_except_pack<Ts...>::type;

	private:
		template <std::size_t... Is, typename T>
		static auto create_iterator(std::index_sequence<Is...>, const T& t, size_t index = 0)
		{
			return std::make_tuple(std::next(std::begin(std::get<Is>(t)), index)...);
		}
		template <std::size_t... Is, typename T>
		static auto iterator_end(std::index_sequence<Is...>, const T& t)
		{
			return std::make_tuple(std::end(std::get<Is>(t))...);
		}


	  public:

		class iterator
		{
		public:
			typedef iterator self_type;
			typedef iterator_element_t value_type;
			typedef iterator_element_t& reference;
			typedef iterator_element_t* pointer;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
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
			auto advance(std::index_sequence<Is...>, difference_type offset = 1)
			{
				return (std::advance(std::get<Is>(data), offset), ...);
			}

		  public:


			constexpr iterator(const range_t& range) noexcept
				: data(create_iterator(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, range)){

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
				advance(std::make_index_sequence<std::tuple_size<range_element_t>::value>{});
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

			iterator& operator+=(difference_type offset)
			{
				advance(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, offset);
				return *this;
			}

			iterator& operator-=(difference_type offset)
			{
				advance(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, -offset);
				return *this;
			}

			auto operator*() { return pack_impl(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}); }
			auto operator*() const { return cpack_impl(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}); }

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

		pack(range_t views) : m_Pack(views) {}

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

		auto operator[](size_t index) const noexcept -> decltype(std::declval<iterator>().operator*())
		{
			return *iterator{create_iterator(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, m_Pack, index)};
		}

		iterator begin() const noexcept { return iterator{create_iterator(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, m_Pack)}; }

		iterator end() const noexcept { return iterator{iterator_end(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, m_Pack)}; }
		constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }

	  private:

		  template <size_t N>
		  auto reference_get() noexcept -> decltype(std::get<N>(std::declval<range_t>()))&
		  {
			  static_assert(N < std::tuple_size<range_t>::value,
				  "you requested a component outside of the range of the pack");
			  return std::get<N>(m_Pack);
		  }

		range_t m_Pack;
	};

	template <typename... Ts>
	class entity_pack
	{
		friend class core::ecs::state;


		template<typename... Ts>
		struct to_iter{};

		template<typename... Ts>
		struct to_iter<std::tuple<Ts...>>
		{
			using type = std::tuple<typename Ts::iterator...>;
		};

	public:
		using range_element_t = typename details::typelist_to_physical_pack<Ts..., core::ecs::entity>::type;
		using range_t		  = typename details::wrap_with_array_view<range_element_t>::type;
		using iterator_element_t = typename to_iter<range_t>::type;


		using filter_t = typename details::typelist_to_pack<Ts...>::type;
		using combine_t = typename details::typelist_to_combine_pack<Ts...>::type;
		using break_t   = typename details::typelist_to_break_pack<Ts...>::type;
		using add_t		= typename details::typelist_to_add_pack<Ts...>::type;
		using remove_t  = typename details::typelist_to_remove_pack<Ts...>::type;
		using except_t  = typename details::typelist_to_except_pack<Ts...>::type;

	private:

		template <std::size_t... Is, typename T>
		static auto iterator_end(std::index_sequence<Is...>, const T& t)
		{
			return std::make_tuple(std::end(std::get<Is>(t))...);
		}

		template <std::size_t... Is, typename T>
		static auto create_iterator(std::index_sequence<Is...>, const T& t, size_t index = 0)
		{
			return std::make_tuple(std::next(std::begin(std::get<Is>(t)), index)...);
		}

	public:

		class iterator
		{
		public:
			typedef iterator self_type;
			typedef iterator_element_t value_type;
			typedef iterator_element_t& reference;
			typedef iterator_element_t* pointer;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
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
			auto advance(std::index_sequence<Is...>, difference_type offset = 1)
			{
				return (std::advance(std::get<Is>(data), offset), ...);
			}
		public:

			constexpr iterator(const range_t& range) noexcept
				: data(create_iterator(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, range)){

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
				advance(std::make_index_sequence<std::tuple_size<range_element_t>::value>{});
				return *this;
			}

			iterator& operator+=(difference_type offset)
			{
				advance(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, offset);
				return *this;
			}

			iterator& operator-=(difference_type offset)
			{
				advance(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, -offset);
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

			auto operator*() { return pack_impl(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}); }
			auto operator*() const { return cpack_impl(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}); }

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
		auto operator[](size_t index) const noexcept -> decltype(std::declval<iterator>().operator*())
		{
			return *iterator{create_iterator(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, m_Pack, index)};
		}

		iterator begin() const noexcept { return iterator{create_iterator(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, m_Pack)}; }

		iterator end() const noexcept { return iterator{iterator_end(std::make_index_sequence<std::tuple_size<range_element_t>::value>{}, m_Pack)}; }
		constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }

		entity_pack() : m_Pack() {};
		entity_pack(range_t views) : m_Pack(views) {}
		psl::array_view<core::ecs::entity> entities() const
		{
			return reference_get<std::tuple_size<range_t>::value - 1>();
		}
	private:
		psl::array_view<core::ecs::entity>& reference_get_entities() noexcept
		{
			return reference_get<std::tuple_size<range_t>::value - 1>();
		}


		template <size_t N>
		auto reference_get() noexcept -> decltype(std::get<N>(std::declval<range_t>()))&
		{
			static_assert(N < std::tuple_size<range_t>::value,
				"you requested a component outside of the range of the pack");
			return std::get<N>(m_Pack);
		}

		range_t m_Pack;
	};
} // namespace core::ecs