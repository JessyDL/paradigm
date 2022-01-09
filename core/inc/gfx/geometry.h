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
#ifdef PE_VULKAN
		explicit geometry(core::resource::handle<core::ivk::geometry>& handle);
#endif
#ifdef PE_GLES
		explicit geometry(core::resource::handle<core::igles::geometry>& handle);
#endif

		geometry(core::resource::cache& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<context> context,
				 core::resource::handle<core::data::geometry> data,
				 core::resource::handle<buffer> geometryBuffer,
				 core::resource::handle<buffer> indicesBuffer);
		~geometry();
		geometry(const geometry&) = delete;
		geometry(geometry&&)	  = delete;
		geometry& operator=(const geometry&) = delete;
		geometry& operator=(geometry&&) = delete;


		void recreate(core::resource::handle<core::data::geometry> data);
		void recreate(core::resource::handle<core::data::geometry> data,
					  core::resource::handle<core::gfx::buffer> geometryBuffer,
					  core::resource::handle<core::gfx::buffer> indicesBuffer);
		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<geometry, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

		size_t vertices() const noexcept;
		size_t indices() const noexcept;
		size_t triangles() const noexcept;

	  private:
		core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::geometry> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::geometry> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx