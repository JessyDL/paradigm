#include "stdafx.h"
#include "gfx/details/instance.h"
#include "gfx/material.h"
#include "data/material.h"
#include "vk/shader.h"
#include "meta/shader.h"
#include "vk/buffer.h"

using namespace core::gfx;
using namespace core::gfx::details::instance;

constexpr uint32_t default_capacity = 8;

void data::add(core::resource::handle<material> material)
{
	if(m_Bindings.find(material.ID()) != std::end(m_Bindings)) return;

	auto& data = m_Bindings[material.ID()];

	for(const auto& stage : material->data()->stages())
	{
		if(stage.shader_stage() != vk::ShaderStageFlagBits::eVertex) continue;

		auto shader_handle = material.cache().find<core::gfx::shader>(stage.shader());

		size_t max = 0;
		for(const auto& vBinding : shader_handle->meta()->instance_bindings())
		{
			max = std::max(max, (size_t)vBinding.binding_slot() + 1);
		}
		data.resize(max);
		for(const auto& vBinding : shader_handle->meta()->instance_bindings())
		{
			data[vBinding.binding_slot()] = binding{vBinding.buffer(), vBinding.size()};
		}
	}

	for(const auto& d : data)
	{
		if(d.size_of_element == 0) continue;

		auto it = std::find_if(std::begin(m_UniqueBindings), std::end(m_UniqueBindings),
							   [&d](const auto& pair) { return pair.first == d; });
		if(it == std::end(m_UniqueBindings))
		{
			m_UniqueBindings.emplace_back(std::pair<binding, uint32_t>{d, 0});

			for(auto& [uid, obj] : m_InstanceData)
			{
				auto res = m_InstanceBuffer->reserve(obj.id_generator.capacity() * d.size_of_element);
				if(!res) core::gfx::log->error("could not allocate");

				obj.data.emplace_back(res.value());
			}
		}
		else
			it->second += 1;
	}
}

uint32_t data::add(core::resource::tag<core::gfx::geometry> uid)
{
	auto it = m_InstanceData.find(uid.uid());
	if(it == std::end(m_InstanceData))
	{
		it = m_InstanceData.emplace(object{uid, default_capacity}).first;
		for(const auto& b : m_UniqueBindings)
		{
			auto res = m_InstanceBuffer->reserve(it->second.id_generator.capacity() * b.first.size_of_element);
			if(!res) core::gfx::log->error("could not allocate");
			it->second.data.emplace_back(res.value());
		}
	}

	if(it->second.id_generator.available() == 0)
	{
		auto size = it->second.id_generator.size();
		if(!it->second.id_generator.resize(it->second.id_generator.size() * 2))
			core::gfx::log->error("could not increase the id_generator size");
		else
		{
			for(auto& d : it->second.data)
			{
				auto res = m_InstanceBuffer->reserve(it->second.id_generator.capacity() * d.range().size() / size);
				if(!res) core::gfx::log->error("could not allocate");
				m_InstanceBuffer->copy_from(m_InstanceBuffer, {vk::BufferCopy{d.range().begin, d.range().end}});
				std::swap(d, res.value());
				m_InstanceBuffer->deallocate(res.value());
			}
		}
	}

	return it->second.id_generator.create();
}


void data::remove(core::resource::handle<material> material)
{
	for(const auto& stage : material->data()->stages())
	{
		auto shader_handle = material.cache().find<core::gfx::shader>(stage.shader());
		for(const auto& vBinding : shader_handle->meta()->instance_bindings())
		{
			if(auto it = m_Bindings.find(binding{vBinding.buffer(), vBinding.size(), stage.shader_stage()});
			   it != std::end(m_Bindings))
			{
				it->second -= 1;
				if(it->second == 0)
				{
					for(auto& [uid, instance_object] : m_InstanceData)
					{
						if(instance_object. == it->first.name &&
					}
				}
			}
		}
	}
}


uint32_t data::count(core::resource::tag<core::gfx::geometry> uid) const noexcept
{
	if(auto it = m_InstanceData.find(uid.uid()); it != std::end(m_InstanceData))
	{
		return it->second.id_generator.size();
	}
	return 0;
}