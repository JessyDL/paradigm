#pragma once
#include "resource/resource.hpp"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class framebuffer;
}
#endif
#ifdef PE_VULKAN
namespace core::ivk
{
	class framebuffer;
}
#endif

namespace core::data
{
	class framebuffer;
}

namespace core::gfx
{
	class context;

	class framebuffer
	{
		using value_type = std::variant<
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
	  public:
		framebuffer(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
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