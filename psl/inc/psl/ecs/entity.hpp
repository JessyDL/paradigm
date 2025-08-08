#pragma once
#include "psl/array.hpp"
#include <cstdint>
#include <type_traits>

namespace psl::ecs {
class state_t;
/// \brief tag type to circumvent constructing an object in the backing data
template <typename T>
struct empty {};

struct entity_t;

namespace details {
	using entity_size_type = std::uint32_t;
	template <typename T>
	struct is_empty_container : std::false_type {};

	template <typename T>
	struct is_empty_container<psl::ecs::empty<T>> : std::true_type {};

	template <typename T>
	struct empty_container {
		using type = void;
	};

	template <typename T>
	struct empty_container<psl::ecs::empty<T>> {
		using type = T;
	};

	template <typename T>
	struct is_range_t : std::false_type {};

	template <typename T>
	struct is_range_t<psl::array<T>> : std::true_type {
		using type = T;
	};

	constexpr inline entity_t make_entity(entity_size_type value) noexcept;
	constexpr inline entity_size_type get_value(entity_t entity) noexcept;
}	 // namespace details

/// ----------------------------------------------------------------------------------------------
/// Entity
/// ----------------------------------------------------------------------------------------------
// struct entity;

/// \brief entity points to a collection of components
struct entity_t {
	friend struct std::hash<entity_t>;
	friend class psl::ecs::state_t;
	// edit this value for smaller or larger entities.
	using size_type												   = details::entity_size_type;
	static constexpr size_type INVALID_ENTITY_VALUE				   = std::numeric_limits<size_type>::max();
	constexpr entity_t()										   = default;
	constexpr entity_t(const entity_t& entity) noexcept			   = default;
	constexpr entity_t& operator=(const entity_t& entity) noexcept = default;
	constexpr entity_t(entity_t&& entity) noexcept				   = default;
	constexpr entity_t& operator=(entity_t&& entity) noexcept	   = default;

	explicit constexpr inline operator size_type const&() const noexcept {
		return m_Value;
	}

	constexpr inline friend bool operator==(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.m_Value == rhs.m_Value;
	}
	constexpr inline friend bool operator!=(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.m_Value != rhs.m_Value;
	}
	constexpr inline friend bool operator<(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.m_Value < rhs.m_Value;
	}
	constexpr inline friend bool operator>(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.m_Value > rhs.m_Value;
	}
	constexpr inline friend bool operator<=(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.m_Value <= rhs.m_Value;
	}
	constexpr inline friend bool operator>=(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.m_Value >= rhs.m_Value;
	}

	explicit constexpr inline operator bool() const noexcept {
		return m_Value != INVALID_ENTITY_VALUE;
	}

	constexpr inline bool valid() const noexcept {
		return m_Value != INVALID_ENTITY_VALUE;
	}

	constexpr inline size_type value() const noexcept {
		return m_Value;
	}

	friend constexpr inline entity_t details::make_entity(details::entity_size_type value) noexcept;
	friend constexpr inline details::entity_size_type details::get_value(entity_t entity) noexcept;

  private:
	constexpr entity_t(size_type entity_value) noexcept : m_Value(entity_value) {}
	size_type m_Value {INVALID_ENTITY_VALUE};
};

static constexpr entity_t invalid_entity {};

template <typename T>
concept IsEntity = std::is_same_v<std::remove_cvref_t<T>, entity_t>;

namespace details {
	constexpr inline entity_t make_entity(entity_size_type value) noexcept {
		return entity_t(value);
	}

	constexpr inline entity_size_type get_value(entity_t entity) noexcept {
		return entity.m_Value;
	}
}	 // namespace details
}	 // namespace psl::ecs


namespace std {
template <>
struct hash<psl::ecs::entity_t> {
	size_t operator()(const psl::ecs::entity_t& entity) const noexcept {
		return std::hash<psl::ecs::entity_t::size_type>()(entity.m_Value);
	}
};
}	 // namespace std
