#pragma once
#include "psl/array.hpp"
#include "psl/platform_utils.hpp"
#include "psl/ustring.hpp"

namespace assembler {
class pathstring {
  public:
	pathstring() = default;
	pathstring(char const* path) : path(std::move(psl::utility::platform::file::to_generic(psl::string(path)))) {};
	pathstring(psl::string8_t path) : path(std::move(psl::utility::platform::file::to_generic(path))) {};
	pathstring(psl::string8::view path) : path(std::move(psl::utility::platform::file::to_generic(path))) {};

	operator psl::string8::view() const noexcept {
		return path;
	}
	operator psl::string8_t&() noexcept {
		return path;
	}

	psl::string8_t windows() const noexcept {
		return psl::utility::platform::file::to_windows(path);
	}
	psl::string8_t unix() const noexcept {
		return psl::utility::platform::file::to_unix(path);
	}

	psl::string8_t platform() const noexcept {
		return psl::utility::platform::file::to_platform(path);
	}

	psl::string8_t const* operator->() const noexcept {
		return &path;
	}
	psl::string8_t* operator->() noexcept {
		return &path;
	}

	auto operator[](size_t index) noexcept {
		return path[index];
	}
	auto operator[](size_t index) const noexcept {
		return path[index];
	}

	auto begin() noexcept {
		return std::begin(path);
	}
	auto end() noexcept {
		return std::end(path);
	}

	psl::string8_t& data() noexcept {
		return path;
	}
	psl::string8_t const& data() const noexcept {
		return path;
	}

  private:
	psl::string8_t path {};
};

inline psl::array<std::pair<pathstring, pathstring>> get_files(pathstring input, pathstring output = {}) {
	psl::array<std::pair<pathstring, pathstring>> result;
	psl::string append {};
	if(auto pos = output->find('*'); pos != output->npos) {
		append = output->substr(pos + 1);
		output->erase(pos);
	}
	if(input[input->size() - 1] == '*') {
		auto directory = input->erase(input->size() - 1);
		if(!psl::utility::platform::directory::is_directory(directory)) {
			directory.erase(directory.find_last_of('/') + 1);
		}
		auto ifiles = psl::utility::platform::directory::all_files(directory, true);
		if(output->empty()) {
			output = directory;
		}

		output->erase(std::next(std::begin(output), psl::utility::string::rfind_first_not_of(output, '/')),
					  std::end(output));

		bool prepend = false;
		if(output[0] == '.') {
			auto offset = 1;
			while(output[offset] == '/') ++offset;
			output	= output->substr(offset);
			prepend = true;
		}

		for(pathstring in : ifiles) {
			auto out = output.data() + "/" + in->substr(directory.size());
			if(prepend)
				out = directory + out;
			result.emplace_back(in, out + append);
		}
	} else {
		if(output->empty()) {
			output = input;
		} else {
			if(output[0] == '.') {
				auto directory = input;
				auto offset	   = 1;
				while(output[offset] == '/') ++offset;
				if(!psl::utility::platform::directory::is_directory(directory)) {
					directory->erase(directory->find_last_of('/') + 1);
				}
				output = directory + output->substr(offset);
			}
		}
		result.emplace_back(input, output + append);
	}
	return result;
}
}	 // namespace assembler
