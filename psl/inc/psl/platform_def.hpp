#pragma once

#if !defined(PLATFORM_WINDOWS) && !defined(PLATFORM_IOS) && !defined(PLATFORM_OSX) && !defined(PLATFORM_ANDROID) &&    \
  !defined(PLATFORM_LINUX) && !defined(PLATFORM_POSIX)
#ifdef _WIN64
#define PLATFORM_WINDOWS
#elif _WIN32
#define PLATFORM_WINDOWS
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
// define something for simulator
#define PLATFORM_IOS
#elif TARGET_OS_IPHONE
// define something for iphone
#define PLATFORM_IOS
#else
#define TARGET_OS_OSX 1
// define something for OSX
#define PLATFORM_OSX
#endif
#elif __ANDROID__
#define PLATFORM_ANDROID
#elif __linux
// linux
#define PLATFORM_LINUX
#elif __posix
// POSIX
#define PLATFORM_POSIX
#endif
#endif


#ifndef PLATFORM_WINDOWS
typedef unsigned long DWORD;	//-V677
typedef unsigned short WORD;	//-V677
typedef unsigned int UNINT32;
typedef void* HANDLE;	 //-V677
// typedef unsigned __int64 ULONG64, *PULONG64; //-V677
// typedef unsigned __int64 DWORD64, *PDWORD64; //-V677
#endif


// namespace psl
//{
//#if defined(_WIN64)
//	using size_t = long long unsigned int;
//	using ptrdiff_t = long long int;
//#elif defined(_WIN32)
//	using size_t = unsigned int;
//	using ptrdiff_t = int;
//#elif defined (__linux__) && defined(__SIZE_TYPE__) && defined(__PTRDIFF_TYPE__)
//	using size_t = __SIZE_TYPE__;
//	using ptrdiff_t = __PTRDIFF_TYPE__;
//#else
//#	error unknown platform
//#endif
//}

// using ptrdiff_t = psl::ptrdiff_t;


/// \brief platform specific utilities that help in identifying the current platform, and various specifications of it.
namespace utility::platform
{
	/// \brief contains all known, supported (or to be supported) platforms.
	/// \note we need to add an extra dash to linux as the define already exists on linux platforms
	enum class platform_t
	{
		UNKNOWN = 0,
		lnx		= 1,
		osx		= 2,
		windows = 3,
		android = 4,
		ios		= 5
	};

	/// \brief encoding enum, that can be used to help identify encoding of items.
	enum class encoding_t
	{
		UNKNOWN = 0,
		UTF8	= 1,
		UTF16	= 2
	};
#if defined(PLATFORM_WINDOWS)
	/// \brief contains what platform this current is.
	constexpr platform_t platform = platform_t::windows;
	/// \brief contains the UTF-ness of the current platform.
	constexpr encoding_t encoding = encoding_t::UTF16;
#elif defined(PLATFORM_IOS)
	constexpr platform_t platform = platform_t::ios;
	constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PLATFORM_OSX)
	constexpr platform_t platform = platform_t::osx;
	constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PLATFORM_ANDROID)
	constexpr platform_t platform = platform_t::android;
	constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PLATFORM_LINUX)
	constexpr platform_t platform = platform_t::lnx;
	constexpr encoding_t encoding = encoding_t::UTF8;
#elif defined(PLATFORM_POSIX)
	constexpr platform_t platform = platform_t::posix;
	constexpr encoding_t encoding = encoding_t::UTF8;
#else
#error not supported
#endif

	/// \brief contains all known, supported (or to be supported) architectures.
	enum class architecture_t
	{
		UNKNOWN = 0,
		x64,
		ARM
	};

#if defined(PLATFORM_X64)
	/// \brief the architecture of the current platform.
	constexpr architecture_t architecture = architecture_t::x64;
#elif defined(PLATFORM_ANDROID)
	constexpr architecture_t architecture = architecture_t::ARM;
#else
	constexpr architecture_t architecture = architecture_t::UNKNOWN;
#endif
}	 // namespace utility::platform
