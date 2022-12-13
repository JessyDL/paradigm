#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/limits.hpp"
#include "psl/ustring.hpp"
#include <cstdint>
namespace core::os
{
class surface;
}

namespace core::igles
{
namespace details
{
	struct xcb_context_data
	{
#ifdef SURFACE_XCB
		void* egl_display {nullptr};
		void* egl_surface {nullptr};
		void* egl_context {nullptr};
		void* egl_config {nullptr};
#endif
	};
	struct win32_context_data
	{
#ifdef SURFACE_WIN32
#endif
	};
}	 // namespace details
class context
{
  public:
	context(core::resource::cache_t& cache,
			const core::resource::metadata& metaData,
			psl::meta::file* metaFile,
			psl::string8::view name);
	~context();
	context(const context&)			   = delete;
	context(context&&)				   = delete;
	context& operator=(const context&) = delete;
	context& operator=(context&&)	   = delete;

	void enable(const core::os::surface& surface);
	bool swapbuffers(core::resource::handle<core::os::surface> surface);

	const core::gfx::limits& limits() const noexcept { return m_Limits; }

  private:
	void quey_capabilities() noexcept;

	core::gfx::limits m_Limits;

#ifdef SURFACE_WIN32
	details::win32_context_data data {};
#endif

#ifdef SURFACE_XCB
	details::xcb_context_data data {};
#endif
};
}	 // namespace core::igles