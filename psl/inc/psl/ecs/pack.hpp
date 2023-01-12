#pragma once
#include "details/selectors.hpp"
#include "entity.hpp"
#include "selectors.hpp"


namespace psl::ecs {
class state_t;

namespace {
	template <typename... Ys>
	struct pack_base_types {
		using pack_t		= typename details::typelist_to_pack_view<Ys...>::type;
		using filter_t		= typename details::typelist_to_pack<Ys...>::type;
		using combine_t		= typename details::typelist_to_combine_pack<Ys...>::type;
		using break_t		= typename details::typelist_to_break_pack<Ys...>::type;
		using add_t			= typename details::typelist_to_add_pack<Ys...>::type;
		using remove_t		= typename details::typelist_to_remove_pack<Ys...>::type;
		using except_t		= typename details::typelist_to_except_pack<Ys...>::type;
		using conditional_t = typename details::typelist_to_conditional_pack<Ys...>::type;
		using order_by_t	= typename details::typelist_to_orderby_pack<Ys...>::type;
	};

	template <typename... Ys>
	struct pack_base : public pack_base_types<Ys...> {
		using policy_t = psl::ecs::full;
	};

	template <typename... Ys>
	struct pack_base<psl::ecs::full, Ys...> : public pack_base_types<Ys...> {
		using policy_t = psl::ecs::full;
	};

	template <typename... Ys>
	struct pack_base<psl::ecs::partial, Ys...> : public pack_base_types<Ys...> {
		using policy_t = psl::ecs::partial;
	};
}	 // namespace

/// \brief an iterable container to work with components and entities.
template <typename... Ts>
class pack : public pack_base<Ts...> {
	template <typename T, typename... Ys>
	void check_policy() {
		static_assert(!psl::HasType<psl::ecs::partial, psl::type_pack_t<Ys...>> &&
						!psl::HasType<psl::ecs::full, psl::type_pack_t<Ys...>>,
					  "policy types such as 'partial' and 'full' can only appear as the first type");
	}

  public:
	static constexpr bool has_entities {std::disjunction<std::is_same<psl::ecs::entity, Ts>...>::value};

	using pack_t		= typename pack_base<Ts...>::pack_t;
	using filter_t		= typename pack_base<Ts...>::filter_t;
	using combine_t		= typename pack_base<Ts...>::combine_t;
	using break_t		= typename pack_base<Ts...>::break_t;
	using add_t			= typename pack_base<Ts...>::add_t;
	using remove_t		= typename pack_base<Ts...>::remove_t;
	using except_t		= typename pack_base<Ts...>::except_t;
	using conditional_t = typename pack_base<Ts...>::conditional_t;
	using order_by_t	= typename pack_base<Ts...>::order_by_t;
	using policy_t		= typename pack_base<Ts...>::policy_t;

	static_assert(std::tuple_size<order_by_t>::value <= 1, "multiple order_by statements make no sense");

  public:
	pack() : m_Pack() { check_policy<Ts...>(); };
	pack(pack_t views) : m_Pack(views) { check_policy<Ts...>(); }

	template <typename... Ys>
	pack(Ys&&... values) : m_Pack(std::forward<Ys>(values)...) {
		check_policy<Ts...>();
	}
	pack_t view() { return m_Pack; }

	template <typename T>
	psl::array_view<T> get() const noexcept {
		return m_Pack.template get<T>();
	}

	template <size_t N>
	auto get() const noexcept -> decltype(std::declval<pack_t>().template get<N>()) {
		return m_Pack.template get<N>();
	}

	auto operator[](size_t index) const noexcept { return m_Pack.unpack(index); }
	auto operator[](size_t index) noexcept { return m_Pack.unpack(index); }
	auto begin() const noexcept { return m_Pack.unpack_begin(); }
	auto end() const noexcept { return m_Pack.unpack_end(); }
	constexpr auto size() const noexcept -> size_t { return m_Pack.size(); }
	constexpr auto empty() const noexcept -> bool { return m_Pack.size() == 0; }

  private:
	pack_t m_Pack;
};

template <typename T>
concept IsPolicy = std::is_same_v<T, psl::ecs::partial> || std::is_same_v<T, psl::ecs::full>;

struct direct_t {};
struct indirect_t {};

static constexpr direct_t direct {};
static constexpr indirect_t indirect {};

template <typename T>
concept IsAccessType = std::is_same_v<T, psl::ecs::direct_t> || std::is_same_v<T, psl::ecs::indirect_t>;
//
// template<typename T>
// concept IsValidPack = std::tuple_size<typename T::order_by_t>::value <= 1;

// internal details for non-owning views
namespace details {

	template <typename T, typename SizeType = size_t>
	struct indirect_array_iterator_t {
		using size_type			   = SizeType;
		using value_type		   = std::remove_cvref_t<T>;
		using reference_type	   = value_type&;
		using const_reference_type = value_type const&;
		using pointer_type		   = value_type*;
		using const_pointer_type   = value_type const*;

		using difference_type		   = std::ptrdiff_t;
		using unpack_iterator_category = std::random_access_iterator_tag;

		indirect_array_iterator_t(size_type* it, pointer_type data) : m_It(it), m_Data(data) {}
		indirect_array_iterator_t(size_type const* it, pointer_type data)
			: m_It(const_cast<size_type*>(it)), m_Data(data) {}

		// operator T&() noexcept { return m_Data[*m_It]; }
		// operator T const&() const noexcept { return m_Data[*m_It]; }


		reference_type operator*() noexcept { return m_Data[*m_It]; }
		const_reference_type operator*() const noexcept { return m_Data[*m_It]; }
		pointer_type operator->() noexcept { return &(m_Data[*m_It]); }
		const_pointer_type operator->() const noexcept { return &m_Data[*m_It]; }

		constexpr friend bool operator==(const indirect_array_iterator_t& lhs,
										 const indirect_array_iterator_t& rhs) noexcept {
			return lhs.m_It == rhs.m_It && lhs.m_Data == rhs.m_Data;
		}
		constexpr friend bool operator!=(const indirect_array_iterator_t& lhs,
										 const indirect_array_iterator_t& rhs) noexcept {
			return lhs.m_It != rhs.m_It || lhs.m_Data != rhs.m_Data;
		}

		constexpr friend bool operator<(const indirect_array_iterator_t& lhs,
										const indirect_array_iterator_t& rhs) noexcept {
			return *lhs.m_It < *rhs.m_It;
		}
		constexpr friend bool operator>(const indirect_array_iterator_t& lhs,
										const indirect_array_iterator_t& rhs) noexcept {
			return *lhs.m_It > *rhs.m_It;
		}
		constexpr friend bool operator<=(const indirect_array_iterator_t& lhs,
										 const indirect_array_iterator_t& rhs) noexcept {
			return *lhs.m_It <= *rhs.m_It;
		}
		constexpr friend bool operator>=(const indirect_array_iterator_t& lhs,
										 const indirect_array_iterator_t& rhs) noexcept {
			return *lhs.m_It >= *rhs.m_It;
		}

		constexpr indirect_array_iterator_t& operator++() noexcept {
			++m_It;
			return *this;
		}

		constexpr indirect_array_iterator_t& operator--() noexcept {
			--m_It;
			return *this;
		}

		constexpr indirect_array_iterator_t operator++(int) noexcept {
			auto tmp = *this;
			++(*this);
			return tmp;
		}

		constexpr indirect_array_iterator_t operator--(int) noexcept {
			auto tmp = *this;
			--(*this);
			return tmp;
		}

		constexpr indirect_array_iterator_t& operator+=(difference_type offset) noexcept {
			m_It = std::next(m_It, offset);
			return *this;
		}
		constexpr indirect_array_iterator_t operator+(difference_type offset) const noexcept {
			auto tmp = *this;
			tmp += offset;
			return tmp;
		}
		constexpr indirect_array_iterator_t operator-(difference_type offset) {
			auto tmp = *this;
			tmp -= offset;
			return tmp;
		}

		size_type* m_It {};
		pointer_type m_Data {nullptr};
	};

	/// @brief
	/// @tparam T
	/// @tparam SizeType
	template <typename T, typename SizeType = size_t>
	struct indirect_array_t {
		using size_type	   = SizeType;
		using value_type   = std::remove_cvref_t<T>;
		using pointer_type = value_type*;

		using iterator_type = indirect_array_iterator_t<T, size_type>;

		template <template <typename> typename Y = psl::array>
		indirect_array_t(Y<size_type>&& indices, pointer_type data)
			: m_Indices(std::forward<Y<size_type>>(indices)), m_Data(data) {}
		indirect_array_t(psl::array_view<size_type> indices, pointer_type data) : m_Indices(indices), m_Data(data) {}
		indirect_array_t() = default;

		constexpr auto size() const noexcept { return m_Indices.size(); }
		constexpr auto begin() noexcept { return iterator_type(m_Indices.data(), m_Data); }
		constexpr auto end() noexcept { return iterator_type(m_Indices.data() + m_Indices.size(), m_Data); }
		constexpr auto begin() const noexcept { return iterator_type(m_Indices.data(), m_Data); }
		constexpr auto end() const noexcept { return iterator_type(m_Indices.data() + m_Indices.size(), m_Data); }
		constexpr auto cbegin() const noexcept { return begin(); }
		constexpr auto cend() const noexcept { return end(); }

		psl::array<size_type> m_Indices {};
		pointer_type m_Data {};
	};

	template <typename T, typename IndiceType, template <typename, typename...> typename Container, typename... Rest>
	indirect_array_t(Container<IndiceType, Rest...>, T*) -> indirect_array_t<T, IndiceType>;

	template <typename... Ts>
	struct indirect_pack_view_iterator_t {
		using size_type = psl::ecs::entity;
		template <typename T>
		using value_element_type   = indirect_array_iterator_t<T, size_type>;
		using value_type		   = std::tuple<value_element_type<Ts>...>;
		using reference_type	   = value_type&;
		using const_reference_type = value_type const&;
		using pointer_type		   = value_type*;
		using const_pointer_type   = value_type const*;

		using difference_type		   = std::ptrdiff_t;
		using unpack_iterator_category = std::random_access_iterator_tag;

		indirect_pack_view_iterator_t() = default;
		indirect_pack_view_iterator_t(indirect_array_iterator_t<Ts, size_type>... data) : m_Data(data...) {}

		std::tuple<Ts&...> operator*() noexcept {
			return std::tuple<Ts&...>(*std::get<value_element_type<Ts>>(m_Data)...);
		}
		std::tuple<Ts const&...> operator*() const noexcept {
			return std::tuple<Ts const&...> {*std::get<value_element_type<Ts>>(m_Data)...};
		}
		pointer_type operator->() noexcept { return &m_Data; }
		const_pointer_type operator->() const noexcept { return &m_Data; }

		constexpr friend bool operator==(const indirect_pack_view_iterator_t& lhs,
										 const indirect_pack_view_iterator_t& rhs) noexcept {
			return std::get<0>(lhs.m_Data) == std::get<0>(rhs.m_Data);
		}
		constexpr friend bool operator!=(const indirect_pack_view_iterator_t& lhs,
										 const indirect_pack_view_iterator_t& rhs) noexcept {
			return std::get<0>(lhs.m_Data) != std::get<0>(rhs.m_Data);
		}

		constexpr friend bool operator<(const_reference_type lhs, const_reference_type rhs) noexcept {
			return std::get<0>(lhs.m_Data) < std::get<0>(rhs.m_Data);
		}
		constexpr friend bool operator>(const_reference_type lhs, const_reference_type rhs) noexcept {
			return std::get<0>(lhs.m_Data) > std::get<0>(rhs.m_Data);
		}
		constexpr friend bool operator<=(const_reference_type lhs, const_reference_type rhs) noexcept {
			return std::get<0>(lhs.m_Data) <= std::get<0>(rhs.m_Data);
		}
		constexpr friend bool operator>=(const_reference_type lhs, const_reference_type rhs) noexcept {
			return std::get<0>(lhs.m_Data) >= std::get<0>(rhs.m_Data);
		}

		constexpr indirect_pack_view_iterator_t& operator++() noexcept {
			[](auto&... iterators) { (void(++iterators), ...); }(std::get<value_element_type<Ts>>(m_Data)...);
			return *this;
		}

		constexpr indirect_pack_view_iterator_t operator--() noexcept {
			[](auto&... iterators) { (void(--iterators), ...); }(std::get<value_element_type<Ts>>(m_Data)...);
			return *this;
		}

		constexpr indirect_pack_view_iterator_t operator++(int) noexcept {
			auto tmp = *this;
			++(*this);
			return tmp;
		}

		constexpr indirect_pack_view_iterator_t operator--(int) noexcept {
			auto tmp = *this;
			--(*this);
			return tmp;
		}

		constexpr indirect_pack_view_iterator_t& operator+=(difference_type offset) noexcept {
			[](auto offset, auto&... iterators) {
				([offset](auto& iterator) { iterator += offset; }(iterators), ...);
			}(std::get<value_element_type<Ts>>(m_Data)...);
			return *this;
		}

		constexpr indirect_pack_view_iterator_t& operator-=(difference_type offset) noexcept {
			[](auto offset, auto&... iterators) {
				([offset](auto& iterator) { iterator -= offset; }(iterators), ...);
			}(std::get<value_element_type<Ts>>(m_Data)...);
			return *this;
		}
		constexpr indirect_pack_view_iterator_t operator+(difference_type offset) const noexcept {
			auto tmp = *this;
			tmp += offset;
			return tmp;
		}
		constexpr indirect_pack_view_iterator_t operator-(difference_type offset) {
			auto tmp = *this;
			tmp -= offset;
			return tmp;
		}

		value_type m_Data {};
	};
	template <typename IndiceType, typename... Ts>
	struct indirect_pack_view_t {
		using indice_type = IndiceType;
		template <typename Y>
		using _indice_tuple_machinery = psl::array<indice_type>;
		using indices_tuple_t		  = std::tuple<_indice_tuple_machinery<Ts>...>;


		template <typename... Ys>
		indirect_pack_view_t(Ys&&... data) : m_Data(std::forward<Ys>(data)...) {
// we hide the assert behind a check as the fold expression otherwise doesn't exist which leads to a compile error. This
// approach is cleaner without adding more machinery.
#if defined(PE_ASSERT)
			if constexpr((sizeof...(Ts)) > 0) {
				[&]<size_t... Indices>(std::index_sequence<Indices...>) {
					(void(psl_assert(std::get<Indices>(m_Data).size() == std::get<0>(m_Data).size(),
									 "Indices size mismatch in pack. Expected size '{}' but got '{}'",
									 std::get<0>(m_Data).size(),
									 std::get<Indices>(m_Data).size())),
					 ...);
				}
				(std::make_index_sequence<sizeof...(Ts)> {});
			}
#endif
		}

		indirect_pack_view_t() = default;

		template <size_t N>
		constexpr inline auto const& get() const noexcept {
			return std::get<N>(m_Data);
		}

		template <size_t N>
		constexpr inline auto get() noexcept {
			return std::get<N>(m_Data);
		}

		template <typename T>
		constexpr inline auto const& get() const noexcept {
			return std::get<indirect_array_t<T, indice_type>>(m_Data);
		}

		template <typename T>
		constexpr inline auto get() noexcept {
			return std::get<indirect_array_t<T, indice_type>>(m_Data);
		}

		constexpr inline auto size() const noexcept -> size_t {
			if constexpr(sizeof...(Ts) > 0) {
				return std::get<0>(m_Data).size();
			} else {
				return 0;
			}
		}

		constexpr inline auto empty() const noexcept -> bool {
			if constexpr(sizeof...(Ts) > 0) {
				return std::get<0>(m_Data).empty();
			} else {
				return true;
			}
		}

		constexpr inline auto operator[](size_t index) const noexcept {
			return indirect_pack_view_iterator_t(
			  std::next(std::get<indirect_array_t<Ts, indice_type>>(m_Data).begin(), index)...);
		}
		constexpr inline auto operator[](size_t index) noexcept {
			return indirect_pack_view_iterator_t(
			  std::next(std::get<indirect_array_t<Ts, indice_type>>(m_Data).begin(), index)...);
		}
		constexpr inline auto begin() noexcept {
			return indirect_pack_view_iterator_t(std::get<indirect_array_t<Ts, indice_type>>(m_Data).begin()...);
		}
		constexpr inline auto end() noexcept {
			return indirect_pack_view_iterator_t(std::get<indirect_array_t<Ts, indice_type>>(m_Data).end()...);
		}
		constexpr inline auto begin() const noexcept {
			return indirect_pack_view_iterator_t(std::get<indirect_array_t<Ts, indice_type>>(m_Data).begin()...);
		}
		constexpr inline auto end() const noexcept {
			return indirect_pack_view_iterator_t(std::get<indirect_array_t<Ts, indice_type>>(m_Data).end()...);
		}

		constexpr inline auto cbegin() const noexcept { return begin(); }
		constexpr inline auto cend() const noexcept { return end(); }
		std::tuple<indirect_array_t<Ts, indice_type>...> m_Data {};
	};

	template <typename IndiceType, typename... Ts>
	indirect_pack_view_t(indirect_array_t<Ts, IndiceType>...) -> indirect_pack_view_t<IndiceType, Ts...>;

	template <typename T>
	struct tuple_to_indirect_pack_view {};


	template <typename... Ts>
	struct tuple_to_indirect_pack_view<std::tuple<Ts...>> {
		using type = indirect_pack_view_t<psl::ecs::entity, Ts...>;
	};

	template <typename T>
	using tuple_to_indirect_pack_view_t = typename tuple_to_indirect_pack_view<T>::type;
}	 // namespace details


template <IsPolicy Policy, typename... Ts>
class view_t {
  public:
	using pack_t		= details::tuple_to_indirect_pack_view_t<typename details::typelist_to_tuple<Ts...>::type>;
	using filter_t		= typename details::typelist_to_pack<Ts...>::type;
	using combine_t		= typename details::typelist_to_combine_pack<Ts...>::type;
	using break_t		= typename details::typelist_to_break_pack<Ts...>::type;
	using add_t			= typename details::typelist_to_add_pack<Ts...>::type;
	using remove_t		= typename details::typelist_to_remove_pack<Ts...>::type;
	using except_t		= typename details::typelist_to_except_pack<Ts...>::type;
	using conditional_t = typename details::typelist_to_conditional_pack<Ts...>::type;
	using order_by_t	= typename details::typelist_to_orderby_pack<Ts...>::type;
	using policy_t		= Policy;
	static constexpr bool has_entities {std::disjunction<std::is_same<psl::ecs::entity, Ts>...>::value};

	static_assert(std::tuple_size<order_by_t>::value <= 1, "multiple order_by statements make no sense");

	constexpr view_t() = default;

	template <typename... Ys>
	constexpr view_t(Policy, Ys&&... data) : m_Pack(std::forward<Ys>(data)...) {}

	template <typename... Ys>
	constexpr view_t(Ys&&... data) : m_Pack(std::forward<Ys>(data)...) {}

	template <size_t N>
	constexpr inline auto const& get() const noexcept {
		return m_Pack.template get<N>();
	}

	template <size_t N>
	constexpr inline auto get() noexcept {
		return m_Pack.template get<N>();
	}

	template <typename T>
	constexpr inline auto const& get() const noexcept {
		return m_Pack.template get<T>();
	}

	template <typename T>
	constexpr inline auto get() noexcept {
		return m_Pack.template get<T>();
	}

	constexpr inline auto size() const noexcept -> size_t { return m_Pack.size(); }
	constexpr inline auto empty() const noexcept -> bool { return m_Pack.empty(); }
	constexpr inline auto const& operator[](size_t index) const noexcept { return m_Pack[index]; }
	constexpr inline auto& operator[](size_t index) noexcept { return m_Pack[index]; }
	constexpr inline auto begin() noexcept { return m_Pack.begin(); }
	constexpr inline auto end() noexcept { return m_Pack.end(); }
	constexpr inline auto begin() const noexcept { return m_Pack.begin(); }
	constexpr inline auto end() const noexcept { return m_Pack.end(); }
	constexpr inline auto cbegin() const noexcept { return begin(); }
	constexpr inline auto cend() const noexcept { return end(); }

  private:
	pack_t m_Pack {};
};

template <IsPolicy Policy, typename... Ts>
view_t(Policy, details::indirect_array_t<Ts, psl::ecs::entity>...) -> view_t<Policy, Ts...>;

template <typename... Ts>
using view_partial_t = view_t<psl::ecs::partial, Ts...>;

template <typename... Ts>
using view_full_t = view_t<psl::ecs::full, Ts...>;

template <IsPolicy Policy, IsAccessType Access, typename... Ts>
class pack_t : pack<Policy, Ts...> {
	using base_type = pack<Policy, Ts...>;

  public:
	template <typename... Ys>
	pack_t(Policy, Ys&&... values) : base_type(std::forward<Ys>(values)...) {}
};

template <IsPolicy Policy, typename... Ts>
class pack_t<Policy, indirect_t, Ts...> : public view_t<Policy, Ts...> {
	using base_type = view_t<Policy, Ts...>;

  public:
	template <typename... Ys>
	pack_t(Policy, Ys&&... values) : base_type(std::forward<Ys>(values)...) {}
};

template <IsPolicy Policy, typename... Ts>
pack_t(Policy, details::indirect_array_t<Ts, psl::ecs::entity>...) -> pack_t<Policy, indirect_t, Ts...>;

template <IsPolicy Policy, typename... Ts>
pack_t(Policy, psl::array_view<Ts>...) -> pack_t<Policy, direct_t, Ts...>;

template <IsPolicy Policy, typename... Ts>
pack_t(Policy, psl::array<Ts>...) -> pack_t<Policy, direct_t, Ts...>;
}	 // namespace psl::ecs
