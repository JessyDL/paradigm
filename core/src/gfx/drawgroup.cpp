
#include "gfx/drawgroup.h"
#include "vk/material.h"
#include "vk/geometry.h"
#include "data/geometry.h"
#include "vk/pipeline.h"
#include "vk/framebuffer.h"
#include "vk/swapchain.h"
#include "vk/buffer.h"
#include "logging.h"

using namespace core::gfx;

void drawgroup::build(vk::CommandBuffer cmdBuffer, core::resource::handle<core::ivk::framebuffer> framebuffer,
					  uint32_t index)
{
	PROFILE_SCOPE(core::profiler)

	for(auto& drawLayer : m_Group)
	{
		for(auto& drawCall : drawLayer.second)
		{
			if(drawCall.m_Geometry.size() == 0) continue;
			auto bundle = drawCall.m_Bundle;

			auto matIndices = drawCall.m_Bundle->materialIndices(drawLayer.first.begin(), drawLayer.first.end());

			for(auto index : matIndices)
			{
				bundle->bind_material(index);
				bundle->bind_pipeline(cmdBuffer, framebuffer, index);
				auto mat{bundle->bound()};
				for(auto& [geometryHandle, count] : drawCall.m_Geometry)
				{
					uint32_t instance_n = bundle->instances(geometryHandle);
					if(instance_n == 0 || !geometryHandle->compatible(mat)) continue;

					geometryHandle->bind(cmdBuffer, mat);
					bundle->bind_geometry(cmdBuffer, geometryHandle);

					cmdBuffer.drawIndexed((uint32_t)geometryHandle->data()->indices().size(), instance_n, 0, 0, 0);
				}
			}
		}
	}
}


void drawgroup::build(vk::CommandBuffer cmdBuffer, core::resource::handle<core::ivk::swapchain> swapchain, uint32_t index)
{
	PROFILE_SCOPE(core::profiler)

	for(auto& drawLayer : m_Group)
	{
		for(auto& drawCall : drawLayer.second)
		{
			if(drawCall.m_Geometry.size() == 0) continue;
			auto bundle = drawCall.m_Bundle;

			auto matIndices = drawCall.m_Bundle->materialIndices(drawLayer.first.begin(), drawLayer.first.end());

			for(auto index : matIndices)
			{
				bundle->bind_material(index);
				bundle->bind_pipeline(cmdBuffer, swapchain, index);
				auto mat{bundle->bound()};
				for(auto& [geometryHandle, count] : drawCall.m_Geometry)
				{
					uint32_t instance_n = bundle->instances(geometryHandle);
					if(instance_n == 0 || !geometryHandle->compatible(mat)) continue;

					geometryHandle->bind(cmdBuffer, mat);
					bundle->bind_geometry(cmdBuffer, geometryHandle);

					cmdBuffer.drawIndexed((uint32_t)geometryHandle->data()->indices().size(), instance_n, 0, 0, 0);
				}
			}
		}
	}
}

const drawlayer& drawgroup::layer(const psl::string& layer, uint32_t priority, uint32_t extent) noexcept
{
	auto it = std::find_if(std::begin(m_Group), std::end(m_Group),
						   [&layer](const auto& element) { return element.first.name == layer; });
	if(it != std::end(m_Group)) return it->first;

	return m_Group.emplace(std::pair<drawlayer, std::vector<drawcall>>(drawlayer{layer, priority, extent}, {}))
		.first->first;
}

bool drawgroup::contains(const psl::string& layer) const noexcept
{
	return std::find_if(std::begin(m_Group), std::end(m_Group),
						[&layer](const auto& element) { return element.first.name == layer; }) != std::end(m_Group);
}
std::optional<std::reference_wrapper<const drawlayer>> drawgroup::get(const psl::string& layer) const noexcept
{
	auto it = std::find_if(std::begin(m_Group), std::end(m_Group),
						   [&layer](const auto& element) { return element.first.name == layer; });

	if(it != std::end(m_Group)) return it->first;
	return std::nullopt;
}
bool drawgroup::priority(drawlayer& layer, uint32_t priority) noexcept
{
	auto it = std::find_if(std::begin(m_Group), std::end(m_Group),
						   [&layer](const auto& element) { return element.first.name == layer.name; });

	if(it != std::end(m_Group))
	{
		std::vector<drawcall> copy = std::move(it->second);
		m_Group.erase(it);
		layer =
			m_Group
				.emplace(std::pair<drawlayer, std::vector<drawcall>>(drawlayer{layer.name, priority}, std::move(copy)))
				.first->first;
		return true;
	}
	return false;
}

drawcall& drawgroup::add(const drawlayer& layer, core::resource::handle<core::gfx::bundle> bundle) noexcept
{
	auto it = m_Group.find(layer);
	if(it != std::end(m_Group))
	{
		if(auto matIt = std::find_if(std::begin(it->second), std::end(it->second),
									 [&bundle](const drawcall& call) { return call.bundle() == bundle; });
		   matIt != std::end(it->second))
		{
			return *matIt;
		}
		else
		{
			return it->second.emplace_back(bundle);
		}
	}
	return m_Group[layer].emplace_back(bundle);
}

std::optional<std::reference_wrapper<drawcall>>
drawgroup::get(const drawlayer& layer, core::resource::handle<core::gfx::bundle> bundle) noexcept
{
	auto it = m_Group.find(layer);
	if(it != std::end(m_Group))
	{
		if(auto matIt = std::find_if(std::begin(it->second), std::end(it->second),
									 [&bundle](const drawcall& call) { return call.bundle() == bundle; });
		   matIt != std::end(it->second))
		{
			return *matIt;
		}
	}

	return std::nullopt;
}