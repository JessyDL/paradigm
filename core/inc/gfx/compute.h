#pragma once
#include "fwd/gfx/compute.h"
#include "gfx/types.h"
#include "psl/math/vec.h"
#include "resource/resource.hpp"

namespace core::data
{
	class material_t;
}
namespace core::gfx
{
	class context;
	class pipeline_cache;

	class compute;
#ifdef PE_VULKAN
	template <>
	struct backend_type<compute, graphics_backend::vulkan>
	{
		using type = core::ivk::compute;
	};
#endif
#ifdef PE_GLES
	template <>
	struct backend_type<compute, graphics_backend::gles>
	{
		using type = core::igles::compute;
	};
#endif

	class compute
	{
		friend class core::resource::cache_t;

	  public:
#ifdef PE_VULKAN
		explicit compute(core::resource::handle<core::ivk::compute>& handle);
#endif
#ifdef PE_GLES
		explicit compute(core::resource::handle<core::igles::compute>& handle);
#endif

		compute(core::resource::cache_t& cache,
				const core::resource::metadata& metaData,
				core::meta::shader* metaFile,
				core::resource::handle<context> context_handle,
				core::resource::handle<core::data::material_t> data,
				core::resource::handle<pipeline_cache> pipeline_cache);
		~compute() = default;

		compute(const compute& other)	  = default;
		compute(compute&& other) noexcept = default;
		compute& operator=(const compute& other) = default;
		compute& operator=(compute&& other) noexcept = default;


		core::meta::shader* meta() const noexcept;

		void dispatch(const psl::static_array<uint32_t, 3>& size);

		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<compute, backend>> resource() const noexcept
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
		core::resource::handle<core::ivk::compute> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::compute> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx