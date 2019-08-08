#pragma once
#include "fwd/resource/resource.h"
#include "memory/range.h"
#include "memory/segment.h"
#include "array.h"
#include "ustring.h"
#include "resource/handle.h"

namespace core::data
{
	class geometry;
}
namespace core::igles
{
	class buffer;

	class geometry
	{
		struct binding
		{
			psl::string name;
			memory::segment segment;
			memory::range sub_range;
		};

	  public:
		geometry(psl::UID uid, core::resource::cache& cache, core::resource::handle<core::data::geometry> data,
				 core::resource::handle<core::igles::buffer> vertexBuffer,
				 core::resource::handle<core::igles::buffer> indexBuffer);
		~geometry();
		/// \returns wether this geometry can be combined with the given material (i.e. it has
		/// all the required channels that the material needs).
		/// \param[in] material the material to check against.
		//bool compatible(const core::ivk::material& material) const noexcept;

		void bind();

	  private:
		memory::segment m_IndicesSegment;
		memory::range m_IndicesSubRange;
		core::resource::handle<core::igles::buffer> m_GeometryBuffer;
		core::resource::handle<core::igles::buffer> m_IndicesBuffer;

		psl::array <binding> m_Bindings;
	};
} // namespace core::igles