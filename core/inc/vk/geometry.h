#pragma once
#include "fwd/gfx/context.h"
#include "psl/memory/range.hpp"
#include "psl/memory/segment.hpp"
#include "resource/resource.hpp"
#include "vk/ivk.h"
#include <vector>

namespace core::data
{
	class geometry_t;
	class material_t;
}	 // namespace core::data

namespace core::ivk
{
	class context;
	class buffer_t;
	class material_t;

	/// \brief describes the driver visible concept of geometry.
	class geometry_t
	{
		struct binding
		{
			psl::string name;
			memory::segment segment;
			memory::range_t sub_range;
		};

	  public:
		/// \brief constructs, and uploads the geometry data to the buffers.
		/// \param[in] data the geometry source data for this instance.
		/// \param[in] geometryBuffer the buffer that the mesh data will be uploaded to.
		/// \param[in] indicesBuffer the buffer that the indices data will be uploaded to.
		geometry_t(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<core::ivk::context> context,
				 core::resource::handle<core::data::geometry_t> data,
				 core::resource::handle<core::ivk::buffer_t> geometryBuffer,
				 core::resource::handle<core::ivk::buffer_t> indicesBuffer);
		~geometry_t();
		geometry_t(const geometry_t&) = delete;
		geometry_t(geometry_t&&)	  = delete;
		geometry_t& operator=(const geometry_t&) = delete;
		geometry_t& operator=(geometry_t&&) = delete;

		void recreate(core::resource::handle<core::data::geometry_t> data);
		void recreate(core::resource::handle<core::data::geometry_t> data,
					  core::resource::handle<core::ivk::buffer_t> geometryBuffer,
					  core::resource::handle<core::ivk::buffer_t> indicesBuffer);
		/// \returns wether this geometry can be combined with the given material (i.e. it has
		/// all the required channels that the material needs).
		/// \param[in] material the material to check against.
		bool compatible(const core::ivk::material_t& material) const noexcept;

		/// \brief binds the geometry to the given material for rendering.
		/// \param[in] buffer the command buffer to upload the commands to.
		/// \param[in] material the material to bind with.
		/// \warning only invoke this method in the context of recording draw instructions.
		void bind(vk::CommandBuffer& buffer, const core::ivk::material_t& material) const noexcept;

		/// \returns the geometry data used by this instance.
		core::resource::handle<core::data::geometry_t> data() const noexcept { return m_Data; };

		size_t vertices() const noexcept;
		size_t triangles() const noexcept;
		size_t indices() const noexcept;

	  private:
		void clear();
		core::resource::handle<core::ivk::context> m_Context {};
		core::resource::handle<core::data::geometry_t> m_Data {};
		core::resource::handle<core::ivk::buffer_t> m_GeometryBuffer {};
		core::resource::handle<core::ivk::buffer_t> m_IndicesBuffer {};
		memory::segment m_IndicesSegment {};
		memory::range_t m_IndicesSubRange {};
		std::vector<binding> m_Bindings {};
		const psl::UID& m_UID;
		size_t m_Vertices {0};
		size_t m_Triangles {0};
	};
}	 // namespace core::ivk
