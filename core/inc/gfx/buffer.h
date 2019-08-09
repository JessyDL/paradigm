#pragma once
#include "fwd/gfx/buffer.h"
#include "resource/resource.hpp"

#ifdef PE_VULKAN
#include "vk/buffer.h"
#endif
#ifdef PE_GLES
#include "gles/buffer.h"
#endif

#include <variant>

namespace core::data
{
	class buffer;
}

namespace core::gfx
{
	class context;
	class buffer
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::buffer
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::buffer
#endif
			>;
	  public:
		buffer(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::gfx::context> context,
			   core::resource::handle<core::data::buffer> data);

		~buffer();

		buffer(const buffer& other)		= delete;
		buffer(buffer&& other) noexcept = delete;
		buffer& operator=(const buffer& other) = delete;
		buffer& operator=(buffer&& other) noexcept = delete;

		core::resource::handle<value_type> resource() noexcept { return m_Handle; };
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx