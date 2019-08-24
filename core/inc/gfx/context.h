#pragma once
#include "fwd/gfx/context.h"
#include "resource/resource.hpp"
#include "types.h"

namespace core::os
{
	class surface;
}

#include <variant>

namespace core::gfx
{	class context
	{
	  public:
		  using alias_type = core::resource::alias<
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
		using value_type = alias_type;
		context(core::resource::handle<value_type>& handle);
		context(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				graphics_backend backend,
				const psl::string8_t& name);

		~context(){};

		context(const context& other)	 = delete;
		context(context&& other) noexcept = delete;
		context& operator=(const context& other) = delete;
		context& operator=(context&& other) noexcept = delete;

		graphics_backend backend() const noexcept { return m_Backend; }
		
		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

	  private:
		graphics_backend m_Backend;
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx