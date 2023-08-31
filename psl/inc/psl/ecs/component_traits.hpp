#pragma once
#include "psl/details/fixed_astring.hpp"
#include "strtype/strtype.hpp"
#include <cstdint>

namespace psl::ecs {
/// \brief Specialize this type to override default behaviours, or to enable serialization
/// \note `serializable` controls the default behaviour. Ineligible types (complex component types) will throw compile errors if you try to force serialization.
template <typename T>
struct component_traits {
	static constexpr bool serializable {false};
	static constexpr auto name {strtype::stringify_typename<T>()};
};

enum class component_type : std::uint8_t {
	COMPLEX = 0,
	TRIVIAL = 1,
	FLAG	= 2,
};

template <typename T>
concept IsComponentFlagType = std::is_empty_v<T>;

template <typename T>
concept IsComponentTrivialType = std::is_trivial_v<T> && !IsComponentFlagType<T>;

template <typename T>
concept IsComponentComplexType = !IsComponentTrivialType<T> && !IsComponentFlagType<T>;

template <typename T>
concept IsComponentTypeSerializable = IsComponentTrivialType<T> || IsComponentFlagType<T>;

/// \warning Do not specialize this.
template <typename T>
static constexpr auto component_type_v = IsComponentFlagType<T>		 ? component_type::FLAG
										 : IsComponentTrivialType<T> ? component_type::TRIVIAL
																	 : component_type::COMPLEX;

namespace details {
	template <typename T>
	concept HasComponentTraitsPrototypeDefinition = requires() {
#if !defined(PE_PLATFORM_ANDROID)
		{
#endif
			component_traits<T>::prototype()
#if !defined(PE_PLATFORM_ANDROID)
			} -> std::same_as<T>
#endif
			  ;
	};

	template <typename T>
	concept HasComponentMemberPrototypeDefinition = requires() {
#if !defined(PE_PLATFORM_ANDROID)
		{
#endif
			T::prototype()
#if !defined(PE_PLATFORM_ANDROID)
			} -> std::same_as<T>
#endif
			  ;
	};

	template <typename T>
	concept HasComponentTypePrototypeDefinition =
	  HasComponentTraitsPrototypeDefinition<T> || HasComponentMemberPrototypeDefinition<T>;

	/// \brief Designates types that either _need_ to be constructed, or that have a prototype definition that override their normal operations.
	template <typename T>
	concept DoesComponentTypeNeedPrototypeCall = IsComponentComplexType<T> || HasComponentTypePrototypeDefinition<T>;

	template <DoesComponentTypeNeedPrototypeCall T>
	FORCEINLINE static constexpr auto prototype_for() -> T {
		if constexpr(HasComponentTraitsPrototypeDefinition<T>) {
			return component_traits<T>::prototype();
		} else if constexpr(HasComponentMemberPrototypeDefinition<T>) {
			return T::prototype();
		} else {
			return T {};
		}
	}

}	 // namespace details
}	 // namespace psl::ecs
