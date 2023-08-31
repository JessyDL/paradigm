#pragma once

#include "psl/platform_def.hpp"
#include "psl/ustring.hpp"
#include <cctype>
#include <clocale>
#include <numeric>
#include <optional>
#include <vector>


// https://en.wikipedia.org/wiki/Path_%28computing%29#Representations_of_paths_by_operating_system_and_shell

#if defined(PLATFORM_ANDROID)
struct AAssetManager;
#endif

namespace psl::utility::platform {
/// \brief helper class that contains directory specific I/O manipulation methods and platform utilities.
class directory {
  public:
	/// \brief the seperator character that all commands will be assumed to use, and all platform specific commands
	/// will be translated to/from
	static constexpr psl::string_view seperator = "/";
#ifdef PLATFORM_WINDOWS
	/// \brief the platform specific seperator for the current platform (i.e. the one the platform by default uses)
	static constexpr psl::string_view seperator_platform = "\\";
#else
	static constexpr psl::string_view seperator_platform = "/";
#endif
	/// \brief identifier that signifies the "current location" symbol in a file path.
	static constexpr psl::string_view current_symbol = ".";

	/// \brief identifier that signifies the "go up one location" symbol in a file path.
	static constexpr psl::string_view parent_symbol = "..";

	/// \brief translated the given path to one that is accepted on Unix systems
	/// \param[in] path the path to translate to one that works in unix systems.
	/// \returns a string that *should* work on unix systems, and satisfies the requirements.
	static psl::string to_unix(psl::string_view path);

	/// \brief translated the given path to one that is accepted on Windows systems
	/// \param[in] path the path to translate to one that works in Windows systems.
	/// \returns a string that *should* work on Windows systems, and satisfies the requirements.
	static psl::string to_windows(psl::string_view path) {
		psl::string dir;
		while(path.size() > 0 && (path[0] == '\"' || path[0] == '\'')) {
			path = path.substr(1);
		}

		while(path.size() > 0 && (path[path.size() - 1] == '\"' || path[path.size() - 1] == '\'')) {
			path = path.substr(0, path.size() - 1);
		}

		if(path[0] == '/') {
			dir = psl::string(path.substr(5u, 1u));
			dir += ":/";
			dir += path.substr(7u);
		} else {
			dir = path;
		}

		auto find = dir.find('\\');
		while(find < dir.size()) {
			dir.replace(find, 1, "/");
			find = dir.find('\\');
		}

		find = dir.find("//");
		while(find < dir.size()) {
			dir.erase(find, 1);
			find = dir.find("//");
		}
		return dir;
	}

	static psl::string to_android(psl::string_view path) {
		psl::string result {};
		auto constituents = [](psl::string_view path) -> std::vector<psl::string_view> {
			std::vector<psl::string_view> res {};
			size_t offset = 0;
			size_t index  = path.find('/', offset);
			while(index != psl::string_view::npos) {
				res.emplace_back(path.substr(offset, index - offset));
				offset = index + 1;
				index  = path.find('/', offset);
			}
			res.emplace_back(path.substr(offset, path.size() - offset));
			return res;
		}(path);

		size_t offset = 0;
		auto it		  = std::begin(constituents);
		auto next	  = std::next(it);
		while(next != std::end(constituents)) {
			if(*next == psl::string_view {".."}) {
				constituents.erase(it, std::next(next));
				if(offset + 2 >= constituents.size())
					break;
				it	 = std::next(std::begin(constituents), offset);
				next = std::next(next);
			} else {
				offset += 1;
				it	 = std::next(it);
				next = std::next(next);
			}
		}
		if(constituents.empty())
			return result;


		const auto size = std::accumulate(std::begin(constituents),
										  std::end(constituents),
										  size_t {0},
										  [](size_t sum, psl::string_view value) { return sum + value.size(); }) +
						  constituents.size() - 1;
		result.reserve(size);

		for(auto it = std::begin(constituents), end = std::prev(std::end(constituents)); it != end;
			it = std::next(it)) {
			result.append(*it);
			result.append(seperator);
		}
		result.append(*std::prev(std::end(constituents)));
		return result;
	}

	/// \brief translated the given path to one that is accepted on the current platform
	/// \param[in] path the path to translate to one that works on the current platform.
	/// \returns a string that *should* work on the current platform, and satisfies the requirements.
	static psl::string to_platform(psl::string_view path) {
#ifdef PLATFORM_WINDOWS
		return to_windows(path);
#elif defined(PLATFORM_ANDROID)
		return to_android(path);
#else
		return to_unix(path);
#endif
	}

	/// \brief translates the given path to the default path encoding that is used across this application as the
	/// standard. \param[in] path the path to translate. \returns a string that is translated to the
	/// application-wide standard encoding (unix).
	static psl::string to_generic(psl::string_view path) {
		return to_unix(path);
	}

	/// \brief checks if the given path is a directory.
	/// \param[in] path the path to check.
	/// \returns true in case it is a directory.
	/// \todo android platform always returns false. Check if there is a way around this or redesign this.
	static bool is_directory(psl::string_view path);

	/// \brief sanitizes the seperators into the application wide standard one.
	/// \param[in] path the path to sanitize.
	/// \returns the transformed input path.
	static psl::string sanitize(psl::string_view path);

	/// \brief tries to erase/delete the given item this path points to.
	///
	/// tries to delete the file this path points  to, or the folder this path points to.
	/// \param[in] path the path to what you wish to be erased.
	/// \returns true if erasing the target item is successful.
	/// \todo android platform always fails this, is it possible this is not always the case?
	static bool erase(psl::string_view path);

	/// \brief checks if the given path (absolute) points to a directory or not.
	/// \param[in] absolutePath the path to check.
	/// \returns true in case this path is valid and points to a directory.
	static bool exists(psl::string_view absolutePath);

	/// \brief creates a directory that satisfies this path.
	/// \param[in] absolutePath the path to create the directory at.
	/// \param[in] recursive should we make all non-existing parent directories as well (true) or fail (false) when
	/// the path to the given directory does not exist? \returns true if the path is now fully valid and the
	/// directory exists.
	static bool create(psl::string_view absolutePath, bool recursive = false);

	/// \brief gets all files in the target directory, no folders.
	///
	/// gets all files in the target directory exluding the sub-directories. It however can recursively walk the
	/// sub-directories when needed, it will only return their contained items though.
	/// \param[in] target_directory the directory to run a search from.
	/// \param[in] recursive should we also search the sub-directories (true), or not (false).
	/// \returns a container of elements of the type psl::string, of all found items, that are transformed to the
	/// generic path format. \todo on android no search is run.
	static std::vector<psl::string> all_files(psl::string_view target_directory, bool recursive);
};

/// \brief file i/o and manpulations utilities namespace
namespace file {
#if defined(PLATFORM_ANDROID)
	extern AAssetManager* ANDROID_ASSET_MANAGER;
#endif
	/// \brief checks if the given path points to a file or not.
	///
	/// checks if the given path points to a file or not. This path can be relative to the current working
	/// directory, usually the app root is the working directory, or an absolute path. \param[in] filename the path
	/// to the file to check. \returns true in case the target is indeed a file, otherwise in all scenarios it
	/// returns false. \todo android lacks an implementation.
	bool exists(psl::string_view filename);

	/// \brief erases the file ath the given path.
	/// \param[in] filename the path to the file.
	/// \returns true in case the file was successfuly erased.
	/// \todo android lacks an implementation.
	bool erase(psl::string_view filename);

	/// \brief reads the given filepath's contents into a psl::char_t container.
	/// \param[in] filename the path to the file.
	/// \param[in, out] out the output container to read into.
	/// \returns true on successfuly reading the file. This can be false in case something went wrong.
	/// \note this function might not work in all scenarios on non-desktop platforms that have sandboxed away the
	/// interaction with the filesystem. Please verify that your target platform actually supports this function for
	/// the file you wish to load.
	bool
	read(psl::string_view filename, std::vector<psl::char_t>& out, size_t count = std::numeric_limits<size_t>::max());

	/// \brief reads the given filepath's contents into a psl::string container.
	/// \param[in] filename the path to the file.
	/// \param[in, out] out the output container to read into.
	/// \returns true on successfuly reading the file. This can be false in case something went wrong.
	/// \note this function might not work in all scenarios on non-desktop platforms that have sandboxed away the
	/// interaction with the filesystem. Please verify that your target platform actually supports this function for
	/// the file you wish to load.
	bool read(psl::string_view filename, psl::string& out, size_t count = std::numeric_limits<size_t>::max());

	/// \brief reads the given filepath's contents into a psl::string container on success.
	/// \param[in] filename the path to the file.
	/// \returns a psl::string on success, and a std::nullopt on failure.
	/// \note this function might not work in all scenarios on non-desktop platforms that have sandboxed away the
	/// interaction with the filesystem. Please verify that your target platform actually supports this function for
	/// the file you wish to load.
	static std::optional<psl::string> read(psl::string_view filename,
										   size_t count = std::numeric_limits<size_t>::max()) {
		psl::string res;
		if(!read(filename, res, count))
			return std::nullopt;
		return res;
	}

	/// \brief write the contents to the given location on the filesystem.
	/// \param[in] filename the path to the target location.
	/// \param[in] content the content to write at the given location.
	/// \returns true when the content has successfully been written at the target location.
	bool write(psl::string_view filename, psl::string_view content);

	/// \brief transforms the given path to the unix format.
	/// \param[in] path the path to transform.
	/// \returns the transformed path.
	static psl::string to_unix(psl::string_view path) {
		return directory::to_unix(path);
	}

	/// \brief transforms the given path to the windows format.
	/// \param[in] path the path to transform.
	/// \returns the transformed path.
	static psl::string to_windows(psl::string_view path) {
		return directory::to_windows(path);
	}

	static psl::string to_android(psl::string_view path) {
		return directory::to_android(path);
	}

	/// \brief transforms the given path to the current platforms format.
	/// \param[in] path the path to transform.
	/// \returns the transformed path.
	static psl::string to_platform(psl::string_view path) {
#ifdef PLATFORM_WINDOWS
		return to_windows(path);
#elif defined(PLATFORM_ANDROID)
		return to_android(path);
#else
		return to_unix(path);
#endif
	}

	/// \brief transforms the given path to the application wide standard format.
	/// \param[in] path the path to transform.
	/// \returns the transformed path.
	static psl::string to_generic(psl::string_view path) {
		return to_unix(path);
	}
};	  // namespace file

/// \brief platform specific utility that can freeze the console running on the current thread.
static void cmd_prompt_presskeytocontinue() {
	if constexpr(platform == platform_t::windows) {
		system("pause");
	} else if constexpr(platform == platform_t::macos) {
		system("read");
	} else {
		static_assert("CMDPromptPressAnyKeyToContinue has not been implemented for the given platform.");
	}
}

/// \brief OS specific utility functions
class os {
  public:
	static std::vector<unsigned int> to_virtual_keycode(const psl::string8_t& key);
};
}	 // namespace psl::utility::platform
