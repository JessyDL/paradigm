#pragma once
#include "core/meta/shader.hpp"
#include "core/resource/resource.hpp"
#include "psl/array.hpp"
#include "psl/generator.hpp"
#include "psl/memory/segment.hpp"
#include "psl/meta.hpp"
#include "psl/sparse_array.hpp"

// bundles consist out of N instances of unique geometry, and M instances of unique materials
// they manage the instance data associated to these geometry/material combinations.
// additionally they manage the instance data associated to the materials themselves (only 1 per material).
// This data is shared between all drawcalls using this bundle.

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

struct storage_link {
	struct swap_command {
		uint32_t src;
		uint32_t dst;
		uint32_t count;
	};
	struct resize_command {
		uint32_t new_size;
	};
	uint32_t m_MaxSize {std::numeric_limits<uint32_t>::max()};
	std::vector<std::variant<swap_command, resize_command>> m_Commands;
};

/// \brief A virtual storage buffer that records commands instead of executing them
/// \details This storage buffer does not actually store any data, but instead records commands that can
/// be executed later. This is useful for recording operations that need to be performed on a GPU buffer.
/// This is used as the backing storage for instance data in a `psl::sparse_array`.
template <typename Key>
class gpu_storage_buffer {
  public:
	gpu_storage_buffer([[maybe_unused]] Key max_size, std::shared_ptr<storage_link> link) : m_Link(link) {}
	Key capacity() const noexcept {
		return m_Link->m_MaxSize;
	}

	void swap([[maybe_unused]] Key first, [[maybe_unused]] Key second) {
		m_Link->m_Commands.push_back(storage_link::swap_command {second, first, 1});
	}
	void reserve([[maybe_unused]] Key new_size) {
		// reserve does nothing, as this is a virtual buffer
		// we only resize when needed
	}
	void insert_space([[maybe_unused]] Key index, [[maybe_unused]] Key count) {
		m_Size += count;
		m_Link->m_Commands.push_back(storage_link::resize_command {m_Size});
		// m_Link->m_Commands.push_back(storage_link::swap_command {index, index + count, m_Size - (index + count)});
	}
	void truncate([[maybe_unused]] Key new_size) {
		m_Size = new_size;
		m_Link->m_Commands.push_back(storage_link::resize_command {m_Size});
	}
	void clear([[maybe_unused]] bool release_memory = false) noexcept {
		m_Size = 0;
		m_Link->m_Commands.push_back(storage_link::resize_command {m_Size});
	}

	void emplace_back() {
		++m_Size;
		m_Link->m_Commands.push_back(storage_link::resize_command {m_Size});
	};

  private:
	std::shared_ptr<storage_link> m_Link;
	Key m_Size {0};
};
struct binding {
	struct header final {
		bool operator==(const header& b) const noexcept {
			return /*size_of_element == b.size_of_element && */ name == b.name;
		}
		psl::string name {};
		uint32_t size_of_element {0};
	};

	bool operator==(const binding& b) const noexcept {
		return description == b.description;
	}
	header description;
	uint32_t slot;
};

// for every unique geometry we will have an instance of this that will manage the instance data
// associated to this geometry for all materials in the bundle.
struct geometry_instance_data {
	struct entry {
		memory::segment memory;
		binding::header description;
		uint32_t slot;
	};

	geometry_instance_data(uint32_t capacity) : manager(capacity, m_Link), instance_data(), m_Max(capacity) {}

	std::shared_ptr<storage_link> m_Link {std::make_shared<storage_link>()};
	psl::sparse_indice_array<std::uint32_t,
							 std::uint32_t,
							 4096,
							 psl::details::default_buffer_growth_strategy_t,
							 gpu_storage_buffer<std::uint32_t>>
	  manager;
	std::vector<entry> instance_data;

	uint32_t m_Head {0};				// Head is always the highest allocated id + 1
	std::vector<uint32_t> m_Orphans;	// Orphans are ids that were allocated but later freed, we can reuse these.
										// Note that these will never be compacted.
	uint32_t m_Max {0};					// Maximum number of instances allowed, set to numeric_limits::max to disable

	uint32_t available() const noexcept;
	size_t capacity() const noexcept;
	void capacity(uint32_t max);
	std::vector<uint32_t> add(uint32_t count);
	uint32_t size() const noexcept;
	void erase(uint32_t id);
	void erase(auto&& first, auto&& last);
	uint32_t offset_of(uint32_t id) const noexcept;
	void clear() noexcept;
	psl::array<core::gfx::memory_copy> consume();
};

}	 // namespace core::gfx::details::instance

namespace std {

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
	std::vector<uint32_t> add(core::resource::tag<core::gfx::geometry_t> uid, uint32_t count = 1);

	bool remove(core::resource::handle<core::gfx::material_t> material) noexcept;


	bool has_element(core::resource::tag<core::gfx::geometry_t> geometry, psl::string_view name) const noexcept;
	std::optional<std::pair<memory::segment, uint32_t>> segment(core::resource::tag<core::gfx::geometry_t> geometry,
																psl::string_view name) const noexcept;
	uint32_t count(core::resource::tag<core::gfx::geometry_t> uid) const noexcept;

	psl::array<std::pair<size_t, std::uintptr_t>>
	bindings(core::resource::tag<core::gfx::material_t> material,
			 core::resource::tag<core::gfx::geometry_t> geometry) const noexcept;

	core::resource::handle<core::gfx::buffer_t> vertex_buffer() const noexcept {
		return m_VertexInstanceBuffer;
	}
	core::resource::handle<core::gfx::buffer_t> material_buffer() const noexcept;

	bool erase(core::resource::tag<core::gfx::geometry_t> geometry, uint32_t id) noexcept;
	bool erase(core::resource::tag<core::gfx::geometry_t> geometry, std::span<uint32_t const> ids) noexcept;
	bool clear(core::resource::tag<core::gfx::geometry_t> geometry) noexcept;
	bool clear() noexcept;

	bool
	set(core::resource::tag<core::gfx::material_t> material, const void* data, size_t size, size_t offset) noexcept;

	/// \returns the offset of the material data's member.
	/// \remark nested declarations (like struct within binding), must be seperated by a '.', so that the chain is
	/// respected and looks like this "data.player.rotation". \remark array based declarations must be indexed with
	/// the bracket operator '[i]', otherwise it will default to '[0]' implicitly.
	size_t offset_of(core::resource::tag<core::gfx::material_t> material, psl::string_view name) const noexcept;

	size_t offset_of(core::resource::tag<core::gfx::geometry_t> geometry, std::uint32_t id) const noexcept;

	void apply();

  public:
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
	std::unordered_map<psl::UID, material_instance_data> m_MaterialInstanceData {};
	psl::array<size_t> m_MaterialDataSizes {};
	core::resource::handle<core::gfx::buffer_t> m_VertexInstanceBuffer;
	core::resource::handle<core::gfx::shader_buffer_binding> m_MaterialInstanceBuffer;
	std::unordered_map<psl::UID, geometry_instance_data>
	  m_GeometryInstanceData;	 // <geometry_t, geometry_instance_data>
};
}	 // namespace core::gfx::details::instance
