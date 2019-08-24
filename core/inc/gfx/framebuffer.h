#pragma once
#include "fwd/gfx/framebuffer.h"
#include "resource/resource.hpp"

namespace core::data
{
	class framebuffer;
}

namespace core::gfx
{
	class context;

	class framebuffer
	{
	  public:
		  using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::framebuffer
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::framebuffer
#endif
			>;
		using value_type = alias_type;
		framebuffer(core::resource::handle<value_type>& handle);
		framebuffer(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
					core::resource::handle<core::gfx::context> context, core::resource::handle<data::framebuffer> data);
		~framebuffer() = default;

		framebuffer(const framebuffer& other)	 = delete;
		framebuffer(framebuffer&& other) noexcept = delete;
		framebuffer& operator=(const framebuffer& other) = delete;
		framebuffer& operator=(framebuffer&& other) noexcept = delete;

		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx