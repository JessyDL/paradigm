#pragma once
#include "psl/ustring.hpp"
#include <cassert>


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

#if !defined(PE_DEBUG) || (PE_DEBUG == 0)
#define assert_debug_break(condition)                                                                                  \
	if(!(condition))                                                                                                   \
	{                                                                                                                  \
		debug_break();                                                                                                 \
	}
#else
#define assert_debug_break(condition)
#endif

#ifdef assert
#undef assert
#ifdef PE_DEBUG

#define assert(expression, ...) ((void)0)

#else

#define assert(expression, ...) (void)((!!(expression)) || (debug_break(), 0))
#endif
#endif

#ifdef PE_DEBUG
#ifdef PLATFORM_WINDOWS
#define psl_assert(expression, message)                                                                                \
	(void)((!!(expression)) ||                                                                                         \
		   (_wassert((wchar_t*)(psl::string8::to_string16_t(message + " in expression:" + #expression)).data(),        \
					 _CRT_WIDE(__FILE__),                                                                              \
					 (unsigned)(__LINE__)),                                                                            \
			(debug_break(), 0))
#else
#define psl_assert(expression, message) assert(expression, message)
#endif
#else
#define psl_assert(expression, message)
#endif

#if !defined(PE_DEBUG) || (PE_DEBUG == 0)
#define dbg_assert(expr)                                                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		if(!DBG_LIKELY(expr))                                                                                          \
		{                                                                                                              \
			trap();                                                                                                    \
		}                                                                                                              \
	} while(0)
#else
#define dbg_assert(expr)
#endif