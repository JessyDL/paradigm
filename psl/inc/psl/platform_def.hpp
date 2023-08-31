#pragma once

#ifndef PE_PLATFORM_WINDOWS
typedef unsigned long DWORD;	//-V677
typedef unsigned short WORD;	//-V677
typedef unsigned int UNINT32;
typedef void* HANDLE;	 //-V677
// typedef unsigned __int64 ULONG64, *PULONG64; //-V677
// typedef unsigned __int64 DWORD64, *PDWORD64; //-V677
#endif


// namespace psl
//{
// #if defined(_WIN64)
//	using size_t = long long unsigned int;
//	using ptrdiff_t = long long int;
// #elif defined(_WIN32)
//	using size_t = unsigned int;
//	using ptrdiff_t = int;
// #elif defined (__linux__) && defined(__SIZE_TYPE__) && defined(__PTRDIFF_TYPE__)
//	using size_t = __SIZE_TYPE__;
//	using ptrdiff_t = __PTRDIFF_TYPE__;
// #else
// #	error unknown platform
// #endif
//}

// using ptrdiff_t = psl::ptrdiff_t;


/// \brief platform specific utilities that help in identifying the current platform, and various specifications of it.
namespace psl::utility::platform {
/// \brief contains all known, supported (or to be supported) platforms.
/// \note we need to add an extra dash to linux as the define already exists on linux platforms
enum class platform_t { UNKNOWN = 0, linux = 1, macos = 2, windows = 3, android = 4, ios = 5, web = 6 };

/// \brief encoding enum, that can be used to help identify encoding of items.
enum class encoding_t { UNKNOWN = 0, UTF8 = 1, UTF16 = 2 };
#if defined(PE_PLATFORM_WINDOWS)
/// \brief contains what platform this current is.
constexpr platform_t platform = platform_t::windows;
/// \brief contains the UTF-ness of the current platform.
constexpr encoding_t encoding = encoding_t::UTF16;
#elif defined(PE_PLATFORM_IOS)
constexpr platform_t platform = platform_t::ios;
constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PE_PLATFORM_MACOS)
constexpr platform_t platform = platform_t::macos;
constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PE_PLATFORM_ANDROID)
constexpr platform_t platform = platform_t::android;
constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PE_PLATFORM_LINUX)
constexpr platform_t platform = platform_t::linux;
constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PE_PLATFORM_WEB)
constexpr platform_t platform = platform_t::web;
constexpr encoding_t encoding = encoding_t::UTF8;
#else
	#error not supported
#endif

/// \brief contains all known, supported (or to be supported) architectures.
enum class architecture_t { UNKNOWN = 0, x86, x86_64, ARM64, WASM };

/// \brief the architecture of the current platform.
constexpr architecture_t architecture =
#if defined(PE_ARCHITECTURE_X86_64)
  architecture_t::x86_64
#elif defined(PE_ARCHITECTURE_X86)
  architecture_t::x86
#elif defined(PE_ARCHITECTURE_ARM64)
  architecture_t::ARM64
#elif defined(PE_ARCHITECTURE_WASM)
  architecture_t::WASM
#else
  architecture_t::UNKNOWN
#endif
  ;
}	 // namespace psl::utility::platform


#ifdef _MSC_VER
	#define FORCEINLINE __forceinline
#else
	#define FORCEINLINE __attribute__((always_inline))
#endif
