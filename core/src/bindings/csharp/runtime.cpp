#include "core/bindings/csharp/runtime.hpp"

#include "nethost.h"
#include <vector>
#ifdef _WIN32
	#include <Windows.h>
#else
	#include <dlfcn.h>
#endif

namespace core::bindings::csharp {
runtime::runtime(psl::string root, psl::string dll) {
	auto hostfxrPath = []() {
		std::vector<psl::pchar_t> buffer(1024);
		size_t bufferSize = buffer.size() * sizeof(psl::pchar_t);

		int rc = get_hostfxr_path(buffer.data(), &bufferSize, nullptr);
		if(rc != 0) {
			bufferSize = 0;
		}

		// shrink it back to the actual content size
		buffer.resize(bufferSize / sizeof(psl::pchar_t));

		return psl::to_string8_t(psl::pstring_t(buffer.data(), buffer.size() * sizeof(psl::pchar_t)));
	}();

#ifdef _WIN32
	m_HostLibrary = LoadLibraryA(hostfxrPath.c_str());
#else
	m_HostLibrary = dlopen(hostfxrPath.c_str(), RTLD_NOW);
#endif
	if(!m_HostLibrary) {
		throw std::runtime_error("Failed to load the host library.");
	}

	auto load_lib_fn = [library = m_HostLibrary](const char* name, auto& target) {
		target = reinterpret_cast<std::remove_cvref_t<decltype(target)>>(GetProcAddress(
#ifdef _WIN32
		  static_cast<HMODULE>(library)
#else
		  library
#endif
			,
		  name));
		return target != nullptr;
	};

	if(!load_lib_fn("hostfxr_initialize_for_runtime_config", initialize_runtime_config) ||
	   !load_lib_fn("hostfxr_get_runtime_delegate", get_runtime_delegate) ||
	   !load_lib_fn("hostfxr_close", close_hostfxr)) {
		throw std::runtime_error("Failed to load the hostfxr functions.");
	}

	auto runtimeconfig_path = psl::to_pstring(root + dll + ".runtimeconfig.json");
	{
		int rc = initialize_runtime_config(runtimeconfig_path.c_str(), nullptr, &m_HostContext);
		if(rc != 0) {
			throw std::runtime_error("Failed to initialize the runtime.");
		}
	}

	auto init_runtime_delegate_fn = [get_runtime_delegate = get_runtime_delegate,
									 host_context = m_HostContext](hostfxr_delegate_type delegate_type, auto& target) {
		void* delegate = nullptr;
		int rc		   = get_runtime_delegate(host_context, delegate_type, &delegate);
		if(rc != 0 || delegate == nullptr) {
			core::log->error("Failed to get runtime delegate: {}", rc);
			return false;
		}
		target = reinterpret_cast<std::remove_cvref_t<decltype(target)>>(delegate);
		return true;
	};

	if(!init_runtime_delegate_fn(hdt_load_assembly, load_assembly) ||
	   !init_runtime_delegate_fn(hdt_get_function_pointer, get_function_pointer)) {
		throw std::runtime_error("Failed to load the hostfxr functions.");
	}

	// load assembly
	{
		auto assembly_path = psl::to_pstring(root +
#if _WIN32
											 dll + ".dll"
#elif __APPLE__
											 "lib" + dll + ".dylib"
#else
											 "lib" + dll + ".so"
#endif
		);
		auto rc = load_assembly(assembly_path.c_str(), nullptr, nullptr);
		if(rc != 0) {
			throw std::runtime_error("Failed to load the .NET assembly.");
		}
	}
	{
		using delegate_void_void	= void(CORECLR_DELEGATE_CALLTYPE*)();
		delegate_void_void entry_fn = nullptr;
		auto rc						= get_function_pointer(L"Paradigm.Example, csharp_bindings",
									   L"Initialize",
									   L"Paradigm.Example+VoidVoidDelegate, csharp_bindings",
									   nullptr,
									   nullptr,
									   reinterpret_cast<void**>(&entry_fn));
		if(rc != 0) {
			throw std::runtime_error("Failed to get the function pointer.");
		}
		entry_fn();
	}
	{
		using delegate_string_void	= int(CORECLR_DELEGATE_CALLTYPE*)(char*, int);
		delegate_string_void entry_fn = nullptr;

		auto rc = get_function_pointer(L"Paradigm.Example, csharp_bindings",
									   L"GetAllEcsSystemMethods",
									   L"Paradigm.Example+GetAllEcsSystemMethodsDelegate, csharp_bindings",
									   nullptr,
									   nullptr,
									   reinterpret_cast<void**>(&entry_fn));
		if(rc != 0) {
			throw std::runtime_error("Failed to get the function pointer.");
		}

		int size = entry_fn(nullptr, 0);
		psl::string8_t buffer {};
		buffer.resize(size);
		entry_fn(buffer.data(), size);
		core::log->info("ECS System Description: {}", buffer);
	}
}
runtime::~runtime() {
	if(m_HostContext) {
		close_hostfxr(m_HostContext);
	}
	if(m_HostLibrary) {
#ifdef _WIN32
		FreeLibrary(static_cast<HMODULE>(m_HostLibrary));
#else
		dlclose(m_HostLibrary);
#endif
	}
}

bool runtime::is_running() {
	return m_HostContext != nullptr;
}
}	 // namespace core::bindings::csharp
