#pragma once
#include "platform_def.hpp"
#include "psl/ustring.hpp"
#include "source_location.hpp"
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

	namespace details
	{
		template <typename... Args>
		struct last_type_pack
		{};

		template <typename Arg>
		struct last_type_pack<Arg>
		{
			using type = Arg;
		};

		template <typename First, typename... Args>
		requires(sizeof...(Args) >= 1) struct last_type_pack<First, Args...> : last_type_pack<Args...>
		{};

		template <typename... Args>
		using last_type_pack_t = typename last_type_pack<Args...>::type;

		template <typename... Args>
		concept HasSourceLocOverride =
		  (std::is_same_v<std::remove_cvref_t<last_type_pack_t<Args...>>, psl::source_location>);

		template <typename... Args>
		struct print_t
		{
/// \todo When Android implements source_location, remove the ifdefs and migrate to source_location
#if defined(PLATFORM_ANDROID)
			int android_log_level(level_t level) noexcept
			{
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
				return log_level;
			}
			print_t(level_t level, const char* func, const char* file, int line, const char* format, Args&&... args)
			{
				auto log_level = android_log_level(level);
				__android_log_write(
				  log_level, "paradigm", fmt::format(fmt::runtime(format), std::forward<Args>(args)...).c_str());
				__android_log_write(log_level, "paradigm", fmt::format("at: {} ({}:{})", func, file, line).c_str());
			}
#else
			print_t(level_t level,
					const char* fmt,
					Args&&... args,
					const psl::source_location& loc =
					  psl::source_location::current()) requires(!HasSourceLocOverride<Args...>)
			{
				internal_print(
				  level, fmt, std::forward_as_tuple(args...), std::make_index_sequence<sizeof...(Args)> {}, loc);
			}

			print_t(level_t level, const char* fmt, Args&&... args) requires(HasSourceLocOverride<Args...>)
			{
				internal_print(
				  level, fmt, std::forward_as_tuple(args...), std::make_index_sequence<sizeof...(Args) - 1> {});
			}

		  private:
			template <typename... Ys, size_t... Is>
			void
			internal_print(level_t level, const char* fmt, std::tuple<Ys&...> args, std::index_sequence<Is...> indices)
			{
				internal_print(level, fmt, args, indices, std::get<sizeof...(Ys) - 1>(args));
			}

			template <typename... Ys, size_t... Is>
			void internal_print(level_t level,
								const char* fmt,
								std::tuple<Ys&...> args,
								std::index_sequence<Is...> indices,
								const psl::source_location& loc)
			{
				const char* log_level;
				switch(level)
				{
				case level_t::verbose:
					log_level = "[verbose] {}\n    at: {} ({}:{}:{})";
					break;
				case level_t::debug:
					log_level = "[debug]   {}\n    at: {} ({}:{}:{})";
					break;
				case level_t::info:
					log_level = "[info]    {}\n    at: {} ({}:{}:{})";
					break;
				case level_t::warn:
					log_level = "[warn]    {}\n    at: {} ({}:{}:{})";
					break;
				case level_t::error:
					log_level = "[error]   {}\n    at: {} ({}:{}:{})";
					break;
				case level_t::fatal:
					log_level = "[fatal]   {}\n    at: {} ({}:{}:{})";
					break;
				default:
					log_level = "[info]    {}\n    at: {} ({}:{}:{})";
				}
				fmt::print(
				  fmt::runtime(fmt::format(
					fmt::runtime(log_level), fmt, loc.function_name(), loc.file_name(), loc.line(), loc.column())),
				  std::get<Is>(args)...);
			}
#endif
		};

#if defined(PLATFORM_ANDROID)
		template <typename... Ts>
		print_t(level_t, const char*, const char*, int, const char*, Ts&&...) -> print_t<Ts...>;
#else
		template <typename... Ts>
		print_t(level_t, const char*, Ts&&...) -> print_t<Ts...>;
#endif
	}	 // namespace details
}	 // namespace psl

#if defined(PLATFORM_ANDROID)
	#define psl_print(level, message, ...)                                                                             \
		psl::details::print_t { level, __PRETTY_FUNCTION__, __FILE__, __LINE__, message, __VA_ARGS__ }
#else
	#define psl_print(level, message, ...)                                                                             \
		psl::details::print_t { level, message, __VA_ARGS__ }
#endif
namespace psl
{
	[[noreturn]] inline void unreachable(const std::string& reason = "")
	{
		if(reason.empty())
			psl_print(level_t::fatal, "unreachable code reached.");
		else
			psl_print(level_t::fatal, "{}", reason);
		std::terminate();
	}

	[[noreturn]] inline void not_implemented(size_t issue = 0)
	{
		if(issue != 0)
			psl_print(level_t::fatal,
					  "feature not implemented, follow development at https://github.com/JessyDL/paradigm/issues/{}",
					  issue);
		else
			psl_print(level_t::fatal, "feature not implemented");
		std::terminate();
	}

	[[noreturn]] inline void not_implemented(const std::string& reason, size_t issue = 0)
	{
		if(issue != 0)
			psl_print(level_t::fatal,
					  "feature not implemented reason: '{}', follow development at "
					  "https://github.com/JessyDL/paradigm/issues/{}",
					  reason,
					  issue);
		else
			psl_print(level_t::fatal, "feature not implemented reason: '{}'", reason);
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
	#define psl_assert(expression, ...)                                                                                \
		(void)((!!(expression)) || (psl_print(psl::level_t::fatal, __VA_ARGS__), 0) || (std::terminate(), 0))
#else
	#define psl_assert(expression, ...)
#endif
