#pragma once
#include "psl/details/fixed_astring.hpp"
#include "strtype/strtype.hpp"
#include <cstdint>

namespace psl::ecs {

/// \brief specialize this trait to change the serialization name of a component (also includes the debug name)
template <typename T>
struct component_trait_name_t {
	static constexpr auto name {strtype::stringify_typename<T>()};
};

/// \brief allows you to inject a version tag for a given component
template <typename T>
struct component_trait_version_t {
	static constexpr size_t version {0};
};

/// \brief controls wether a component can be serialized
/// \note unlike the other traits this one can be overriden at runtime, this
/// only influences the default behaviour when an ecs state object is created.
template <typename T>
struct component_trait_serializable_t {
	static constexpr bool serializable {false};
};

// keep the indices stable as this enum will be serialized
enum class component_mutability_behaviour_t : std::uint8_t {
	unrestricted = 0,
	restricted	 = 1,
};


/// \brief trait to specify a component that has restricted mutability
///
/// Restricted mutability components can only be modified through mutation instruction events. They are
/// handled at the start of the system ticks before the user-defined systems are executed.
/// Additionally any modifications during the system tick will not be applied to the component till
/// the next tick. This allows you to have a component that is guaranteed to be in a consistent state
/// when the system tick is executed.
/// This is useful for components that are owned by a system, and the system needs to have final say
/// on the data. It is also useful for components that are used in a system that needs to
/// react to changes in the data.
///
/// \example the audio system must be able to react to changes in f.e. volume in a performant manner
/// this allows it to filter only the audio components that have been known to have mutated since
/// the last observation of the system.
///
/// \warning although the name of the filter can be ambiguous, the `on_mutate` will not fire for
/// mutations that are not done through a mutation instruction.
template <typename T>
struct component_trait_mutability_t {
	static constexpr component_mutability_behaviour_t mutability {component_mutability_behaviour_t::unrestricted};
};

namespace details {
	template <typename T>
	struct component_traits_impl_t : public component_trait_name_t<T>,
									 public component_trait_serializable_t<T>,
									 public component_trait_version_t<T>,
									 public component_trait_mutability_t<T> {};
}	 // namespace details

/// \warning do not specialize, specialize the individual traits instead so as to preserve future compatibility
template <typename T>
constexpr auto component_traits_v = details::component_traits_impl_t<T> {};

/// \warning do not specialize, specialize the individual traits instead so as to preserve future compatibility
template <typename T>
using component_traits_t = details::component_traits_impl_t<T>;

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

template <typename T>
concept IsRestrictedMutable =
  component_trait_mutability_t<T>::mutability != component_mutability_behaviour_t::unrestricted;

template <typename T>
concept IsUnrestrictedMutable =
  component_trait_mutability_t<T>::mutability == component_mutability_behaviour_t::unrestricted;

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
			component_traits_t<T>::prototype()
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
			return component_traits_t<T>::prototype();
		} else if constexpr(HasComponentMemberPrototypeDefinition<T>) {
			return T::prototype();
		} else {
			return T {};
		}
	}

}	 // namespace details
}	 // namespace psl::ecs
