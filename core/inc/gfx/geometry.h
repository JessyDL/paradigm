#pragma once
#include "fwd/gfx/geometry.h"
#include "resource/resource.hpp"

namespace core::data
{
	class geometry;
}

namespace core::gfx
{
	class context;
	class buffer;

	class geometry
	{
	  public:
		  using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::geometry
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::geometry
#endif
			>;
		using value_type = alias_type;
		geometry(core::resource::handle<value_type>& handle);
		geometry(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 core::resource::handle<context> context, core::resource::handle<core::data::geometry> data,
				 core::resource::handle<buffer> geometryBuffer, core::resource::handle<buffer> indicesBuffer);
		~geometry();
		geometry(const geometry&) = delete;
		geometry(geometry&&)	  = delete;
		geometry& operator=(const geometry&) = delete;
		geometry& operator=(geometry&&) = delete;

		core::resource::handle<value_type> resource() const noexcept { return m_Handle; };

		void recreate(core::resource::handle<core::data::geometry> data);
		void recreate(core::resource::handle<core::data::geometry> data,
			core::resource::handle<core::gfx::buffer> geometryBuffer,
			core::resource::handle<core::gfx::buffer> indicesBuffer);
	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx