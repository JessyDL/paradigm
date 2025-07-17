#pragma once
#include "coreclr_delegates.h"
#include "hostfxr.h"
#include <psl/ustring.hpp>
#include <unordered_map>

namespace core::bindings::csharp {
namespace {
	template <typename T>
	struct delegate_info_t {};

	template <>
	struct delegate_info_t<int> {
		using type = int;
		static constexpr psl::string_view name {"int"};
	};

	template <>
	struct delegate_info_t<float> {
		using type = float;
		static constexpr psl::string_view name {"float"};
	};

	template <>
	struct delegate_info_t<void> {
		using type = void;
		static constexpr psl::string_view name {"void"};
	};

	template <>
	struct delegate_info_t<psl::string> {
		using type = char*;
		static constexpr psl::string_view name {"string"};
	};
}	 // namespace
class runtime {
  public:
	runtime(psl::string root, psl::string dll);
	~runtime();

	bool is_running();
	template <typename Return = void, typename Arg = void, typename... Args>
	bool unsafe_invoke(const psl::string& classname, const psl::string& method, auto&& callback) {
		using delegate_type = typename delegate_info_t<Return>::type(CORECLR_DELEGATE_CALLTYPE*)(
		  typename delegate_info_t<Arg>::type, typename delegate_info_t<Args>::type...);
		psl::string delegate_str = construct_delegate_name<Return, Arg, Args...>();

		delegate_type fn = nullptr;
		auto cache_key	 = classname + method + delegate_str;
		if(m_FunctionCache.find(cache_key) == std::end(m_FunctionCache)) {
			int rc = unsafe_get(classname, method, delegate_str, reinterpret_cast<void**>(&fn));
			if(rc != 0) {
				throw std::runtime_error("Failed to get the function pointer.");
			}
			m_FunctionCache[cache_key] = reinterpret_cast<void*>(fn);
		} else {
			fn = reinterpret_cast<delegate_type>(m_FunctionCache[cache_key]);
		}
		if(fn) {
			callback(fn);
		}
		return fn != nullptr;
	}

  private:
	template <typename... Args>
	psl::string construct_delegate_name() {
		auto res = (construct_delegate_name_2<Args>() + ...);
		res.resize(res.size() - 1);
		return res;
	}
	template <typename Arg>
	psl::string construct_delegate_name_2() {
		return {psl::string(delegate_info_t<Arg>::name) + "_"};
	}

	int
	unsafe_get(const psl::string& classname, const psl::string& method, const psl::string& delegate_type, void** fn);

	void* m_HostContext = nullptr;	  // context handle to the loaded runtime (your C# library)
	void* m_HostLibrary = nullptr;	  // HMODULE on windows, void* on linux. This is the handle to the hostfxr library

	hostfxr_initialize_for_runtime_config_fn initialize_runtime_config = nullptr;
	hostfxr_get_runtime_delegate_fn get_runtime_delegate			   = nullptr;
	hostfxr_close_fn close_hostfxr									   = nullptr;
	load_assembly_fn load_assembly									   = nullptr;
	get_function_pointer_fn get_function_pointer					   = nullptr;

	std::unordered_map<psl::string, void*> m_FunctionCache;
};
}	 // namespace core::bindings::csharp
