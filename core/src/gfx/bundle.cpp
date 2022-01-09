#include "gfx/bundle.h"
#include "data/material.h"
#include "gfx/buffer.h"
#include "gfx/geometry.h"
#include "gfx/material.h"

using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;
using namespace psl;
using namespace core::gfx::details::instance;

bundle::bundle(core::resource::cache& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::gfx::buffer> vertexBuffer,
			   core::resource::handle<core::gfx::shader_buffer_binding> materialBuffer) :
	m_UID(metaData.uid),
	m_Cache(cache), m_InstanceData(vertexBuffer, materialBuffer) {};

// ------------------------------------------------------------------------------------------------------------
// material API
// ------------------------------------------------------------------------------------------------------------

std::optional<core::resource::handle<core::gfx::material>> bundle::get(uint32_t renderlayer) const noexcept
{
	if(auto it = std::find(std::begin(m_Layers), std::end(m_Layers), renderlayer); it != std::end(m_Layers))
	{
		auto index = std::distance(std::begin(m_Layers), std::prev(it));
		return m_Materials[index];
	}
	return std::nullopt;
}

bool bundle::has(uint32_t renderlayer) const noexcept
{
	return std::find(std::begin(m_Layers), std::end(m_Layers), renderlayer) != std::end(m_Layers);
}

void bundle::set_material(handle<core::gfx::material> material, std::optional<uint32_t> render_layer_override)
{
	uint32_t layer = render_layer_override.value_or(material->data().render_layer());
	size_t index {};
	if(auto it = std::upper_bound(std::begin(m_Layers), std::end(m_Layers), layer);
	   it != std::begin(m_Layers) && *std::prev(it) == layer)
	{
		index = std::distance(std::begin(m_Layers), std::prev(it));
		m_InstanceData.add(material);
		m_InstanceData.remove(m_Materials[index]);
		m_Materials[index] = material;
	}
	else
	{
		index = std::distance(std::begin(m_Layers), it);
		m_Layers.insert(it, layer);
		m_Materials.insert(std::next(std::begin(m_Materials), index), material);
		m_InstanceData.add(material);
	}
}

// ------------------------------------------------------------------------------------------------------------
// render API
// ------------------------------------------------------------------------------------------------------------

psl::array<uint32_t> bundle::materialIndices(uint32_t begin, uint32_t end) const noexcept
{
	psl::array<uint32_t> indices {};
	for(auto layer : m_Layers)
	{
		if(layer >= begin && layer < end) indices.emplace_back(layer);
	}
	return indices;
}

bool bundle::bind_material(uint32_t renderlayer) noexcept
{
	m_Bound = {};
	if(auto it = std::find(std::begin(m_Layers), std::end(m_Layers), renderlayer); it != std::end(m_Layers))
	{
		auto index = std::distance(std::begin(m_Layers), it);
		m_Bound	   = m_Materials[index];
		if(!m_InstanceData.has_data(m_Bound) || m_InstanceData.bind_material(m_Bound)) return true;

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

uint32_t bundle::instances(core::resource::tag<core::gfx::geometry> geometry) const noexcept
{
	return m_InstanceData.count(geometry);
}

std::vector<std::pair<uint32_t, uint32_t>>
bundle::instantiate(core::resource::tag<core::gfx::geometry> geometry, uint32_t count, geometry_type type)
{
	return m_InstanceData.add(geometry, count);
}

uint32_t bundle::size(tag<core::gfx::geometry> geometry) const noexcept { return m_InstanceData.count(geometry); }
bool bundle::has(tag<core::gfx::geometry> geometry) const noexcept { return size(geometry) > 0; }

bool bundle::release(tag<core::gfx::geometry> geometry, uint32_t id) noexcept
{
	return m_InstanceData.erase(geometry, id);
}

bool bundle::release_all(std::optional<geometry_type> type) noexcept { return m_InstanceData.clear(); };

bool bundle::set(tag<core::gfx::geometry> geometry,
				 uint32_t id,
				 memory::segment segment,
				 uint32_t size_of_element,
				 const void* data,
				 size_t size,
				 size_t count)
{
	return m_InstanceData.vertex_buffer()->commit({core::gfx::commit_instruction {
	  (void*)data, size * count, segment, memory::range {size_of_element * id, size_of_element * (id + count)}}});
}


bool bundle::set(tag<core::gfx::material> material, const void* data, size_t size, size_t offset)
{
	return m_InstanceData.set(material, data, size, offset);
}
