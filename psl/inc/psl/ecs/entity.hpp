#pragma once
#include "psl/array.hpp"
#include <cstdint>
#include <type_traits>

namespace psl::ecs {
/// \brief tag type to circumvent constructing an object in the backing data
template <typename T>
struct empty {};

namespace details {
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
}	 // namespace details

/// ----------------------------------------------------------------------------------------------
/// Entity
/// ----------------------------------------------------------------------------------------------
// struct entity;

/// \brief entity points to a collection of components
struct entity_t {
	// edit this value for smaller or larger entities.
	using size_type = uint32_t;
	constexpr entity_t() = default;
	constexpr entity_t(size_type value) noexcept : value(value) {}
	constexpr entity_t(const entity_t& entity) noexcept			   = default;
	constexpr entity_t& operator=(const entity_t& entity) noexcept = default;
	constexpr entity_t(entity_t&& entity) noexcept				   = default;
	constexpr entity_t& operator=(entity_t&& entity) noexcept	   = default;

	explicit constexpr inline operator size_type const&() const noexcept { return value; }
	explicit constexpr inline operator size_type&() noexcept { return value; }

	constexpr inline friend bool operator==(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.value == rhs.value;
	}
	constexpr inline friend bool operator!=(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.value != rhs.value;
	}

	constexpr inline operator bool() const noexcept { return value != 0; }

	size_type value {};
};

static constexpr entity_t invalid_entity {0};

template <typename T>
concept IsEntity = std::is_same_v<std::remove_cvref_t<T>, entity_t>;
}	 // namespace psl::ecs
