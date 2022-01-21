#include "psl/application_utils.hpp"

#if defined(PLATFORM_WINDOWS)
#include "Shlwapi.h"
#include <Windows.h>
#endif

#if defined(PLATFORM_LINUX)
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#endif

using namespace utility::application::path;

psl::string utility::application::path::get_path()
{
#ifdef WIN32
	wchar_t dest[MAX_PATH];
	GetModuleFileName(NULL, dest, MAX_PATH);
	PathRemoveFileSpec(dest);
	return psl::to_string8_t(psl::platform::view {dest}) + "\\";
#elif defined PLATFORM_LINUX
	char result[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
	// const char *path;
	if(count != -1)
	{
		return psl::string {dirname(result)} + "/";
	}
	return "";
#elif defined(PLATFORM_ANDROID)	   // we run in a sandbox where we are root
	return "";
#else
#error not supported
#endif
}