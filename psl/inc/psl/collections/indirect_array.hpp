#pragma once
#include "psl/array.hpp"
#include <cstdint>

namespace psl {
template <typename T, typename SizeType = size_t>
class indirect_array_iterator_t;

template <typename T>
struct is_indirect_array_iterator : std::false_type {};

template <typename T, typename SizeType>
struct is_indirect_array_iterator<indirect_array_iterator_t<T, SizeType>> : std::true_type {};

template <typename T, typename Y>
struct is_compatible_indirect_array_iterator : std::false_type {};

template <typename T, typename Y, typename SizeType>
requires(std::is_same_v<std::remove_const_t<T>, std::remove_const_t<Y>>) struct is_compatible_indirect_array_iterator<
  indirect_array_iterator_t<T, SizeType>,
  indirect_array_iterator_t<Y, SizeType>> : std::true_type {};

template <typename T, typename SizeType>
class indirect_array_iterator_t {
  private:
	static constexpr bool _is_const = std::is_const_v<T>;

	// give access to the const/non-const version of itself so we can have the logical operators access internals.
	friend class indirect_array_iterator_t<
	  std::conditional_t<_is_const, std::remove_cvref_t<T>, std::remove_cvref_t<T> const>,
	  SizeType>;

  public:
	using size_type			   = SizeType;
	using value_type		   = std::remove_cvref_t<T>;
	using const_value_type	   = value_type const;
	using reference_type	   = value_type&;
	using const_reference_type = value_type const&;
	using pointer_type		   = value_type*;
	using const_pointer_type   = value_type const*;

	using difference_type		   = std::ptrdiff_t;
	using unpack_iterator_category = std::random_access_iterator_tag;


	indirect_array_iterator_t(size_type* it, pointer_type data) : m_It(it), m_Data(data) {}
	indirect_array_iterator_t(size_type* it, const_pointer_type data) requires(_is_const)
		: m_It(it), m_Data(const_cast<pointer_type>(data)) {}
	indirect_array_iterator_t(size_type const* it, pointer_type data)
		: m_It(const_cast<size_type*>(it)), m_Data(data) {}

	indirect_array_iterator_t(size_type const* it, const_pointer_type data) requires(_is_const)
		: m_It(const_cast<size_type*>(it)), m_Data(const_cast<pointer_type>(data)) {}

	reference_type operator*() noexcept requires(!_is_const) { return m_Data[*m_It]; }
	const_reference_type operator*() const noexcept { return m_Data[*m_It]; }
	pointer_type operator->() noexcept requires(!_is_const) { return &(m_Data[*m_It]); }
	const_pointer_type operator->() const noexcept { return &m_Data[*m_It]; }

	template <typename Y>
	requires(is_compatible_indirect_array_iterator<indirect_array_iterator_t, Y>::value) constexpr friend bool
	operator==(indirect_array_iterator_t const& lhs, Y const& rhs) noexcept {
		return lhs.m_It == rhs.m_It && lhs.m_Data == rhs.m_Data;
	}

	template <typename Y>
	requires(is_compatible_indirect_array_iterator<indirect_array_iterator_t, Y>::value) constexpr friend bool
	operator!=(indirect_array_iterator_t const& lhs, Y const& rhs) noexcept {
		return lhs.m_It != rhs.m_It || lhs.m_Data != rhs.m_Data;
	}

	template <typename Y>
	requires(is_compatible_indirect_array_iterator<indirect_array_iterator_t, Y>::value) constexpr friend bool
	operator<(indirect_array_iterator_t const& lhs, Y const& rhs) noexcept {
		return *lhs.m_It < *rhs.m_It;
	}

	template <typename Y>
	requires(is_compatible_indirect_array_iterator<indirect_array_iterator_t, Y>::value) constexpr friend bool
	operator>(indirect_array_iterator_t const& lhs, Y const& rhs) noexcept {
		return *lhs.m_It > *rhs.m_It;
	}

	template <typename Y>
	requires(is_compatible_indirect_array_iterator<indirect_array_iterator_t, Y>::value) constexpr friend bool
	operator<=(indirect_array_iterator_t const& lhs, Y const& rhs) noexcept {
		return *lhs.m_It <= *rhs.m_It;
	}

	template <typename Y>
	requires(is_compatible_indirect_array_iterator<indirect_array_iterator_t, Y>::value) constexpr friend bool
	operator>=(indirect_array_iterator_t const& lhs, Y const& rhs) noexcept {
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

  private:
	size_type* m_It {};
	pointer_type m_Data {nullptr};
};

template <typename T, typename SizeType = size_t>
class indirect_array_t {
  private:
	static constexpr bool _is_const = std::is_const_v<T>;

  public:
	using size_type			 = SizeType;
	using value_type		 = std::remove_cvref_t<T>;
	using const_value_type	 = value_type const;
	using pointer_type		 = value_type*;
	using const_pointer_type = value_type const*;

	using iterator_type		  = indirect_array_iterator_t<value_type, size_type>;
	using const_iterator_type = indirect_array_iterator_t<const_value_type, size_type>;

	template <typename Y>
	indirect_array_t(Y&& indices, pointer_type data) : m_Indices(std::forward<Y>(indices)), m_Data(data) {}
	template <typename Y>
	indirect_array_t(Y&& indices, const_pointer_type data) requires(_is_const)
		: m_Indices(std::forward<Y>(indices)), m_Data(const_cast<pointer_type>(data)) {}
	indirect_array_t() = default;

	constexpr inline auto& operator[](size_t index) noexcept requires(!_is_const) {
		return *(m_Data + m_Indices[index]);
	}
	constexpr inline auto const& operator[](size_t index) const noexcept { return *(m_Data + m_Indices[index]); }
	constexpr inline auto& at(size_t index) noexcept requires(!_is_const) { return *(m_Data + m_Indices[index]); }
	constexpr inline auto const& at(size_t index) const noexcept { return *(m_Data + m_Indices[index]); }

	constexpr inline auto size() const noexcept { return m_Indices.size(); }
	constexpr inline auto begin() noexcept requires(!_is_const) { return iterator_type(m_Indices.data(), m_Data); }
	constexpr inline auto end() noexcept requires(!_is_const) {
		return iterator_type(m_Indices.data() + m_Indices.size(), m_Data);
	}
	constexpr inline auto begin() const noexcept { return const_iterator_type(m_Indices.data(), m_Data); }
	constexpr inline auto end() const noexcept {
		return const_iterator_type(m_Indices.data() + m_Indices.size(), m_Data);
	}
	constexpr inline auto cbegin() const noexcept { return begin(); }
	constexpr inline auto cend() const noexcept { return end(); }
	constexpr inline auto empty() const noexcept -> bool { return size() == 0; }

	constexpr inline auto data() noexcept -> pointer_type requires(!_is_const) { return m_Data; }
	constexpr inline auto data() const noexcept -> const_pointer_type { return m_Data; }
	constexpr inline auto cdata() const noexcept -> const_pointer_type { return m_Data; }

  private:
	psl::array<size_type> m_Indices {};
	pointer_type m_Data {};
};

template <typename T, typename IndiceType, template <typename, typename...> typename Container, typename... Rest>
indirect_array_t(Container<IndiceType, Rest...>, T*) -> indirect_array_t<T, IndiceType>;
}	 // namespace psl
