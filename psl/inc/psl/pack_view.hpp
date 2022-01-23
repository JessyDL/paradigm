#pragma once
#include "psl/array_view.hpp"
#include "psl/template_utils.hpp"
#include <iterator>
#include <type_traits>

namespace psl
{
	namespace details
	{
		// todo move to ecs/pack.h
		template <typename... Ts>
		class unpack_iterator
		{
		  public:
			using self_type		  = unpack_iterator;
			using internal_type	  = std::tuple<typename psl::array_view<Ts>::iterator...>;
			using value_type	  = std::tuple<typename psl::array_view<Ts>::iterator...>;
			using reference		  = value_type&;
			using const_reference = reference;
			using pointer		  = value_type*;

			using difference_type		   = std::ptrdiff_t;
			using unpack_iterator_category = std::random_access_iterator_tag;

		  private:
			void advance_tuple(std::uintptr_t count)
			{
				(void(std::get<typename psl::array_view<Ts>::iterator>(data) =
						std::next(std::get<typename psl::array_view<Ts>::iterator>(data), count)),
				 ...);
			}

		  public:
			unpack_iterator(const value_type& data) :
				data(std::get<typename psl::array_view<Ts>::iterator>(data)...) {};
			~unpack_iterator() = default;
			unpack_iterator(const unpack_iterator& other) noexcept : data(other.data) {};
			unpack_iterator(unpack_iterator&& other) noexcept : data(std::move(other.data)) {};
			unpack_iterator& operator=(const unpack_iterator& other) noexcept
			{
				if(this != &other)
				{
					data = other.data;
				}
				return *this;
			};
			unpack_iterator& operator=(unpack_iterator&& other) noexcept
			{
				if(this != &other)
				{
					data	   = std::move(other.data);
					other.data = {};
				}
				return *this;
			};

			constexpr unpack_iterator operator++() const noexcept
			{
				auto next = *this;
				++next;
				return next;
			}
			constexpr unpack_iterator& operator++() noexcept
			{
				advance_tuple(1);
				return *this;
			}

			constexpr unpack_iterator operator--() const noexcept
			{
				auto next = *this;
				--next;
				return next;
			}
			constexpr unpack_iterator& operator--() noexcept
			{
				advance_tuple(-1);
				return *this;
			}

			unpack_iterator& operator+=(difference_type offset)
			{
				advance_tuple(offset);
				return *this;
			}

			unpack_iterator& operator-=(difference_type offset)
			{
				advance_tuple(-offset);
				return *this;
			}


			unpack_iterator operator+(difference_type offset) const
			{
				auto copy {*this};
				copy += offset;
				return copy;
			}

			unpack_iterator operator-(difference_type offset) const
			{
				auto copy {*this};
				copy -= offset;
				return copy;
			}

			difference_type operator-(const unpack_iterator& offset) const
			{
				return difference_type {std::get<0>(data) - std::get<0>(offset.data)};
			}

			difference_type operator+(const unpack_iterator& offset) const
			{
				return difference_type {std::get<0>(data) + std::get<0>(offset.data)};
			}


			bool operator!=(const unpack_iterator& other) const noexcept
			{
				return std::get<0>(data) != std::get<0>(other.data);
			}

			bool operator==(const unpack_iterator& other) const noexcept
			{
				return std::get<0>(data) == std::get<0>(other.data);
			}

			bool operator<(const unpack_iterator& other) const noexcept
			{
				return std::get<0>(data) < std::get<0>(other.data);
			}
			bool operator<=(const unpack_iterator& other) const noexcept
			{
				return std::get<0>(data) <= std::get<0>(other.data);
			}
			bool operator>(const unpack_iterator& other) const noexcept
			{
				return std::get<0>(data) > std::get<0>(other.data);
			}
			bool operator>=(const unpack_iterator& other) const noexcept
			{
				return std::get<0>(data) >= std::get<0>(other.data);
			}
			
			auto operator*() noexcept
			{
				return std::tuple<Ts&...>(*std::get<typename psl::array_view<Ts>::iterator>(data)...);
			}

			auto operator*() const noexcept
			{
				return std::tuple<const Ts&...>(*std::get<typename psl::array_view<Ts>::iterator>(data)...);
			}

			constexpr auto deref() noexcept
			{
				return std::tuple<Ts&...>(*std::get<typename psl::array_view<Ts>::iterator>(data)...);
			}

			constexpr const auto& deref() const noexcept
			{
				return std::tuple<const Ts&...>(*std::get<typename psl::array_view<Ts>::iterator>(data)...);
			}

			template <std::size_t N>
			auto& get() noexcept
			{
				return std::get<N>(deref());
			}
			template <std::size_t N>
			auto&& get() noexcept
			{
				return std::get<N>(deref());
			}

			template <std::size_t N>
			auto const& get() noexcept
			{
				return std::get<N>(deref());
			}

			template <std::size_t N>
			auto const&& get() const noexcept
			{
				return *std::get<N>(deref());
			}

			template <class T>
			constexpr auto& get() noexcept
			{
				return std::get<T>(deref());
			}

			template <class T>
			constexpr auto&& get() noexcept
			{
				return std::get<T>(deref());
			}

			template <class T>
			constexpr const auto& get() const noexcept
			{
				return std::get<T>(deref());
			}

			template <class T>
			constexpr const auto&& get() const noexcept
			{
				return std::get<T>(deref());
			}

			operator value_type&() noexcept { return *reinterpret_cast<value_type*>(&data); }
			operator const value_type&() const noexcept { return *reinterpret_cast<const value_type*>(&data); }

		  private:
			internal_type data;
		};

		template <typename... Ts>
		class iterator
		{
		  public:
			using self_type		  = iterator;
			using internal_type	  = std::tuple<Ts*...>;
			using value_type	  = std::tuple<Ts*...>;
			using reference		  = value_type&;
			using const_reference = reference;
			using pointer		  = value_type*;

			using difference_type	= std::ptrdiff_t;
			using iterator_category = std::random_access_iterator_tag;

		  private:
			template <typename T, T... indices>
			void advance_tuple(std::uintptr_t count, std::integer_sequence<T, indices...> indices_sequence)
			{
				(void(std::get<indices>(data.data) += count), ...);
			}

			void advance_tuple(std::uintptr_t count) { advance_tuple(count, std::index_sequence_for<Ts...> {}); }

		  public:
			~iterator() = default;
			iterator(const iterator& other) noexcept : data(other.data) {};
			iterator(iterator&& other) noexcept : data(std::move(other.data)) {};
			iterator& operator=(const iterator& other) noexcept
			{
				if(this != &other)
				{
					data = other.data;
				}
				return *this;
			};
			iterator& operator=(iterator&& other) noexcept
			{
				if(this != &other)
				{
					data	   = std::move(other.data);
					other.data = {};
				}
				return *this;
			};

			constexpr iterator operator++() const noexcept
			{
				auto next = *this;
				++next;
				return next;
			}
			constexpr iterator& operator++() noexcept
			{
				advance_tuple(1);
				return *this;
			}

			constexpr iterator operator--() const noexcept
			{
				auto next = *this;
				--next;
				return next;
			}
			constexpr iterator& operator--() noexcept
			{
				advance_tuple(-1);
				return *this;
			}

			iterator& operator+=(difference_type offset)
			{
				advance_tuple(offset);
				return *this;
			}

			iterator& operator-=(difference_type offset)
			{
				advance_tuple(-offset);
				return *this;
			}


			iterator operator+(difference_type offset) const
			{
				auto copy {*this};
				copy += offset;
				return copy;
			}

			iterator operator-(difference_type offset) const
			{
				auto copy {*this};
				copy -= offset;
				return copy;
			}

			difference_type operator-(const iterator& offset) const
			{
				return difference_type {std::get<0>(data.data) - std::get<0>(offset.data.data)};
			}

			difference_type operator+(const iterator& offset) const
			{
				return difference_type {std::get<0>(data.data) + std::get<0>(offset.data.data)};
			}


			bool operator!=(const iterator& other) const noexcept
			{
				return std::get<0>(data.data) != std::get<0>(other.data.data);
			}

			bool operator==(const iterator& other) const noexcept
			{
				return std::get<0>(data.data) == std::get<0>(other.data.data);
			}

			bool operator<(const iterator& other) const noexcept
			{
				return std::get<0>(data.data) < std::get<0>(other.data.data);
			}
			bool operator<=(const iterator& other) const noexcept
			{
				return std::get<0>(data.data) <= std::get<0>(other.data.data);
			}
			bool operator>(const iterator& other) const noexcept
			{
				return std::get<0>(data.data) > std::get<0>(other.data.data);
			}
			bool operator>=(const iterator& other) const noexcept
			{
				return std::get<0>(data.data) >= std::get<0>(other.data.data);
			}

			value_type& operator*() noexcept { return data; }
			const value_type& operator*() const noexcept { return data; }
			/*
						template <typename T>
						T& get()
						{
							static_assert(::utility::templates::template tuple_contains_type<T*, internal_type>::value,
										  "the requested component type does not exist in the pack");
							return *std::get<T*>(data.data);
						}

						template <size_t N>
						auto get() const noexcept -> decltype(std::get<N>(std::declval<internal_type>()))
						{
							static_assert(N < std::tuple_size<internal_type>::value,
										  "you requested a component outside of the range of the pack");
							return *std::get<N>(data.data);
						}*/
			template <std::size_t N>
			auto& get() noexcept
			{
				return *std::get<N>(data);
			}
			template <std::size_t N>
			auto&& get() noexcept
			{
				return *std::get<N>(data);
			}

			template <std::size_t N>
			auto const& get() noexcept
			{
				return *std::get<N>(data);
			}

			template <std::size_t N>
			auto const&& get() const noexcept
			{
				return *std::get<N>(data);
			}

			template <class T>
			constexpr auto& get() noexcept
			{
				return *std::get<T*>(data);
			}

			template <class T>
			constexpr auto&& get() noexcept
			{
				return *std::get<T*>(data);
			}

			template <class T>
			constexpr const auto& get() const noexcept
			{
				return *std::get<T*>(data);
			}

			template <class T>
			constexpr const auto&& get() const noexcept
			{
				return *std::get<T*>(data);
			}


			operator value_type&() noexcept { return *reinterpret_cast<value_type*>(&data.data); }
			operator const value_type&() const noexcept { return *reinterpret_cast<const value_type*>(&data.data); }

		  private:
			internal_type data;
		};
	}	 // namespace details
	template <typename... Ts>
	class pack_view
	{
	  public:
		using range_t		  = std::tuple<psl::array_view<Ts>...>;
		using unpack_iterator = details::unpack_iterator<Ts...>;
		using iterator		  = details::iterator<Ts...>;

	  private:
		template <typename T, T... indices>
		constexpr std::tuple<Ts*...>
		iterator_begin(const range_t& t,
					   size_t index,
					   std::integer_sequence<T, indices...> indices_sequence) const noexcept
		{
			return std::tuple<Ts*...>(&(*std::next(std::begin(std::get<indices>(t)), index))...);
		}

		template <typename T, T... indices>
		constexpr std::tuple<Ts*...> iterator_end(const range_t& t,
												  size_t index,
												  std::integer_sequence<T, indices...> indices_sequence) const noexcept
		{
			return std::tuple<Ts*...>(&(*std::prev(std::end(std::get<indices>(t)), index))...);
		}

		template <typename T, T... indices>
		constexpr std::tuple<typename psl::array_view<Ts>::iterator...>
		unpack_iterator_begin(const range_t& t,
							  size_t index,
							  std::integer_sequence<T, indices...> indices_sequence) const noexcept
		{
			return {std::next(std::begin(std::get<indices>(t)), index)...};
		}

		template <typename T, T... indices>
		constexpr std::tuple<typename psl::array_view<Ts>::iterator...>
		unpack_iterator_end(const range_t& t,
							size_t index,
							std::integer_sequence<T, indices...> indices_sequence) const noexcept
		{
			return {std::prev(std::end(std::get<indices>(t)), index)...};
		}


		constexpr std::tuple<Ts*...> iterator_begin(const range_t& t, size_t index = 0) const noexcept
		{
			return iterator_begin(t, index, std::index_sequence_for<Ts...> {});
		}

		constexpr std::tuple<Ts*...> iterator_end(const range_t& t, size_t index = 0) const noexcept
		{
			return iterator_end(t, index, std::index_sequence_for<Ts...> {});
		}

		constexpr auto unpack_iterator_begin(const range_t& t, size_t index = 0) const noexcept
		{
			return unpack_iterator_begin(t, index, std::index_sequence_for<Ts...> {});
		}

		constexpr auto unpack_iterator_end(const range_t& t, size_t index = 0) const noexcept
		{
			return unpack_iterator_end(t, index, std::index_sequence_for<Ts...> {});
		}

	  public:
		pack_view(psl::array_view<Ts>... views) : m_Pack(std::make_tuple(std::forward<psl::array_view<Ts>>(views)...))
		{
#ifdef PE_DEBUG
			auto test = [](size_t size1, size_t size2) { assert_debug_break(size1 == size2); };
			(test(views.size(), size()), ...);
#endif
		}

		range_t view() { return m_Pack; }

		template <typename T>
		psl::array_view<T> get() const noexcept
		{
			static_assert(::utility::templates::template tuple_contains_type<psl::array_view<T>, range_t>::value,
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

		auto operator[](size_t index) const noexcept
		{
			return *(iterator_begin(m_Pack, index));
		}

		auto operator[](size_t index) noexcept
		{
			return *(iterator_begin(m_Pack, index));
		}

		iterator begin() const noexcept { return iterator {iterator_begin(m_Pack)}; }
		iterator end() const noexcept { return iterator {iterator_end(m_Pack)}; }


		auto unpack(size_t index) const noexcept
		{
			auto x {unpack_iterator_begin(m_Pack, index)};
			return *unpack_iterator(x);
		}
		auto unpack(size_t index) noexcept
		{
			auto x {unpack_iterator_begin(m_Pack, index)};
			return *unpack_iterator(x);
		}
		unpack_iterator unpack_begin() const noexcept { return {unpack_iterator_begin(m_Pack)}; }
		unpack_iterator unpack_end() const noexcept { return {unpack_iterator_end(m_Pack)}; }
		constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }

	  private:
		range_t m_Pack {};
	};

	template <typename... Ts>
	psl::pack_view<Ts...> zip(psl::array_view<Ts>... range)
	{
		return psl::pack_view<Ts...>(range...);
	}

	template <typename... Ts>
	auto zip(Ts... range)
	{
		return psl::pack_view(psl::array_view {begin(range), end(range)}...);
	}

	template <typename... Ts, typename T>
	psl::pack_view<Ts...> make_pack(const T& pack)
	{
		return psl::pack_view<Ts...> {pack.template get<Ts>()...};
	}
}	 // namespace psl
