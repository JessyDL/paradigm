#pragma once
#include "psl/ustring.hpp"
#include <cstdint>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace psl::utility {
/// \brief contains some debug information, such as trace information, as well as utilities to debug.
class debug {
  public:
	/// \brief contains the structure of a trace
	struct trace_info {
		/// \brief UTF-8 encoded psl::string that contains the demangled information of a trace.
		psl::string name;
		/// \brief address location in memory of the traced function.
		std::uintptr_t addr;
	};

	/// \brief method to get trace information of the current callstack.
	/// \param[in] offset what height of the callstack to start from (0 being the bottom, not root of the
	/// callstack). \param[in] depth how far up should we get information from (i.e. a depth of 1 would only get the
	/// current information, while 255 would likely get you to the root). \returns an std::vector containing a
	/// struct of trace_info of the current callstack (excluding this method).
	static std::vector<trace_info>
	trace(size_t offset = 0u, size_t depth = 255u, std::optional<std::thread::id> id = std::nullopt);


	/// \brief method to get raw trace information of the current callstack.
	/// \param[in] offset what height of the callstack to start from (0 being the bottom, not root of the
	/// callstack). \param[in] depth how far up should we get information from (i.e. a depth of 1 would only get the
	/// current information, while 255 would likely get you to the root). \returns an std::vector containing void
	/// pointers pointing to the address of the current callstack (excluding this method).
	static std::vector<void*> raw_trace(size_t offset = 0u, size_t depth = 255u);

	static psl::utility::debug::trace_info demangle(void* target);
	/// \brief helper method to give a name to a specific thread.
	/// \param[in] id thread::id that will be named.
	/// \param[in] name the name to assign to the given thread::id.
	static void register_thread(const std::thread::id& id, const psl::string8_t& name) {
		m_ThreadMap.insert(std::pair<std::thread::id, psl::string8_t>(id, name));
	}

	/// \brief helper method to name the current (this) thread.
	/// \param[in] name the name to assign to std::this_thread.
	static void register_thread(const psl::string8_t& name) { register_thread(std::this_thread::get_id(), name); }

	/// \brief gets the name of the given thread::id.
	/// \param[in] id the id of the thread to look up.
	/// \returns the name (if any) that is assigned to the given thread::id.
	static const psl::string8_t thread_name(const std::thread::id& id) { return m_ThreadMap[id]; }

	/// \brief gets the name of the current (this) thread.
	/// \returns the name (if any) that is assigned to the current thread.
	static const psl::string8_t thread_name() { return thread_name(std::this_thread::get_id()); }

	/// \brief helper method to extract just the class from a trace.
	/// \param[in] fullFuncName the signature of the trace.
	/// \returns the class name.
	static const psl::string8_t func_to_class(const char* fullFuncName) {
		psl::string8_t fullFuncNameStr(fullFuncName);
		size_t pos = fullFuncNameStr.find_last_of("::");
		if(pos == psl::string8_t::npos) {
			return fullFuncNameStr;
		}
		psl::string8_t newName = fullFuncNameStr.substr(0, pos - 1);
		pos					   = newName.find_last_of("::");
		if(pos == psl::string8_t::npos) {
			return newName;
		}
		return newName.substr(pos + 1);
	}

	static const psl::string8_t func_to_namespace(const char* fullFuncName) {
		psl::string8_t fullFuncNameStr(fullFuncName);
		size_t pos = fullFuncNameStr.find_last_of("::");
		if(pos == psl::string8_t::npos) {
			return fullFuncNameStr;
		}
		return fullFuncNameStr.substr(0, pos - 1);
	}

  private:
	static std::unordered_map<std::thread::id, psl::string8_t> m_ThreadMap;
};
}	 // namespace psl::utility
