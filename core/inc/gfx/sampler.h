#pragma once
#include "fwd/gfx/sampler.h"
#include "resource/resource.hpp"
#include <variant>

namespace core::data
{
	class sampler;
}

namespace core::gfx
{
	class context;

	class sampler
	{
	  public:
#ifdef PE_VULKAN
		explicit sampler(core::resource::handle<core::ivk::sampler>& handle);
#endif
#ifdef PE_GLES
		explicit sampler(core::resource::handle<core::igles::sampler>& handle);
#endif

		sampler(core::resource::cache& cache,
				const core::resource::metadata& metaData,
				psl::meta::file* metaFile,
				core::resource::handle<core::gfx::context> context,
				core::resource::handle<core::data::sampler> sampler_data);
		~sampler() = default;

		sampler(const sampler& other)	  = delete;
		sampler(sampler&& other) noexcept = delete;
		sampler& operator=(const sampler& other) = delete;
		sampler& operator=(sampler&& other) noexcept = delete;

		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<sampler, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

	  private:
		core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::sampler> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::sampler> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx