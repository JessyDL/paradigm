#pragma once
#include "fwd/gfx/material.h"
#include "resource/resource.hpp"


namespace core::data
{
	class material;
}

namespace memory
{
	class segment;
}

namespace core::gfx
{
	class context;
	class pipeline_cache;
	class buffer;

	class material
	{
	  public:
#ifdef PE_VULKAN
		explicit material(core::resource::handle<core::ivk::material>& handle);
#endif
#ifdef PE_GLES
		explicit material(core::resource::handle<core::igles::material>& handle);
#endif


		material(core::resource::cache& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<context> context_handle,
				 core::resource::handle<core::data::material> data,
				 core::resource::handle<pipeline_cache> pipeline_cache,
				 core::resource::handle<buffer> materialBuffer);

		~material() = default;

		material(const material& other)		= delete;
		material(material&& other) noexcept = delete;
		material& operator=(const material& other) = delete;
		material& operator=(material&& other) noexcept = delete;


		const core::data::material& data() const;
		bool bind_instance_data(uint32_t slot, uint32_t offset);
		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<material, backend>> resource() const noexcept
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
		core::resource::handle<core::ivk::material> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::material> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx