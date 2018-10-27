#include "stdafx_psl.h"
#include "platform_utils.h"

#ifdef PLATFORM_WINDOWS
#pragma comment(lib, "Dbghelp.lib")
#include <Dbghelp.h>
#endif

#include "string_utils.h"

// todo: validate windows directory seperator etc..

std::vector<unsigned int> utility::platform::os::to_virtual_keycode(const psl::string8_t& key)
{
	std::vector<unsigned int> values;
#ifdef PLATFORM_WINDOWS
	if(key.size() == 1)
	{
		short result = VkKeyScanEx(*(key.c_str()), GetKeyboardLayout(0));
		values.push_back(LOBYTE(result));

		if(HIBYTE(result) & 0x1)
		{
			values.push_back(VK_SHIFT);
		}
		if(HIBYTE(result) & 0x2)
		{
			values.push_back(VK_CONTROL);
		}
		if(HIBYTE(result) & 0x4)
		{
			values.push_back(VK_MENU);
		}
	}
	psl::string8_t upper = utility::string::to_upper(key);

	if(upper == "SHIFT")
	{
		values.push_back(VK_SHIFT);
	}
	if(upper == "LSHIFT")
	{
		values.push_back(VK_LSHIFT);
	}
	if(upper == "RSHIFT")
	{
		values.push_back(VK_RSHIFT);
	}

	if(upper == "CONTROL")
	{
		values.push_back(VK_CONTROL);
	}
	if(upper == "LCONTROL")
	{
		values.push_back(VK_LCONTROL);
	}
	if(upper == "RCONTROL")
	{
		values.push_back(VK_RCONTROL);
	}

	if(upper == "SPACE")
	{
		values.push_back(VK_SPACE);
	}
	if(upper == "RETURN")
	{
		values.push_back(VK_RETURN);
	}

	if(upper == "LEFT")
	{
		values.push_back(VK_LEFT);
	}
	if(upper == "RIGHT")
	{
		values.push_back(VK_RIGHT);
	}
	if(upper == "UP")
	{
		values.push_back(VK_UP);
	}
	if(upper == "DOWN")
	{
		values.push_back(VK_DOWN);
	}

	if(upper == "ALT")
	{
		values.push_back(VK_MENU);
	}
	if(upper == "LALT")
	{
		values.push_back(VK_LMENU);
	}
	if(upper == "RALT")
	{
		values.push_back(VK_RMENU);
	}
	if(upper == "ESCAPE")
	{
		values.push_back(VK_ESCAPE);
	}
	if(upper == "MOUSE_LCLICK")
	{
		values.push_back(VK_LBUTTON);
	}
	if(upper == "MOUSE_RCLICK")
	{
		values.push_back(VK_RBUTTON);
	}
	if(upper == "MOUSE_MCLICK")
	{
		values.push_back(VK_MBUTTON);
	}
	if(upper == "MOUSE_BUTTON1")
	{
		values.push_back(VK_XBUTTON1);
	}
	if(upper == "MOUSE_BUTTON2")
	{
		values.push_back(VK_XBUTTON2);
	}

#else

#endif
	return values;
}


bool utility::platform::directory::exists(psl::string_view absolutePath)
{
	auto platform_path = to_platform(absolutePath);
#ifdef PLATFORM_WINDOWS
	std::wstring str{psl::to_platform_string(platform_path)};
	auto ftyp = GetFileAttributes(str.c_str());
	if(ftyp == INVALID_FILE_ATTRIBUTES) return false; // something is wrong with your path!

	if(ftyp & FILE_ATTRIBUTE_DIRECTORY) return true; // this is a directory!

	return false; // this is not a directory!
#elif defined(PLATFORM_ANDROID)
	return false;
#else
	return std::filesystem::exists(platform_path);
#endif
}


bool utility::platform::directory::create(psl::string_view absolutePath, bool recursive)
{
	psl::string path = to_platform(absolutePath);
#ifdef PLATFORM_WINDOWS
	std::wstring str{psl::to_platform_string(absolutePath)};
	bool succes = (CreateDirectory(str.c_str(), NULL) != 0);
	if(succes) return true;

	if(!recursive) return false;

	size_t position = path.substr(0, path.size() - directory::seperator.size()).find_last_of(directory::seperator);
	if(position < path.size())
	{
		psl::string parentFolder = path.substr(0, position);
		if(create(parentFolder, true)) // recursive attempt at parent directory creation
			return (CreateDirectory(str.c_str(), NULL) != 0);
	}
	return false;
#elif defined(PLATFORM_ANDROID)
	return false;
#else
	size_t position = path.substr(0, path.size() - directory::seperator.size()).find_last_of(directory::seperator);
	if(std::filesystem::exists(path.substr(0, position)) && std::filesystem::create_directory(path))
		return true;
	else if(!recursive)
		return false;

	position = path.substr(0, path.size() - directory::seperator.size()).find_last_of(directory::seperator);
	if(position < path.size())
	{
		psl::string parentFolder = path.substr(0, position);
		if(create(parentFolder, true)) // recursive attempt at parent directory creation
			return std::filesystem::create_directory(path);
	}
	return false;
#endif
}

std::vector<utility::debug::trace_info> utility::debug::trace(size_t offset, size_t depth)
{
	std::vector<utility::debug::trace_info> res;
#ifdef PLATFORM_WINDOWS
	unsigned int i;
	std::vector<void*> stack;
	stack.resize(depth);
	unsigned short frames;
	IMAGEHLP_SYMBOL64* symbol;
	HANDLE process;

	process = GetCurrentProcess();

	SymInitialize(process, NULL, TRUE);

	frames = CaptureStackBackTrace((DWORD)(offset + 1u), (DWORD)depth, stack.data(), NULL);

	res.reserve(frames);

	symbol				  = (IMAGEHLP_SYMBOL64*)calloc(sizeof(IMAGEHLP_SYMBOL64) + 256 * sizeof(char), 1);
	symbol->MaxNameLength = 255;
	symbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);

	for(i = 0; i < frames; i++)
	{
		SymGetSymFromAddr64(process, (DWORD64)(stack[i]), 0, symbol);
		utility::debug::trace_info info;
		info.name = psl::from_string8_t(psl::string8_t(symbol->Name));
		info.addr = symbol->Address;
		res.push_back(info);
	}

	free(symbol);
#endif
	return res;
}
