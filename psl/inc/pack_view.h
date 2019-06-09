#pragma once
#include <iterator>
#include "array_view.h"
#include "template_utils.h"

namespace psl
{
	template <typename... Ts>
	class tuple_ref
	{
	  public:
		tuple_ref(const std::tuple<Ts&...>& data) : data(&std::get<Ts&>(data)...){};

		template <typename... Ys>
		tuple_ref(Ys&&... data) : data(&std::forward<Ys>(data)...){};
		~tuple_ref()
		{
			if(m_Internal)
			{
				(delete(std::get<Ts*>(data)), ...);
			}
		}
		tuple_ref(const tuple_ref& other) noexcept : data(*std::get<Ts*>(other.data)...){};
		tuple_ref(tuple_ref&& other) noexcept : data(new Ts(*std::get<Ts*>(other.data))...), m_Internal(true){};
		tuple_ref& operator=(const tuple_ref& other) noexcept
		{
			if(this != &other)
			{
				(void(*std::get<Ts*>(data) = *(std::get<Ts*>(other.data))), ...);
			}
			return *this;
		};
		tuple_ref& operator=(tuple_ref&& other) noexcept
		{
			if(this != &other)
			{
				(void(*std::get<Ts*>(data) = std::move(*std::get<Ts*>(other.data))), ...);
			}
			return *this;
		};

		template <size_t N>
		auto& get() noexcept
		{
			return *std::get<N>(data);
		}

		template <typename T>
		T& get() noexcept
		{
			return *std::get<T*>(data);
		}

		template <size_t N>
		const auto& get() const noexcept
		{
			return *std::get<N>(data);
		}

		template <typename T>
		const T& get() const noexcept
		{
			return *std::get<T*>(data);
		}

		operator std::tuple<Ts&...>&() noexcept { return *reinterpret_cast<std::tuple<Ts&...>*>(&data); }
		operator const std::tuple<Ts&...>&() const noexcept { return *reinterpret_cast<std::tuple<Ts&...>*>(&data); }

	  private:
		std::tuple<Ts*...> data;
		bool m_Internal{false};
	};
	template <typename... Ts>
	class pack_view
	{
	  public:
		using range_t = std::tuple<psl::array_view<Ts>...>;

	  private:
		static std::tuple<Ts&...> iterator_begin(const range_t& t, size_t index = 0)
		{
			return std::make_tuple(std::next(std::begin(std::get<psl::array_view<Ts>>(t)), index)...);
		}

		static std::tuple<Ts&...> iterator_end(const range_t& t, size_t index = 0)
		{
			return std::make_tuple(std::prev(std::end(std::get<psl::array_view<Ts>>(t)), index)...);
		}


	  public:
		class iterator
		{
		  public:
			using self_type		  = iterator;
			using value_type	  = std::tuple<std::reference_wrapper<Ts>...>;
			using reference		  = std::tuple<Ts&...>&;
			using const_reference = const reference;
			using pointer		  = value_type*;

			using difference_type   = std::ptrdiff_t;
			using iterator_category = std::random_access_iterator_tag;

		  private:
			template <typename T>
			void advance_tuple_element(std::reference_wrapper<T>& target, std::intptr_t count)
			{
				using type = std::remove_const_t<T>;
				target	 = *((type*)&target.get() + count);
			}

			void advance_tuple(std::uintptr_t count)
			{
				(advance_tuple_element(std::get<std::reference_wrapper<Ts>>(data), count), ...);
			}


		  public:
			iterator(const value_type& data) : data(data){};
			template <typename = std::enable_if_t<std::conditional<!std::is_same<std::tuple<>, range_t>::value>>>
			iterator(const range_t& range) : iterator(iterator_begin(range)){};
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
				return difference_type{&std::get<0>(data).get() - &std::get<0>(offset.data).get()};
			}

			difference_type operator+(const iterator& offset) const
			{
				return difference_type{&std::get<0>(data).get() + &std::get<0>(offset.data).get()};
			}


			bool operator!=(const iterator& other) const noexcept
			{
				return &std::get<0>(data).get() != &std::get<0>(other.data).get();
			}

			bool operator==(const iterator& other) const noexcept
			{
				return &std::get<0>(data).get() == &std::get<0>(other.data).get();
			}

			bool operator<(const iterator& other) const noexcept
			{
				return &std::get<0>(data).get() < &std::get<0>(other.data).get();
			}
			bool operator<=(const iterator& other) const noexcept
			{
				return &std::get<0>(data).get() <= &std::get<0>(other.data).get();
			}
			bool operator>(const iterator& other) const noexcept
			{
				return &std::get<0>(data).get() > &std::get<0>(other.data).get();
			}
			bool operator>=(const iterator& other) const noexcept
			{
				return &std::get<0>(data).get() >= &std::get<0>(other.data).get();
			}

			reference operator*() noexcept { return *reinterpret_cast<std::tuple<Ts&...>*>(&data); }

			const_reference operator*() const noexcept { return *reinterpret_cast<std::tuple<Ts&...>*>(&data); }

			template <typename T>
			T& get()
			{
				static_assert(
					::utility::templates::template tuple_contains_type<std::reference_wrapper<T>, value_type>::value,
					"the requested component type does not exist in the pack");
				return std::get<T&>(*reinterpret_cast<std::tuple<Ts&...>*>(&data));
			}

			template <size_t N>
			auto get() const noexcept -> decltype(std::get<N>(std::declval<value_type>()))
			{
				static_assert(N < std::tuple_size<value_type>::value,
							  "you requested a component outside of the range of the pack");
				return std::get<N>(*reinterpret_cast<std::tuple<Ts&...>*>(&data));
			}

			operator reference() noexcept { return *reinterpret_cast<std::tuple<Ts&...>*>(&data); }
			operator const_reference() const noexcept { return *reinterpret_cast<std::tuple<Ts&...>*>(&data); }

		  private:
			value_type data;
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

		typename iterator::const_reference operator[](size_t index) const noexcept
		{
			auto x{iterator_begin(m_Pack, index)};
			return *iterator{x};
		}

		typename iterator::reference operator[](size_t index) noexcept
		{
			auto x{iterator_begin(m_Pack, index)};
			return *iterator{x};
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
		return psl::pack_view<Ts...>{pack.get<Ts>()...};
	}

} // namespace psl