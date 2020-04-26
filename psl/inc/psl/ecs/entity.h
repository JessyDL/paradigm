#pragma once
#include <cstdint>
#include <type_traits>
#include "psl/array.h"

namespace psl::ecs
{
	/// \brief tag type to circumvent constructing an object in the backing data
	template <typename T>
	struct empty
	{};

	namespace details
	{
		template<typename T>
		struct is_empty_container : std::false_type
		{};

		template<typename T>
		struct is_empty_container<psl::ecs::empty<T>>	:	std::true_type
		{};

		template<typename T>
		struct empty_container
		{
			using type = void;
		};

		template<typename T>
		struct empty_container<psl::ecs::empty<T>>
		{
			using type = T;
		};

		template<typename T>
		struct is_range_t : std::false_type
		{};

		template<typename T>
		struct is_range_t<psl::array<T>> : std::true_type
		{
			using type = T;
		};
	}

	/// ----------------------------------------------------------------------------------------------
	/// Entity
	/// ----------------------------------------------------------------------------------------------
	//struct entity;

	/// \brief entity points to a collection of components
	using entity = uint32_t;

	/// \brief checks if an entity is valid or not
	/// \param[in] e the entity to check
	static bool valid(entity e) noexcept
	{
		return e != 0u;
	};

}
