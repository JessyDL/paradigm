#pragma once
#include "fwd/resource/resource.h"
#include "psl/memory/range.h"
#include "psl/memory/segment.h"
#include "psl/array.h"
#include "psl/ustring.h"
#include "resource/handle.h"
#include <unordered_map>

namespace core::data
{
	class geometry;
}
namespace core::igles
{
	class buffer;
	class material;

	class geometry
	{
		struct binding
		{
			psl::string name;
			memory::segment segment;
			memory::range sub_range;
		};

	  public:
		geometry(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 core::resource::handle<core::data::geometry> data,
				 core::resource::handle<core::igles::buffer> vertexBuffer,
				 core::resource::handle<core::igles::buffer> indexBuffer);
		~geometry();

		void recreate(core::resource::handle<core::data::geometry> data);
		void recreate(core::resource::handle<core::data::geometry> data,
			core::resource::handle<core::igles::buffer> vertexBuffer,
			core::resource::handle<core::igles::buffer> indexBuffer);
		/// \returns wether this geometry can be combined with the given material (i.e. it has
		/// all the required channels that the material needs).
		/// \param[in] material the material to check against.
		// bool compatible(const core::ivk::material& material) const noexcept;

		void create_vao(core::resource::handle<core::igles::material> material,
						core::resource::handle<core::igles::buffer> instanceBuffer,
						psl::array<std::pair<size_t, size_t>> bindings);

		void bind(core::resource::handle<core::igles::material> material, uint32_t instanceCount = 0);
		bool compatible(const core::igles::material& material) const noexcept;
	  private:
		  void clear();
		psl::UID m_UID;
		memory::segment m_IndicesSegment;
		memory::range m_IndicesSubRange;
		core::resource::handle<core::igles::buffer> m_GeometryBuffer;
		core::resource::handle<core::igles::buffer> m_IndicesBuffer;

		std::unordered_map<psl::UID, unsigned int> m_VAOs;
		psl::array<binding> m_Bindings;
	};
} // namespace core::igles