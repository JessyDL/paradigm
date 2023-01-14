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
using entity_size_type = uint32_t;

// enum class entity_t : entity_size_type {};

struct entity_t {
	constexpr entity_t() = default;
	constexpr entity_t(entity_size_type value) noexcept : value(value) {}
	constexpr entity_t(const entity_t& entity) noexcept			   = default;
	constexpr entity_t& operator=(const entity_t& entity) noexcept = default;
	constexpr entity_t(entity_t&& entity) noexcept				   = default;
	constexpr entity_t& operator=(entity_t&& entity) noexcept	   = default;

	explicit constexpr inline operator entity_size_type const&() const noexcept { return value; }
	explicit constexpr inline operator entity_size_type&() noexcept { return value; }

	constexpr inline friend bool operator==(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.value == rhs.value;
	}
	constexpr inline friend bool operator!=(entity_t const& lhs, entity_t const& rhs) noexcept {
		return lhs.value != rhs.value;
	}

	entity_size_type value {};
};

template <typename T>
concept IsEntity = std::is_same_v<std::remove_cvref_t<T>, entity_t>;

/// \brief checks if an entity is valid or not
/// \param[in] e the entity to check
static constexpr bool valid(entity_t e) noexcept {
	return static_cast<entity_size_type>(e) != entity_size_type {0};
};

}	 // namespace psl::ecs
