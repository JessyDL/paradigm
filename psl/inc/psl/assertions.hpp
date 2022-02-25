#pragma once
#include "psl/ustring.hpp"
#include <fmt/format.h>

#ifdef PLATFORM_ANDROID
	#include <android/log.h>
#endif	  // PLATFORM_ANDROID

namespace psl
{
	enum class level_t
	{
		verbose,
		debug,
		info,
		warn,
		error,
		fatal,
	};
	template <typename... Args>
	constexpr void print(level_t level, const char* fmt = "failed assert", Args&&... args)
	{
#ifdef PLATFORM_ANDROID
		int log_level = ANDROID_LOG_SILENT;
		switch(level)
		{
		case level_t::verbose:
			log_level = ANDROID_LOG_VERBOSE;
			break;
		case level_t::debug:
			log_level = ANDROID_LOG_DEBUG;
			break;
		case level_t::info:
			log_level = ANDROID_LOG_INFO;
			break;
		case level_t::warn:
			log_level = ANDROID_LOG_WARN;
			break;
		case level_t::error:
			log_level = ANDROID_LOG_ERROR;
			break;
		case level_t::fatal:
			log_level = ANDROID_LOG_FATAL;
			break;
		default:
			log_level = ANDROID_LOG_SILENT;
		}
		__android_log_write(log_level, "paradigm", fmt::format(fmt, std::forward<Args>(args)...).c_str());
#else
		const char* log_level;
		switch(level)
		{
		case level_t::verbose:
			log_level = "[verbose] {}";
			break;
		case level_t::debug:
			log_level = "[debug]   {}";
			break;
		case level_t::info:
			log_level = "[info]    {}";
			break;
		case level_t::warn:
			log_level = "[warn]    {}";
			break;
		case level_t::error:
			log_level = "[error]   {}";
			break;
		case level_t::fatal:
			log_level = "[fatal]   {}";
			break;
		default:
			log_level = "[info]    {}";
		}
		fmt::print(fmt::format(log_level, fmt), std::forward<Args>(args)...);
#endif
	}

	[[noreturn]] constexpr void unreachable(const std::string& reason = "")
	{
		if(reason.empty())
			print(level_t::fatal, "unreachable code reached.");
		else
			print(level_t::fatal, "{}", reason);
		std::terminate();
	}

	[[noreturn]] constexpr void not_implemented(size_t issue = 0)
	{
		if(issue != 0)
			print(level_t::fatal,
				  "feature not implemented, follow development at https://github.com/JessyDL/paradigm/issues/{}",
				  issue);
		else
			print(level_t::fatal, "feature not implemented");
		std::terminate();
	}

	[[noreturn]] constexpr void not_implemented(const std::string& reason, size_t issue = 0)
	{
		if(issue != 0)
			print(level_t::fatal,
				  "feature not implemented reason: '{}', follow development at "
				  "https://github.com/JessyDL/paradigm/issues/{}",
				  reason,
				  issue);
		else
			print(level_t::fatal, "feature not implemented reason: '{}'", reason);
		std::terminate();
	}
}	 // namespace psl


#if defined(HEDLEY_ALWAYS_INLINE)
	#define DBG__ALWAYS_INLINE HEDLEY_ALWAYS_INLINE
#elif defined(__GNUC__) && (__GNUC__ >= 4)
	#define DBG__ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1200)
	#define DBG__ALWAYS_INLINE __forceinline
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
	#define DBG__ALWAYS_INLINE inline
#else
	#define DBG__ALWAYS_INLINE
#endif

#define DBG__FUNCTION static DBG__ALWAYS_INLINE

#if defined(PLATFORM_ANDROID)
	#define debug_break()
#else
	#if defined(__has_builtin) && !defined(__ibmxl__)
		#if __has_builtin(__builtin_debugtrap)
			#define debug_break() __builtin_debugtrap()
		#elif __has_builtin(__debugbreak)
			#define debug_break() __debugbreak()
		#endif
	#endif
	#if !defined(debug_break)
		#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
			#define debug_break() __debugbreak()
		#elif defined(__ARMCC_VERSION)
			#define debug_break() __breakpoint(42)
		#elif defined(__ibmxl__) || defined(__xlC__)
			#include <builtins.h>
			#define debug_break() __trap(42)
		#elif defined(__DMC__) && defined(_M_IX86)
DBG__FUNCTION void debug_break(void) { __asm int 3h; }
		#elif defined(__i386__) || defined(__x86_64__)
DBG__FUNCTION void debug_break(void) { __asm__ __volatile__("int $03"); }
		#elif defined(__thumb__)
DBG__FUNCTION void debug_break(void) { __asm__ __volatile__(".inst 0xde01"); }
		#elif defined(__aarch64__)
DBG__FUNCTION void debug_break(void) { __asm__ __volatile__(".inst 0xd4200000"); }
		#elif defined(__arm__)
DBG__FUNCTION void debug_break(void) { __asm__ __volatile__(".inst 0xe7f001f0"); }
		#elif defined(__alpha__) && !defined(__osf__)
DBG__FUNCTION void debug_break(void) { __asm__ __volatile__("bpt"); }
		#elif defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 0) && defined(__GNUC__)
			#define debug_break() __builtin_trap()
		#else
			#include <signal.h>
			#if defined(SIGTRAP)
				#define debug_break() raise(SIGTRAP)
			#else
				#define debug_break() raise(SIGABRT)
			#endif
		#endif
	#endif
#endif
#if defined(HEDLEY_LIKELY)
	#define DBG_LIKELY(expr) HEDLEY_LIKELY(expr)
#elif defined(__GNUC__) && (__GNUC__ >= 3)
	#define DBG_LIKELY(expr) __builtin_expect(!!(expr), 1)
#else
	#define DBG_LIKELY(expr) (!!(expr))
#endif

#if defined(PE_DEBUG)
#define psl_assert(expression, ...)                                                                            \
			(void)((!!(expression)) || (psl::print(psl::level_t::fatal, __VA_ARGS__), 0) || (std::terminate(), 0))
#else
#define psl_assert(expression, ...)
#endif