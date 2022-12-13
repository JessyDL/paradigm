#pragma once
#include "platform_def.hpp"

#if !defined(PLATFORM_ANDROID) && !defined(__clang__)
	#include <source_location>
#else
	#include <cstdint>
#endif

namespace psl
{
#if defined(PLATFORM_ANDROID) || defined(__clang__)
/// \todo When Android implements source_location, remove
class source_location
{
  public:
	constexpr source_location() noexcept {}
	source_location(const source_location&) {}
	source_location(source_location&&) noexcept {}

	static constexpr source_location current() noexcept { return {}; }
	constexpr std::uint_least32_t line() const noexcept { return 0; }
	constexpr std::uint_least32_t column() const noexcept { return 0; }
	constexpr const char* file_name() const noexcept { return ""; }
	constexpr const char* function_name() const noexcept { return ""; }
};
#else
using source_location = std::source_location;
#endif
}	 // namespace psl
