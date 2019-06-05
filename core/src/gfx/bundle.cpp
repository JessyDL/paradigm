#include "stdafx.h"
#include "gfx/bundle.h"
#include "gfx/material.h"
#include "data/material.h"
#include "vk/buffer.h"

using namespace core::gfx;
using namespace core::resource;
using namespace psl;
using namespace core::gfx::details::instance;

bundle::bundle(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::gfx::buffer> buffer)
	: m_UID(uid), m_Cache(cache), m_InstanceData(buffer){};

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

void bundle::set(handle<material> material, std::optional<uint32_t> render_layer_override)
{
	uint32_t layer = render_layer_override.value_or(material->data()->render_layer());
	size_t index{};
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
bool bundle::bind_material(uint32_t renderlayer) noexcept 
{
	m_Bound = {};
	if(auto it = std::find(std::begin(m_Layers), std::end(m_Layers), renderlayer); it != std::end(m_Layers))
	{
		auto index = std::distance(std::begin(m_Layers), std::prev(it));
		m_Bound	= m_Materials[index];
		return true;
	}
	return false;
}
bool bundle::bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<framebuffer> framebuffer,
						   uint32_t drawIndex)
{
	if(!m_Bound) return false;

	m_Bound->bind_pipeline(cmdBuffer, framebuffer, drawIndex);
	return true;
}

bool bundle::bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<swapchain> swapchain, uint32_t drawIndex)
{
	if(!m_Bound) return false;

	m_Bound->bind_pipeline(cmdBuffer, swapchain, drawIndex);
	return true;
}

bool bundle::bind_geometry(vk::CommandBuffer cmdBuffer, const core::resource::handle<core::gfx::geometry> geometry) 
{
	if(!m_Bound) return false;


	for(const auto& b : m_InstanceData.bindings(m_Bound, geometry))
	{
		cmdBuffer.bindVertexBuffers(b.first, 1, &m_InstanceData.buffer()->gpu_buffer(), &b.second);
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------------
// instance data API
// ------------------------------------------------------------------------------------------------------------

uint32_t bundle::instances(core::resource::tag<core::gfx::geometry> geometry) const noexcept
{
	return m_InstanceData.count(geometry);
}

std::optional<uint32_t> bundle::instantiate(core::resource::tag<core::gfx::geometry> geometry)
{
	return m_InstanceData.add(geometry);
}

uint32_t bundle::size(tag<geometry> geometry) const noexcept { return m_InstanceData.count(geometry); }
bool bundle::has(tag<geometry> geometry) const noexcept { return size(geometry) > 0; }

bool bundle::release(tag<geometry> geometry, uint32_t id) noexcept
{ return m_InstanceData.erase(geometry, id); }

bool bundle::release_all() noexcept { return m_InstanceData.clear(); };

bool bundle::set(tag<geometry> geometry, uint32_t id, memory::segment segment, uint32_t size_of_element, const void* data, size_t size, size_t count)
{
	m_InstanceData.buffer()->commit({core::gfx::buffer::commit_instruction{
		(void*)data, size * count, segment,
		memory::range{size_of_element * id, size_of_element * (id + count)}}});
	return true;
}