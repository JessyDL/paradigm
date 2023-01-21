#pragma once
#include "core/meta/shader.hpp"
#include "psl/generator.hpp"
#include "psl/array.hpp"
#include "psl/memory/segment.hpp"
#include "psl/meta.hpp"
#include "psl/sparse_array.hpp"
#include "core/resource/resource.hpp"

namespace std {
#ifdef _MSC_VER
template <typename T>
struct hash;
#endif
}	 // namespace std

namespace core::gfx {
class buffer_t;
struct shader_buffer_binding;
class geometry_t;
class material_t;
}	 // namespace core::gfx

namespace core::gfx::details::instance {
struct binding {
	struct header final {
		bool operator==(const header& b) const noexcept {
			return /*size_of_element == b.size_of_element && */ name == b.name;
		}
		psl::string name {};
		uint32_t size_of_element {0};
	};

	bool operator==(const binding& b) const noexcept { return description == b.description; }
	header description;
	uint32_t slot;
};

struct object final {
	object(psl::UID uid) : geometry(uid), id_generator(0) {};
	object(psl::UID uid, uint32_t capacity) : geometry(uid), id_generator(capacity) {};

	bool operator==(const object& rhs) const noexcept { return rhs.geometry == geometry; }

	const psl::UID geometry;	// Defines which geometry this object maps to
	psl::generator<uint32_t> id_generator;
	psl::array<binding::header> description;
	psl::array<memory::segment> data;
};
}	 // namespace core::gfx::details::instance


namespace std {
template <>
struct hash<core::gfx::details::instance::object> {
	std::size_t operator()(const core::gfx::details::instance::object& s) const noexcept {
		return std::hash<psl::UID> {}(s.geometry);
	}
	std::size_t operator()(const psl::UID& s) const noexcept { return std::hash<psl::UID> {}(s); }
};

template <>
struct hash<core::gfx::details::instance::binding::header> {
	std::size_t operator()(const core::gfx::details::instance::binding::header& s) const noexcept {
		std::size_t seed = std::hash<psl::string> {}(s.name);
		// seed ^= (uint64_t)s.size_of_element + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

template <>
struct hash<core::gfx::details::instance::binding> {
	std::size_t operator()(const core::gfx::details::instance::binding& s) const noexcept {
		return std::hash<core::gfx::details::instance::binding::header> {}(s.description);
	}
};
}	 // namespace std


namespace core::gfx::details::instance {
/// \brief handles instance data associated to materials/geometry combinations
/// \details Manages instance data related to materials-geometry, both the geometry specific data (such as instance
/// position, etc...), as well as material-wide instance data (shared with all drawcalls using this specific
/// instance set). The latter could be visualised as all pieces of foliage sharing the same "wind intensity" value.
/// This is not to be confused as "global data", such as all pieces of geometry knowing about "fog", it is still
/// duplicated over every instance of a bundle.
class data final {
	struct material_instance_data {
		core::meta::shader::descriptor descriptor;
		size_t size;
		memory::segment segment;
	};

  public:
	data() = default;
	~data();
	data(core::resource::handle<core::gfx::buffer_t> vertexBuffer,
		 core::resource::handle<core::gfx::shader_buffer_binding> materialBuffer) noexcept;
	void add(core::resource::handle<core::gfx::material_t> material);
	std::vector<std::pair<uint32_t, uint32_t>> add(core::resource::tag<core::gfx::geometry_t> uid, uint32_t count = 1);

	bool remove(core::resource::handle<core::gfx::material_t> material) noexcept;


	bool has_element(core::resource::tag<core::gfx::geometry_t> geometry, psl::string_view name) const noexcept;
	std::optional<std::pair<memory::segment, uint32_t>> segment(core::resource::tag<core::gfx::geometry_t> geometry,
																psl::string_view name) const noexcept;
	uint32_t count(core::resource::tag<core::gfx::geometry_t> uid) const noexcept;

	psl::array<std::pair<size_t, std::uintptr_t>>
	bindings(core::resource::tag<core::gfx::material_t> material,
			 core::resource::tag<core::gfx::geometry_t> geometry) const noexcept;

	core::resource::handle<core::gfx::buffer_t> vertex_buffer() const noexcept { return m_VertexInstanceBuffer; }
	core::resource::handle<core::gfx::buffer_t> material_buffer() const noexcept;

	bool erase(core::resource::tag<core::gfx::geometry_t> geometry, uint32_t id) noexcept;
	bool clear(core::resource::tag<core::gfx::geometry_t> geometry) noexcept;
	bool clear() noexcept;

	bool
	set(core::resource::tag<core::gfx::material_t> material, const void* data, size_t size, size_t offset) noexcept;

	/// \returns the offset of the material data's member.
	/// \remark nested declarations (like struct within binding), must be seperated by a '.', so that the chain is
	/// respected and looks like this "data.player.rotation". \remark array based declarations must be indexed with
	/// the bracket operator '[i]', otherwise it will default to '[0]' implicitly.
	size_t offset_of(core::resource::tag<core::gfx::material_t> material, psl::string_view name) const noexcept;


	/**
	 * \brief bind the given material's instance data (if present).
	 * \returns true if there was instance data found, otherwise propogates lower failure.
	 */
	bool bind_material(core::resource::handle<core::gfx::material_t> material);

	/*
	 * \brief returns true if the material has instance data.
	 */
	bool has_data(core::resource::handle<core::gfx::material_t> material) const noexcept;

  private:
	std::unordered_map<psl::UID, psl::array<binding>> m_Bindings;		  // <material_t, bindings[]>
	psl::array<std::pair<binding::header, uint32_t>> m_UniqueBindings;	  // unique binding and usage count
	std::unordered_map<psl::UID, object> m_InstanceData;				  // <geometry_t, object>
	std::unordered_map<psl::UID, material_instance_data> m_MaterialInstanceData {};
	psl::array<size_t> m_MaterialDataSizes {};
	core::resource::handle<core::gfx::buffer_t> m_VertexInstanceBuffer;
	core::resource::handle<core::gfx::shader_buffer_binding> m_MaterialInstanceBuffer;
};
}	 // namespace core::gfx::details::instance
