#pragma once
#include "fwd/gfx/material.hpp"
#include "resource/resource.hpp"


namespace core::data
{
	class material_t;
}

namespace memory
{
	class segment;
}

namespace core::gfx
{
	class context;
	class pipeline_cache;
	class buffer_t;

	class material_t
	{
	  public:
#ifdef PE_VULKAN
		explicit material_t(core::resource::handle<core::ivk::material_t>& handle);
#endif
#ifdef PE_GLES
		explicit material_t(core::resource::handle<core::igles::material_t>& handle);
#endif


		material_t(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<context> context_handle,
				 core::resource::handle<core::data::material_t> data,
				 core::resource::handle<pipeline_cache> pipeline_cache,
				 core::resource::handle<buffer_t> materialBuffer);

		~material_t() = default;

		material_t(const material_t& other)		= delete;
		material_t(material_t&& other) noexcept = delete;
		material_t& operator=(const material_t& other) = delete;
		material_t& operator=(material_t&& other) noexcept = delete;


		const core::data::material_t& data() const;
		bool bind_instance_data(uint32_t slot, uint32_t offset);
		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<material_t, backend>> resource() const noexcept
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
		core::resource::handle<core::ivk::material_t> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::material_t> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx