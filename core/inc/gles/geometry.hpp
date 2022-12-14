#pragma once
#include "fwd/resource/resource.hpp"
#include "psl/array.hpp"
#include "psl/memory/range.hpp"
#include "psl/memory/segment.hpp"
#include "psl/ustring.hpp"
#include "resource/handle.hpp"
#include <unordered_map>

namespace core::data {
class geometry_t;
}
namespace core::igles {
class buffer_t;
class material_t;

class geometry_t {
	struct binding {
		psl::string name;
		memory::segment segment;
		memory::range_t sub_range;
	};

  public:
	geometry_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::data::geometry_t> data,
			   core::resource::handle<core::igles::buffer_t> vertexBuffer,
			   core::resource::handle<core::igles::buffer_t> indexBuffer);
	~geometry_t();

	void recreate(core::resource::handle<core::data::geometry_t> data);
	void recreate(core::resource::handle<core::data::geometry_t> data,
				  core::resource::handle<core::igles::buffer_t> vertexBuffer,
				  core::resource::handle<core::igles::buffer_t> indexBuffer);
	/// \returns wether this geometry can be combined with the given material (i.e. it has
	/// all the required channels that the material needs).
	/// \param[in] material the material to check against.
	// bool compatible(const core::ivk::material_t& material) const noexcept;

	void create_vao(core::resource::handle<core::igles::material_t> material,
					core::resource::handle<core::igles::buffer_t> instanceBuffer,
					psl::array<std::pair<size_t, size_t>> bindings);

	void bind(core::resource::handle<core::igles::material_t> material, uint32_t instanceCount = 0);
	bool compatible(const core::igles::material_t& material) const noexcept;


	size_t vertices() const noexcept;
	size_t indices() const noexcept;
	size_t triangles() const noexcept;

  private:
	void clear(bool including_vao = true);
	psl::UID m_UID {};
	memory::segment m_IndicesSegment {};
	memory::range_t m_IndicesSubRange {};
	core::resource::handle<core::igles::buffer_t> m_GeometryBuffer {};
	core::resource::handle<core::igles::buffer_t> m_IndicesBuffer {};

	std::unordered_map<psl::UID, unsigned int> m_VAOs {};
	psl::array<binding> m_Bindings {};

	size_t m_Vertices {0};
	size_t m_Triangles {0};
};
}	 // namespace core::igles
