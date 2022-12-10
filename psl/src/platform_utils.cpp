#include "psl/platform_utils.hpp"

#include <filesystem>
#include "psl/ustream.hpp"
#include "psl/assertions.hpp"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

#if defined(PLATFORM_ANDROID)
#include <android/asset_manager.h>
#endif

#include "psl/string_utils.hpp"

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

bool utility::platform::directory::is_directory(psl::string_view path)
{
	return std::filesystem::is_directory(to_platform(path));
}

bool utility::platform::directory::erase(psl::string_view path)
{
	return std::filesystem::remove(directory::to_platform(path));
}

psl::string utility::platform::directory::sanitize(psl::string_view path)
{ 
	return utility::string::replace_all(path, "\\", "/"); 
}

std::vector<psl::string> utility::platform::directory::all_files(psl::string_view target_directory, bool recursive)
{
	auto folder = to_platform(target_directory);
	std::vector<psl::string> names;
	if(recursive)
	{
		for(std::filesystem::recursive_directory_iterator i(folder), end; i != end; ++i)
		{
			if(!std::filesystem::is_directory(i->path()))
			{
#ifdef UNICODE
				auto filename =
					utility::string::replace_all(psl::to_string8_t(i->path().generic_wstring()), "\\", seperator);
#else
				auto filename = utility::string::replace_all(i->path().generic_string(), "\\", seperator);
#endif
				names.push_back(filename);
			}
		}
	}
	else
	{
		for(std::filesystem::directory_iterator i(folder), end; i != end; ++i)
		{
			if(!std::filesystem::is_directory(i->path()))
			{
#ifdef UNICODE
				auto filename =
					utility::string::replace_all(psl::to_string8_t(i->path().generic_wstring()), "\\", seperator);
#else
				auto filename = utility::string::replace_all(i->path().generic_string(), "\\", seperator);
#endif
				names.push_back(filename);
			}
		}
	}
	return names;
}

psl::string utility::platform::directory::to_unix(psl::string_view path)
{
	psl::string dir;
	while(path.size() > 0 && (path[0] == '\"' || path[0] == '\''))
	{
		path = path.substr(1);
	}

	while(path.size() > 0 && (path[path.size() - 1] == '\"' || path[path.size() - 1] == '\''))
	{
		path = path.substr(0, path.size() - 1);
	}

	if(auto find = path.find(":\\\\"); find == 1u)
	{
		psl::char_t drive = utility::string::to_lower(path.substr(0u, 1u))[0];
		dir				  = "/mnt/" + psl::string(1, drive) + seperator + path.substr(4u);
	}
	else if(find = path.find(":\\"); find == 1u)
	{
		psl::char_t drive = utility::string::to_lower(path.substr(0u, 1u))[0];
		dir				  = "/mnt/" + psl::string(1, drive) + seperator + path.substr(3u);
	}
	else if(find = path.find(":/"); find == 1u)
	{
		psl::char_t drive = utility::string::to_lower(path.substr(0u, 1u))[0];
		dir				  = "/mnt/" + psl::string(1, drive) + seperator + path.substr(3u);
	}
	else
	{
		dir = path;
	}

	auto find = dir.find('\\');
	while(find < dir.size())
	{
		dir.replace(find, 1, "/");
		find = dir.find('\\');
	}

	return dir;
}

bool utility::platform::directory::exists(psl::string_view absolutePath)
{
	auto platform_path = to_platform(absolutePath);
#ifdef PLATFORM_WINDOWS
	std::wstring str {psl::to_pstring(platform_path)};
	auto ftyp = GetFileAttributes(str.c_str());
	if(ftyp == INVALID_FILE_ATTRIBUTES) return false;	 // something is wrong with your path!

	if(ftyp & FILE_ATTRIBUTE_DIRECTORY) return true;	// this is a directory!

	return false;	 // this is not a directory!
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
	std::wstring str {psl::to_pstring(absolutePath)};
	bool succes = (CreateDirectory(str.c_str(), NULL) != 0);
	if(succes) return true;

	if(!recursive) return false;

	size_t position = path.substr(0, path.size() - directory::seperator.size()).find_last_of(directory::seperator);
	if(position < path.size())
	{
		psl::string parentFolder = path.substr(0, position);
		if(create(parentFolder, true))	  // recursive attempt at parent directory creation
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
		if(create(parentFolder, true))	  // recursive attempt at parent directory creation
			return std::filesystem::create_directory(path);
	}
	return false;
#endif
}

bool utility::platform::file::exists(psl::string_view filename)
{
	#if defined(PLATFORM_ANDROID)
		return AAssetManager_open(ANDROID_ASSET_MANAGER, directory::to_platform(filename).data(), AASSET_MODE_UNKNOWN) != nullptr;
	#else
		return std::filesystem::exists(directory::to_platform(filename));
	#endif
}

bool utility::platform::file::erase(psl::string_view filename)
{
	return std::filesystem::remove(directory::to_platform(filename));
}


bool utility::platform::file::read(psl::string_view filename, std::vector<psl::char_t>& out, size_t count)
{
	psl::string file_name = directory::to_platform(filename);
	psl_assert(exists(file_name), "Could not find filename {}", file_name);
	#if !defined(PLATFORM_ANDROID)
		psl::ifstream file(file_name, std::ios::binary | std::ios::ate);
		if(!file.is_open())
		{
			psl::fprintf(stderr, "Cannot open file %s!\n", file_name.c_str());
			return false;
		}
		size_t size = std::min(static_cast<size_t>(file.tellg()), count);
		file.seekg(0u, std::ios::beg);
		out.resize(size);
		file.read(&out[0], size);
		file.close();
	#else // PLATFORM_ANDROID
		AAssetDir* assetDir = AAssetManager_openDir(ANDROID_ASSET_MANAGER, "");
		AAsset *asset = AAssetManager_open(ANDROID_ASSET_MANAGER, file_name.data(), AASSET_MODE_STREAMING);
		//holds size of searched file
		off64_t length = AAsset_getLength64(asset);
		//keeps track of remaining bytes to read
		off64_t remaining = AAsset_getRemainingLength64(asset);
		size_t Mb = 1000 *1024; // 1Mb is maximum chunk size for compressed assets
		size_t currChunk;
		out.reserve(length);

		//while we have still some data to read
		while (remaining != 0) 
		{
			//set proper size for our next chunk
			if(remaining >= Mb)
			{
				currChunk = Mb;
			}
			else
			{
				currChunk = remaining;
			}
			char chunk[currChunk];

			//read data chunk
			if(AAsset_read(asset, chunk, currChunk) > 0) // returns less than 0 on error
			{
				//and append it to our vector
				out.insert(out.end(),chunk, chunk + currChunk);
				remaining = AAsset_getRemainingLength64(asset);
			}
		}
		AAsset_close(asset);
	#endif // !PLATFORM_ANDROID
	return true;
}

bool utility::platform::file::write(psl::string_view filename, psl::string_view content)
{
	auto file_name = directory::to_platform(filename);

	std::size_t found = file_name.find_last_of(directory::seperator);

	if(found != std::string::npos && !directory::exists(file_name.substr(0, found)) && !directory::create(file_name.substr(0, found), true))
		return false;

	psl::ofstream output;
	try
	{
		output.open(file_name, std::ios::trunc | std::ios::out | std::ios::binary);
	}
	catch(...)
	{
		psl::fprintf(stderr, "Could not write the file: %s!\n", file_name.c_str());
		return false;
	}
	output.write(content.data(), content.size());
	output.close();
	return true;
}

bool utility::platform::file::read(psl::string_view filename, psl::string& out, size_t count)
{
	psl::string file_name = directory::to_platform(filename);
	psl_assert(exists(file_name), "Could not find filename {}", file_name);
	#if !defined(PLATFORM_ANDROID)
		std::ifstream file(file_name.c_str(), std::ios::binary | std::ios::ate);
		if(!file.is_open())
		{
			psl::fprintf(stderr, "Cannot open file %s!\n", file_name.c_str());
			return false;
		}
		size_t size = std::min(static_cast<size_t>(file.tellg()), count);
		file.seekg(0u, std::ios::beg);
		out.resize(size);
		file.read(&out[0], size);
		file.close();
	#else // PLATFORM_ANDROID
		AAssetDir* assetDir = AAssetManager_openDir(ANDROID_ASSET_MANAGER, "");
		AAsset *asset = AAssetManager_open(ANDROID_ASSET_MANAGER, file_name.data(), AASSET_MODE_BUFFER);
		//holds size of searched file
		off64_t length = AAsset_getLength64(asset);
		//keeps track of remaining bytes to read
		off64_t remaining = AAsset_getRemainingLength64(asset);
		size_t Mb = 1000 *1024; // 1Mb is maximum chunk size for compressed assets
		size_t currChunk;
		out.reserve(length);

		//while we have still some data to read
		while (remaining != 0) 
		{
			//set proper size for our next chunk
			if(remaining >= Mb)
			{
				currChunk = Mb;
			}
			else
			{
				currChunk = remaining;
			}
			char chunk[currChunk];

			//read data chunk
			if(AAsset_read(asset, chunk, currChunk) > 0) // returns less than 0 on error
			{
				//and append it to our vector
				out.insert(out.end(),chunk, chunk + currChunk);
				remaining = AAsset_getRemainingLength64(asset);
			}
		}
		AAsset_close(asset);
	#endif // !PLATFORM_ANDROID
	return true;
}
