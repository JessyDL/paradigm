#pragma once

#if !defined(PLATFORM_ANDROID)
	#include <execution>
namespace psl::ecs::execution {
struct no_exec {};
using sequenced_policy			  = std::execution::sequenced_policy;
using parallel_policy			  = std::execution::parallel_policy;
using parallel_unsequenced_policy = std::execution::parallel_unsequenced_policy;
using unsequenced_policy		  = std::execution::unsequenced_policy;

inline constexpr sequenced_policy seq {};
inline constexpr parallel_policy par {};
inline constexpr parallel_unsequenced_policy par_unseq {};
inline constexpr unsequenced_policy unseq {};
}	 // namespace psl::ecs::execution
#else
namespace psl::ecs::execution {
struct no_exec {};
struct sequenced_policy {};
struct parallel_policy {};
struct parallel_unsequenced_policy {};
struct unsequenced_policy {};

inline constexpr no_exec seq {};
inline constexpr no_exec par {};
inline constexpr no_exec par_unseq {};
inline constexpr no_exec unseq {};
}	 // namespace psl::ecs::execution
#endif
