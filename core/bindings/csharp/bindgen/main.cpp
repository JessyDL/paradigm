#include "coreclr_delegates.h"
#include "hostfxr.h"
#include "nethost.h"
#include "utf8.h"
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
	#include <Windows.h>
#else
	#include <dlfcn.h>
#endif

void* host_context = nullptr;	 // context handle to the loaded runtime (your C# library)
void* host_library = nullptr;	 // HMODULE on windows, void* on linux. This is the handle to the hostfxr library

hostfxr_initialize_for_runtime_config_fn initialize_runtime_config = nullptr;
hostfxr_get_runtime_delegate_fn get_runtime_delegate			   = nullptr;
hostfxr_close_fn close_hostfxr									   = nullptr;
load_assembly_fn load_assembly									   = nullptr;
get_function_pointer_fn get_function_pointer					   = nullptr;

void load(std::filesystem::path const& root, std::string const& dll) {
	auto hostfxrPath = []() {
		std::vector<char_t> buffer(1024);
		size_t bufferSize = buffer.size() * sizeof(char_t);

		int rc = get_hostfxr_path(buffer.data(), &bufferSize, nullptr);
		if(rc != 0) {
			bufferSize = 0;
		}

		// shrink it back to the actual content size
		buffer.resize(bufferSize);
#if defined(UNICODE)
		std::string result {};
		utf8::utf16to8(buffer.begin(), buffer.end(), back_inserter(result));
		result.resize(result.size() - 1);
		return result;
#else
		return std::string(buffer.data(), buffer.size());
#endif
	}();

	std::cout << "Hostfxr path: " << hostfxrPath << std::endl;


#ifdef _WIN32
	host_library = LoadLibraryA(hostfxrPath.c_str());
#else
	host_library = dlopen(hostfxrPath.c_str(), RTLD_NOW);
#endif
	if(!host_library) {
		throw std::runtime_error("Failed to load the host library.");
	}

	std::cout << "Host library loaded." << std::endl;

	auto load_lib_fn = [](const char* name, auto& target) {
		target = reinterpret_cast<std::remove_cvref_t<decltype(target)>>(GetProcAddress(
#ifdef _WIN32
		  static_cast<HMODULE>(host_library)
#else
		  library
#endif
			,
		  name));
		return target != nullptr;
	};

	std::cout << "Loading the hostfxr functions.." << std::endl;

	if(!load_lib_fn("hostfxr_initialize_for_runtime_config", initialize_runtime_config) ||
	   !load_lib_fn("hostfxr_get_runtime_delegate", get_runtime_delegate) ||
	   !load_lib_fn("hostfxr_close", close_hostfxr)) {
		std::cerr << "Failed to load the hostfxr functions." << std::endl;
		throw std::runtime_error("Failed to load the hostfxr functions.");
	}

	auto runtimeconfig_path = [&root, &dll]() {
		std::string buffer = (root / (dll + ".runtimeconfig.json")).string();
#if defined(UNICODE)
		std::wstring result {};
		utf8::utf8to16(buffer.begin(), buffer.end(), back_inserter(result));
		result.resize(result.size() - 1);
		return result;
#else
		return buffer;
#endif
	}();
	{
		std::cout << "Initializing the runtime.. at " << std::endl;
		int rc = initialize_runtime_config(runtimeconfig_path.c_str(), nullptr, &host_context);
		if(rc != 0) {
			throw std::runtime_error("Failed to initialize the runtime.");
		}
	}

	std::cout << "Runtime initialized." << std::endl;

	auto init_runtime_delegate_fn = [get_runtime_delegate = get_runtime_delegate,
									 host_context = host_context](hostfxr_delegate_type delegate_type, auto& target) {
		void* delegate = nullptr;
		int rc		   = get_runtime_delegate(host_context, delegate_type, &delegate);
		if(rc != 0 || delegate == nullptr) {
			std::cerr << "Failed to get runtime delegate: " << rc << std::endl;
			return false;
		}
		target = reinterpret_cast<std::remove_cvref_t<decltype(target)>>(delegate);
		return true;
	};

	if(!init_runtime_delegate_fn(hdt_load_assembly, load_assembly) ||
	   !init_runtime_delegate_fn(hdt_get_function_pointer, get_function_pointer)) {
		throw std::runtime_error("Failed to load the hostfxr functions.");
	}

	std::cout << "Hostfxr functions loaded." << std::endl;

	// load assembly
	{
		auto assembly_path = [&root, &dll]() {
			std::string buffer = (root / (
#if _WIN32
										   dll + ".dll"
#elif __APPLE__
										   "lib" + dll + ".dylib"
#else
										   "lib" + dll + ".so"
#endif
										   ))
								   .string();

#if defined(UNICODE)
			std::wstring result {};
			utf8::utf8to16(buffer.begin(), buffer.end(), back_inserter(result));
			return result;

#else
			return buffer;

#endif
		}();
		auto rc = load_assembly(assembly_path.c_str(), nullptr, nullptr);
		if(rc != 0) {
			throw std::runtime_error("Failed to load the .NET assembly.");
		}
	}
}


int main(int argc, char** argv) {
	std::cout << "Running bindgen.." << std::endl;
	for(auto i = 0; i < argc; ++i) {
		std::cout << argv[i];
		if(i + 1 < argc) {
			std::cout << " ";
		} else {
			std::cout << std::endl;
		}
	}
	auto exe  = std::filesystem::current_path();
	auto dll  = "csharp_bindings";

	std::cout << "Current path: " << exe << std::endl;

	load(exe.parent_path(), dll);

	return 0;
}
