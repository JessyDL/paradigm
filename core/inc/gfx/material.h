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
		  using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::material
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::material
#endif
			>;
		using value_type = alias_type;
		material(core::resource::handle<value_type>& handle);
		material(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 core::resource::handle<context> context_handle,
				 core::resource::handle<core::data::material> data,
				 core::resource::handle<pipeline_cache> pipeline_cache, core::resource::handle<buffer> materialBuffer);

		~material() = default;

		material(const material& other)		= delete;
		material(material&& other) noexcept = delete;
		material& operator=(const material& other) = delete;
		material& operator=(material&& other) noexcept = delete;


		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

		const core::data::material& data() const;
		void bind_instance_data(core::resource::handle<core::gfx::buffer> buffer, memory::segment segment);
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx