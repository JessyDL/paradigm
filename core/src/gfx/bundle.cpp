#include "core/gfx/bundle.hpp"
#include "core/data/material.hpp"
#include "core/gfx/buffer.hpp"
#include "core/gfx/geometry.hpp"
#include "core/gfx/material.hpp"

using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;
using namespace psl;
using namespace core::gfx::details::instance;

bundle::bundle(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::gfx::buffer_t> vertexBuffer,
			   core::resource::handle<core::gfx::shader_buffer_binding> materialBuffer)
	: m_UID(metaData.uid), m_Cache(cache), m_InstanceData(vertexBuffer, materialBuffer) {};

// ------------------------------------------------------------------------------------------------------------
// material API
// ------------------------------------------------------------------------------------------------------------

std::optional<core::resource::handle<core::gfx::material_t>> bundle::get(uint32_t renderlayer) const noexcept {
	if(auto it = std::find(std::begin(m_Layers), std::end(m_Layers), renderlayer); it != std::end(m_Layers)) {
		auto index = std::distance(std::begin(m_Layers), std::prev(it));
		return m_Materials[index];
	}
	return std::nullopt;
}

bool bundle::has(uint32_t renderlayer) const noexcept {
	return std::find(std::begin(m_Layers), std::end(m_Layers), renderlayer) != std::end(m_Layers);
}

void bundle::set_material(handle<core::gfx::material_t> material, std::optional<uint32_t> render_layer_override) {
	uint32_t layer = render_layer_override.value_or(material->data().render_layer());
	size_t index {};
	if(auto it = std::upper_bound(std::begin(m_Layers), std::end(m_Layers), layer);
	   it != std::begin(m_Layers) && *std::prev(it) == layer) {
		index = std::distance(std::begin(m_Layers), std::prev(it));
		m_InstanceData.add(material);
		m_InstanceData.remove(m_Materials[index]);
		m_Materials[index] = material;
	} else {
		index = std::distance(std::begin(m_Layers), it);
		m_Layers.insert(it, layer);
		m_Materials.insert(std::next(std::begin(m_Materials), index), material);
		m_InstanceData.add(material);
	}
}

// ------------------------------------------------------------------------------------------------------------
// render API
// ------------------------------------------------------------------------------------------------------------

psl::array<uint32_t> bundle::materialIndices(uint32_t begin, uint32_t end) const noexcept {
	psl::array<uint32_t> indices {};
	for(auto layer : m_Layers) {
		if(layer >= begin && layer < end)
			indices.emplace_back(layer);
	}
	return indices;
}

bool bundle::bind_material(uint32_t renderlayer) noexcept {
	m_Bound = {};
	if(auto it = std::find(std::begin(m_Layers), std::end(m_Layers), renderlayer); it != std::end(m_Layers)) {
		auto index = std::distance(std::begin(m_Layers), it);
		m_Bound	   = m_Materials[index];
		if(!m_InstanceData.has_data(m_Bound) || m_InstanceData.bind_material(m_Bound))
			return true;

		core::gfx::log->error(
		  "could not bind the material {} due to an issue updating a binding offset, inspect prior log for more info",
		  m_Bound.uid().to_string());
		m_Bound = {};
	}
	return false;
}
// ------------------------------------------------------------------------------------------------------------
// instance data API
// ------------------------------------------------------------------------------------------------------------

instancing_size_type bundle::instances(core::resource::tag<core::gfx::geometry_t> geometry) const noexcept {
	return m_InstanceData.count(geometry);
}

std::vector<instancing_size_type> bundle::instantiate(core::resource::tag<core::gfx::geometry_t> geometry,
													  instancing_size_type count,
													  geometry_type type) {
	return m_InstanceData.add(geometry, count);
}

instancing_size_type bundle::size(tag<core::gfx::geometry_t> geometry) const noexcept {
	return m_InstanceData.count(geometry);
}
bool bundle::has(tag<core::gfx::geometry_t> geometry) const noexcept {
	return size(geometry) > 0;
}

bool bundle::release(tag<core::gfx::geometry_t> geometry, instancing_size_type id) noexcept {
	return m_InstanceData.erase(geometry, id);
}

bool bundle::release(tag<core::gfx::geometry_t> geometry, std::span<instancing_size_type const> ids) noexcept {
	return m_InstanceData.erase(geometry, ids);
}

bool bundle::release_all(std::optional<geometry_type> type) noexcept {
	return m_InstanceData.clear();
};

bool bundle::set(core::resource::tag<core::gfx::geometry_t> geometry,
				 std::span<instancing_size_type const> ids,
				 memory::segment segment,
				 uint32_t size_of_element,
				 std::byte* data,
				 size_t size) {
	if(ids.size() == 0) {
		return true;
	}
	struct range {
		instancing_size_type begin, end;
		std::byte *data_begin, *data_end;
	};
	std::vector<range> ranges {};
	ranges.reserve(ids.size());
	auto data_offset = data;
	{
		auto index_of = m_InstanceData.index_of(geometry, ids[0]);
		ranges.push_back(range {index_of, index_of + 1, data, data + size});
		data_offset += size;
	}

	for(auto i = 1; i < ids.size(); ++i, data_offset += size) {
		auto index_of = m_InstanceData.index_of(geometry, ids[i]);
		if(ranges.back().end == index_of) {
			ranges.back().end = index_of + 1;
			ranges.back().data_end += size;
		} else {
			ranges.emplace_back(range {index_of, index_of + 1, data_offset, data_offset + size});
		}
	}

	std::byte* temp = new std::byte[size * ids.size()];
	// sort the ranges, and swap the memory of the data pointer accordingly.
	std::sort(std::begin(ranges), std::end(ranges), [](const range& a, const range& b) { return a.begin < b.begin; });

	// now copy the data over to a temp buffer in the right order.
	size_t offset = 0;
	for(auto& range : ranges) {
		auto range_size = range.data_end - range.data_begin;
		std::memcpy(temp + offset, range.data_begin, range_size);
		range.data_begin = temp + offset;
		range.data_end	 = range.data_begin + range_size;
		offset += range_size;
	}

	// now that the ranges are sorted, we can merge them if the end == begin of the next range
	for(size_t i = 1; i < ranges.size(); ++i) {
		if(ranges[i - 1].end == ranges[i].begin) {
			auto& prev			 = ranges[i - 1];
			ranges[i].begin		 = prev.begin;
			ranges[i].data_begin = prev.data_begin;
			prev.data_begin		 = nullptr;
		}
	}

	ranges.erase(
	  std::remove_if(std::begin(ranges), std::end(ranges), [](const range& r) { return r.data_begin == nullptr; }),
	  std::end(ranges));

	psl::array<core::gfx::commit_instruction> instructions {};
	for(auto range : ranges) {
		auto range_count = range.end - range.begin;
		instructions.emplace_back(
		  core::gfx::commit_instruction {range.data_begin,
										 size * range_count,
										 segment,
										 memory::range_t {size_of_element * range.begin, size_of_element * range.end}});
	}

	auto res = m_InstanceData.vertex_buffer()->commit(instructions);
	delete[] temp;
	return res;
}


bool bundle::set(tag<core::gfx::material_t> material, const void* data, size_t size, size_t offset) {
	return m_InstanceData.set(material, data, size, offset);
}

void bundle::apply() {
	m_InstanceData.apply();
}
