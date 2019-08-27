#pragma once
#include <iterator>
#include "psl/array_view.h"
#include "psl/template_utils.h"
#include <type_traits>

namespace psl
{
	template <typename... Ts>
	class tuple_ref;
}

namespace std
{

	template <typename... Ts>
	void swap(psl::tuple_ref<Ts...>&, psl::tuple_ref<Ts...>&) noexcept;

	/*template <typename... Ts>
	void swap(psl::tuple_ref<Ts...>, psl::tuple_ref<Ts...>) noexcept;*/
}

namespace psl
{
	template <typename... Ts>
	class tuple_ref
	{
	  public:
		using value_type = std::tuple<Ts&...>;
		tuple_ref(std::tuple<std::vector<Ts>...>& values, size_t index)
			: data(std::get<std::vector<Ts>>(values)[index]...)
		{}

		template <typename... Ys>
		tuple_ref(std::tuple<psl::array_view<Ys>...>& values, size_t index)
			: data(std::get<psl::array_view<Ys>>(values)[index]...)
		{}

		template <typename... Ys>
		tuple_ref(const std::tuple<typename psl::array_view<Ys>::iterator...>& values)
			: data(*std::get<typename psl::array_view<Ys>::iterator>(values)...)
		{}

		template <typename... Ys>
		tuple_ref(std::tuple<std::reference_wrapper<Ys>...>& values)
			: data(std::get<std::reference_wrapper<Ys>>(values)...)
		{}

		template <typename... Ys>
		tuple_ref(std::tuple<Ys*...>& values)
			: data(*std::get<Ys*>(values)...)
		{}

		template <typename... Ys>
		tuple_ref(std::tuple<Ys*...>* values) : data(*std::get<Ys*>(*values)...)
		{}

		tuple_ref(Ts&... values) : data(values...) {}

		tuple_ref(const tuple_ref& other) : data(other.data){};
		tuple_ref(tuple_ref&& other) noexcept : data(other.data){};
		tuple_ref& operator=(const tuple_ref& other)
		{
			if(this != &other)
			{
				data = other.data;
			}
			return *this;
		}
		tuple_ref& operator=(tuple_ref&& other) noexcept
		{
			if(this != &other)
			{
				data = other.data;
			}
			return *this;
		}

		operator value_type&() noexcept { return data; }
		operator const value_type&() const noexcept { return data; }

		/*operator std::tuple<Ts*...>() const noexcept
		{
			return std::tuple<Ts*...>{&std::get<Ts&>(data) ...};
		}*/

		template <size_t N>
		auto& get() noexcept
		{
			return std::get<N>(data);
		}

		template <typename T>
		auto get() noexcept -> std::remove_reference_t<T>&
		{
			using type = std::remove_reference_t<T>;
			return std::get<type&>(data);
		}

		template <size_t N>
		const auto& get() const noexcept
		{
			return std::get<N>(data);
		}

		template <typename T>
		auto get() const noexcept -> const std::remove_reference_t<T>&
		{
			using type = std::remove_reference_t<T>;
			return std::get<type&>(data);
		}

		//friend void std::swap(tuple_ref&, tuple_ref&) noexcept;
		//friend void std::swap(psl::tuple_ref<Ts...>, psl::tuple_ref<Ts...>) noexcept;

		value_type data;
	};
} // namespace psl

namespace std
{

	template <size_t N, typename... Ts>
	struct tuple_element<N, psl::tuple_ref<Ts...>>
		: public std::tuple_element<N, typename psl::tuple_ref<Ts...>::value_type>
	{};

	template <typename... Ts>
	struct tuple_size<psl::tuple_ref<Ts...>> : public std::tuple_size<typename psl::tuple_ref<Ts...>::value_type>
	{};


	template <size_t N, typename... Ts>
	auto& get(const psl::tuple_ref<Ts...>& value)
	{
		return std::get<N>(value.data);
	}

	template <typename T, typename... Ts>
	auto& get(const psl::tuple_ref<Ts...>& value)
	{
		using type = std::remove_reference_t<T>;
		return std::get<type&>(value.data);
	}

	template <typename... Ts>
	inline void swap(psl::tuple_ref<Ts...>& lhs, psl::tuple_ref<Ts...>& rhs) noexcept
	{
		(void(std::swap(std::get<Ts&>(lhs.data), (std::get<Ts&>(rhs.data)))), ...);
	}

	//template <typename... Ts>
	//inline void swap(psl::tuple_ref<Ts...> lhs, psl::tuple_ref<Ts...> rhs) noexcept
	//{
	//	(void(std::swap(std::get<Ts&>(lhs.data), std::get<Ts&>(rhs.data))), ...);
	//}

} // namespace std


namespace psl
{

	template <typename... Ts>
	class pack_view
	{
	  public:
		using range_t = std::tuple<psl::array_view<Ts>...>;

	  private:
		static psl::tuple_ref<Ts...> iterator_begin(const range_t& t, size_t index = 0)
		{
			return psl::tuple_ref<Ts...>(*std::next(std::begin(std::get<psl::array_view<Ts>>(t)), index)...);
		}

		static psl::tuple_ref<Ts...> iterator_end(const range_t& t, size_t index = 0)
		{
			return psl::tuple_ref<Ts...>(*std::prev(std::end(std::get<psl::array_view<Ts>>(t)), index)...);
		}


	  public:
		class iterator
		{
		  public:
			using self_type		  = iterator;
			using internal_type   = std::tuple<Ts*...>;
			using value_type	  = psl::tuple_ref<Ts...>;
			using reference		  = value_type&;
			using const_reference = reference;
			using pointer		  = value_type*;

			using difference_type   = std::ptrdiff_t;
			using iterator_category = std::random_access_iterator_tag;

		  private:
			/*template <typename T>
			void advance_tuple_element(std::reference_wrapper<T>* target, std::intptr_t count)
			{
				using type = std::remove_const_t<T>;
				target	 = *((type*)&target.get() + count);
			}*/

			void advance_tuple(std::uintptr_t count)
			{
				(void(std::get<Ts*>(data) += count), ...);
			}


		  public:
			//iterator(const value_type& data) : data(data){};
			//template <typename = std::enable_if_t<std::conditional<!std::is_same<std::tuple<>, range_t>::value>>>
			//iterator(const range_t& range) : iterator(iterator_begin(range)){};

			iterator(const psl::tuple_ref<Ts...>& data) : data(&std::get<Ts>(data)...){};
			~iterator() = default;
			iterator(const iterator& other) noexcept : data(other.data){};
			iterator(iterator&& other) noexcept : data(std::move(other.data)){};
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
				auto copy{*this};
				copy += offset;
				return copy;
			}

			iterator operator-(difference_type offset) const
			{
				auto copy{*this};
				copy -= offset;
				return copy;
			}

			difference_type operator-(const iterator& offset) const
			{
				return difference_type{std::get<0>(data) - std::get<0>(offset.data)};
			}

			difference_type operator+(const iterator& offset) const
			{
				return difference_type{std::get<0>(data) + std::get<0>(offset.data)};
			}


			bool operator!=(const iterator& other) const noexcept
			{
				return std::get<0>(data) != std::get<0>(other.data);
			}

			bool operator==(const iterator& other) const noexcept
			{
				return std::get<0>(data) == std::get<0>(other.data);
			}

			bool operator<(const iterator& other) const noexcept
			{
				return std::get<0>(data) < std::get<0>(other.data);
			}
			bool operator<=(const iterator& other) const noexcept
			{
				return std::get<0>(data) <= std::get<0>(other.data);
			}
			bool operator>(const iterator& other) const noexcept
			{
				return std::get<0>(data) > std::get<0>(other.data);
			}
			bool operator>=(const iterator& other) const noexcept
			{
				return std::get<0>(data) >= std::get<0>(other.data);
			}

			auto& operator*() noexcept { return *reinterpret_cast<psl::tuple_ref<Ts...>*>(&data); }

			const auto& operator*() const noexcept { return *reinterpret_cast<const psl::tuple_ref<Ts...>*>(&data); }

			template <typename T>
			T& get()
			{
				static_assert(::utility::templates::template tuple_contains_type<T*, internal_type>::value,
					"the requested component type does not exist in the pack");
				return std::get<T>(psl::tuple_ref<Ts...>(data));
			}

			template <size_t N>
			auto get() const noexcept -> decltype(std::get<N>(std::declval<internal_type>()))
			{
				static_assert(N < std::tuple_size<internal_type>::value,
							  "you requested a component outside of the range of the pack");
				return std::get<N>(psl::tuple_ref<Ts...>(data));
			}

			operator psl::tuple_ref<Ts...>&() noexcept { return *reinterpret_cast<psl::tuple_ref<Ts...>*>(&data); }
			operator const psl::tuple_ref<Ts...>&() const noexcept { return *reinterpret_cast<const psl::tuple_ref<Ts...>*>(&data); }

		  private:
			internal_type data;
		};

		pack_view(psl::array_view<Ts>... views) : m_Pack(std::make_tuple(std::forward<psl::array_view<Ts>>(views)...))
		{}
		// pack_view() = default;

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
			auto x{iterator_begin(m_Pack, index)};
			return *iterator(x);
		}

		auto operator[](size_t index) noexcept
		{
			auto x{iterator_begin(m_Pack, index)};
			return *iterator(x);
		}

		iterator begin() const noexcept { return iterator{iterator_begin(m_Pack)}; }
		iterator end() const noexcept { return iterator{iterator_end(m_Pack)}; }
		constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }


	  private:
		range_t m_Pack{};
	};

	template <typename... Ts>
	psl::pack_view<Ts...> zip(psl::array_view<Ts>... range)
	{
		return psl::pack_view<Ts...>(range...);
	}

	template <typename... Ts, typename T>
	psl::pack_view<Ts...> make_pack(const T& pack)
	{
		return psl::pack_view<Ts...>{pack.template get<Ts>()...};
	}


	template <typename... Ts>
	void swap(psl::tuple_ref<Ts...>& lhs, psl::tuple_ref<Ts...>& rhs) noexcept
	{
		(void(std::swap(std::get<Ts&>(lhs.data), std::get<Ts&>(rhs.data))), ...);
	}

	/*template <typename... Ts>
	void swap(psl::tuple_ref<Ts...> lhs, psl::tuple_ref<Ts...> rhs) noexcept
	{
		(void(std::swap(std::get<Ts&>(lhs.data), std::get<Ts&>(rhs.data))), ...);
	}*/
} // namespace psl




// namespace std
//{
//
//	template <typename... Ts>
//	void iter_swap(typename psl::pack_view<Ts...>::iterator& lhs,
//				   typename psl::pack_view<Ts...>::iterator& rhs) noexcept
//	{
//		std::swap(*lhs, *rhs);
//	}
//
//	template <typename... Ts>
//	void iter_swap(typename psl::pack_view<Ts...>::iterator&& lhs, typename psl::pack_view<Ts...>::iterator&& rhs)
//noexcept
//	{
//		std::swap(*lhs, *rhs);
//	}
//} // namespace std