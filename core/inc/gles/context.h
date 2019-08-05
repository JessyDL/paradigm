#pragma once
#include <cstdint>
#include "ustring.h"

namespace core::resource
{
	class cache;

	template <typename T>
	class handle;
} // namespace core::resource

namespace core::os
{
	class surface;
}

namespace psl
{
	struct UID;
} // namespace psl

namespace core::igles
{
	class context
	{
	  public:
		context(const psl::UID &uid, core::resource::cache &cache, psl::string8::view name);
		~context();
		context(const context &) = delete;
		context(context &&)		 = delete;
		context &operator=(const context &) = delete;
		context &operator=(context &&) = delete;

		void enable(core::resource::handle<core::os::surface> surface);
		bool swapbuffers(core::resource::handle<core::os::surface> surface);

	  private:
	};
} // namespace core::igles