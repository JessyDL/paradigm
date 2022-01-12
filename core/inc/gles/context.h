#pragma once
#include "fwd/resource/resource.h"
#include "gfx/limits.h"
#include "psl/ustring.h"
#include <cstdint>
namespace core::os
{
	class surface;
}

namespace core::igles
{
	class context
	{
	  public:
		context(core::resource::cache_t& cache,
				const core::resource::metadata& metaData,
				psl::meta::file* metaFile,
				psl::string8::view name);
		~context();
		context(const context&) = delete;
		context(context&&)		= delete;
		context& operator=(const context&) = delete;
		context& operator=(context&&) = delete;

		void enable(const core::os::surface& surface);
		bool swapbuffers(core::resource::handle<core::os::surface> surface);

		const core::gfx::limits& limits() const noexcept { return m_Limits; }

	  private:
		core::gfx::limits m_Limits;
	};
}	 // namespace core::igles