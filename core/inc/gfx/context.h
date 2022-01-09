#pragma once
#include "fwd/gfx/context.h"
#include "limits.h"
#include "resource/resource.hpp"
#include "types.h"

namespace core::os
{
	class surface;
}

#include <variant>

namespace core::gfx
{
	class context
	{
	  public:
#ifdef PE_VULKAN
		explicit context(core::resource::handle<core::ivk::context>& handle);
#endif
#ifdef PE_GLES
		explicit context(core::resource::handle<core::igles::context>& handle);
#endif

		context(core::resource::cache& cache,
				const core::resource::metadata& metaData,
				psl::meta::file* metaFile,
				graphics_backend backend,
				const psl::string8_t& name);

		~context() {};

		context(const context& other)	  = delete;
		context(context&& other) noexcept = delete;
		context& operator=(const context& other) = delete;
		context& operator=(context&& other) noexcept = delete;

		graphics_backend backend() const noexcept { return m_Backend; }

		const core::gfx::limits& limits() const noexcept;
		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<context, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};
		void wait_idle();

	  private:
		core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::context> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::context> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx