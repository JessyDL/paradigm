#pragma once
#include <psl/ustring.hpp>
#include "coreclr_delegates.h"
#include "hostfxr.h"

namespace core::bindings::csharp {
class runtime {
  public:
	runtime(psl::string root, psl::string dll);
	~runtime();

	bool is_running();
  private:
	void* m_HostContext = nullptr;	  // context handle to the loaded runtime (your C# library)
	void* m_HostLibrary = nullptr;	  // HMODULE on windows, void* on linux. This is the handle to the hostfxr library

	hostfxr_initialize_for_runtime_config_fn initialize_runtime_config				 = nullptr;
	hostfxr_get_runtime_delegate_fn get_runtime_delegate							 = nullptr;
	hostfxr_close_fn close_hostfxr													 = nullptr;
	load_assembly_fn load_assembly													 = nullptr;
	get_function_pointer_fn get_function_pointer									 = nullptr;
};
}	 // namespace core::bindings::csharp
