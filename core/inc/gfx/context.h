#pragma once
#include "fwd/gfx/context.h"
#include "resource/resource.hpp"

#ifdef PE_VULKAN
namespace core::ivk
{
	class context;
}
#endif
#ifdef PE_GLES
namespace core::igles
{
	class context;
}
#endif
namespace core::os
{
	class surface;
}

#include <variant>

namespace core::gfx
{
	enum class graphics_backend
	{
		vulkan = 1 << 0,
		gles   = 1 << 1
	};

	class context
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::context
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::context
#endif
			>;
	  public:
		context(const psl::UID& uid, core::resource::cache& cache, graphics_backend backend,
				const psl::string8_t& name);

		~context() = default;

		context(const context& other)	 = delete;
		context(context&& other) noexcept = delete;
		context& operator=(const context& other) = delete;
		context& operator=(context&& other) noexcept = delete;

		graphics_backend backend() const noexcept { return m_Backend; }

		void target_surface(const core::os::surface& surface);

		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

	  private:
		graphics_backend m_Backend;
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx