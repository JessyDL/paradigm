#include "stdafx.h"
#include "gfx/details/instance.h"
#include "gfx/material.h"
#include "data/material.h"
#include "vk/shader.h"
#include "meta/shader.h"
#include "vk/buffer.h"

using namespace core::gfx;
using namespace core::gfx::details::instance;
using namespace core::resource;

constexpr uint32_t default_capacity = 8;

data::data(core::resource::handle<core::gfx::buffer> buffer) noexcept : m_InstanceBuffer(buffer) {}
void data::add(core::resource::handle<material> material)
{
	if(m_Bindings.find(material.ID()) != std::end(m_Bindings)) return;

	auto& data = m_Bindings[material.ID()];

	for(const auto& stage : material->data()->stages())
	{
		if(stage.shader_stage() != vk::ShaderStageFlagBits::eVertex) continue;

		auto shader_handle = material.cache().find<core::gfx::shader>(stage.shader());

		data.reserve(shader_handle->meta()->instance_bindings().size());
		for(const auto& vBinding : shader_handle->meta()->instance_bindings())
		{
			data.emplace_back(binding{binding::header{vBinding.buffer(), vBinding.size()}, vBinding.binding_slot()});
		}
	}

	for(const auto& d : data)
	{
		auto it = std::find_if(std::begin(m_UniqueBindings), std::end(m_UniqueBindings),
							   [&d](const auto& pair) { return pair.first == d.description; });
		if(it == std::end(m_UniqueBindings))
		{
			m_UniqueBindings.emplace_back(std::pair<binding::header, uint32_t>{d.description, 0});

			for(auto& [uid, obj] : m_InstanceData)
			{
				auto res = m_InstanceBuffer->reserve(obj.id_generator.capacity() * d.description.size_of_element);
				if(!res) core::gfx::log->error("could not allocate");

				obj.data.emplace_back(res.value());
				obj.description.emplace_back(d.description);
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
		it = m_InstanceData.emplace(uid, object{uid, default_capacity}).first;
		for(const auto& b : m_UniqueBindings)
		{
			auto res = m_InstanceBuffer->reserve(it->second.id_generator.capacity() * b.first.size_of_element);
			if(!res) core::gfx::log->error("could not allocate");
			it->second.data.emplace_back(res.value());
			it->second.description.emplace_back(b.first);
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


void data::remove(core::resource::handle<material> material) {}


uint32_t data::count(core::resource::tag<core::gfx::geometry> uid) const noexcept
{
	if(auto it = m_InstanceData.find(uid.uid()); it != std::end(m_InstanceData))
	{
		return it->second.id_generator.size();
	}
	return 0;
}


psl::array<std::pair<size_t, std::uintptr_t>> data::bindings(tag<material> material, tag<geometry> geometry) const
	noexcept
{
	psl::array<std::pair<size_t, std::uintptr_t>> result{};
	if(auto matIt = m_Bindings.find(material); matIt != std::end(m_Bindings))
	{
		if(auto geomIt = m_InstanceData.find(geometry); geomIt != std::end(m_InstanceData))
		{
			size_t count = {0};
			for(const auto& binding : matIt->second)
			{
				auto it = std::find_if(
					std::begin(geomIt->second.description), std::end(geomIt->second.description),
					[& bDescr = binding.description](const binding::header& descr) { return descr == bDescr; });

				auto index	= std::distance(std::begin(geomIt->second.description), it);
				auto& segment = *std::next(std::begin(geomIt->second.data));

				result.emplace_back(binding.slot, segment.range().begin);
			}
		}
	}
	return result;
}