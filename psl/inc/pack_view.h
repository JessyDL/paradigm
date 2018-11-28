#pragma once
#include <iterator>
#include "array_view.h"
#include "template_utils.h"

namespace psl
{


	template <typename... Ts>
	class pack_view
	{
		using range_t = std::tuple<psl::array_view<Ts>...>;
		using range_element_t = std::tuple<Ts...>;
		using iterator_element_t = std::tuple<typename psl::array_view<Ts>::iterator...>;

		template <std::size_t...Is, typename T>
		static auto iterator_begin(std::index_sequence<Is...>, const T& t)
		{
			return std::make_tuple(std::begin(std::get<Is>(t))...);
		}
		template <std::size_t...Is, typename T>
		static auto iterator_end(std::index_sequence<Is...>, const T& t)
		{
			return std::make_tuple(std::end(std::get<Is>(t))...);
		}

	public:
		class iterator
		{
			template <std::size_t...Is>
			auto pack_impl(std::index_sequence<Is...>)
			{
				return std::forward_as_tuple((std::get<Is>(data).value()) ...);
			}

			template <std::size_t...Is>
			const auto cpack_impl(std::index_sequence<Is...>) const
			{
				return std::forward_as_tuple((std::get<Is>(data).cvalue()) ...);
			}

			template <std::size_t...Is>
			auto increment(std::index_sequence<Is...>)
			{
				return (++std::get<Is>(data), ...);
			}
		public:
			constexpr iterator(const range_t& range) noexcept : data(iterator_begin(std::index_sequence_for<Ts...>{}, range))
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

			auto operator*()
			{
				return pack_impl(std::index_sequence_for<Ts...>{});
			}
			auto operator*() const
			{
				return cpack_impl(std::index_sequence_for<Ts...>{});
			}

			template<typename T>
			auto get()	-> typename decltype(*std::get<typename psl::array_view<T>::iterator>(std::declval<iterator_element_t>()))
			{
				using array_view_t = psl::array_view<T>;
				static_assert(utility::templates::tuple_contains_type<array_view_t, range_t>::value, "the requested component type does not exist in the pack");
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
		pack_view(psl::array_view<Ts>... views) : m_Pack(std::make_tuple(std::forward<psl::array_view<Ts>>(views) ...))
		{

		}

		range_t read() { return m_Pack; }

		template <typename T>
		psl::array_view<T> get() const noexcept
		{
			static_assert(utility::templates::tuple_contains_type<psl::array_view<T>, range_t>::value, "the requested component type does not exist in the pack");
			return std::get<psl::array_view<T>>(m_Pack);
		}

		template <size_t N>
		auto get() const noexcept	-> decltype(std::get<N>(std::declval<range_t>()))
		{
			static_assert(N < std::tuple_size<range_t>::value, "you requested a component outside of the range of the pack");
			return std::get<N>(m_Pack);
		}

		iterator begin() const noexcept { return iterator{iterator_begin(std::index_sequence_for<Ts...>{}, m_Pack)}; }

		iterator end() const noexcept { return iterator{iterator_end(std::index_sequence_for<Ts...>{}, m_Pack)}; }
		constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }

	private:
		range_t m_Pack;
	};
} // namespace psl