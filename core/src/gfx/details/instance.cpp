#include "stdafx.h"
#include "gfx/details/instance.h"
#include "gfx/material.h"
#include "data/material.h"
#include "gfx/shader.h"
#include "meta/shader.h"
#include "gfx/buffer.h"
#include "gfx/types.h"
#include "resource/resource.hpp"

using namespace core::gfx;
using namespace core::gfx::details::instance;
using namespace core::resource;

constexpr uint32_t default_capacity = 32000;

data::data(core::resource::handle<core::gfx::buffer> buffer) noexcept : m_InstanceBuffer(buffer) {}
void data::add(core::resource::handle<material> material)
{
	if(m_Bindings.find(material) != std::end(m_Bindings)) return;

	auto& data = m_Bindings[material];

	for(const auto& stage : material->data().stages())
	{
		if(stage.shader_stage() != core::gfx::shader_stage::vertex) continue;

		core::meta::shader* meta = material.cache()->library().get<core::meta::shader>(stage.shader()).value_or(nullptr);

		data.reserve(meta->instance_bindings().size());
		for(const auto& vBinding : meta->instance_bindings())
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
				auto res = m_InstanceBuffer->reserve(obj.id_generator.capacity() *
													 d.description.size_of_element);
				if(!res) core::gfx::log->error("could not allocate");

				obj.data.emplace_back(res.value());
				obj.description.emplace_back(d.description);
			}
		}
		else
		{
			if(it->first.size_of_element != d.description.size_of_element)
				core::gfx::log->error(
					"clash in material binding slots, names are unique and should be all the same size");
			it->second += 1;
		}
	}
}

std::vector<std::pair<uint32_t, uint32_t>> data::add(core::resource::tag<core::gfx::geometry> uid, uint32_t count)
{
	auto it = m_InstanceData.find(uid.uid());
	if(it == std::end(m_InstanceData))
	{
		auto size{(count > default_capacity) ? count << 2 : default_capacity};
		it = m_InstanceData.emplace(uid, object{uid, size}).first;
		for(const auto& b : m_UniqueBindings)
		{
			auto res =
				m_InstanceBuffer->reserve(it->second.id_generator.capacity() * b.first.size_of_element);
			if(!res) core::gfx::log->error("could not allocate");
			it->second.data.emplace_back(res.value());
			it->second.description.emplace_back(b.first);
		}
	}

	if(it->second.id_generator.available() < count)
	{
		auto size	 = it->second.id_generator.size();
		auto new_size = (it->second.id_generator.available() + count) << 2;
		if(!it->second.id_generator.resize(new_size))
			core::gfx::log->error("could not increase the id_generator size");
		else
		{
			for(auto& d : it->second.data)
			{
				auto res = m_InstanceBuffer->reserve(it->second.id_generator.capacity() * d.range().size() / size);
				if(!res) core::gfx::log->error("could not allocate");
				m_InstanceBuffer->copy_from(m_InstanceBuffer.value(), {core::gfx::memory_copy{d.range().begin, res.value().range().begin, d.range().size()}});
				std::swap(d, res.value());
				m_InstanceBuffer->deallocate(res.value());
			}
		}
	}

	return it->second.id_generator.create_multi(count);
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
				auto& segment = *std::next(std::begin(geomIt->second.data), index);

				result.emplace_back(binding.slot, segment.range().begin);
			}
		}
	}
	return result;
}


bool data::has_element(tag<geometry> geometry, psl::string_view name) const noexcept
{
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData))
	{
		return std::find_if(std::begin(it->second.description), std::end(it->second.description),
							[&name](const auto& descr) { return descr.name == name; }) !=
			   std::end(it->second.description);
	}
	return false;
}

std::optional<std::pair<memory::segment, uint32_t>> data::segment(tag<geometry> geometry, psl::string_view name) const
	noexcept
{
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData))
	{
		auto descrIt = std::find_if(std::begin(it->second.description), std::end(it->second.description),
									[&name](const auto& descr) { return descr.name == name; });
		if(descrIt != std::end(it->second.description))
		{
			auto index = std::distance(std::begin(it->second.description), descrIt);

			return std::pair{*std::next(std::begin(it->second.data), index), descrIt->size_of_element};
		}
	}
	return std::nullopt;
}


bool data::erase(core::resource::tag<core::gfx::geometry> geometry, uint32_t id) noexcept
{
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData))
	{
		it->second.id_generator.destroy(id);

		if(it->second.id_generator.size() == 0)
		{
			for(auto& segment : it->second.data) m_InstanceBuffer->deallocate(segment);

			m_InstanceData.erase(it);
		}
		return true;
	}
	return false;
}
bool data::clear(core::resource::tag<core::gfx::geometry> geometry) noexcept
{
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData))
	{
		for(auto& segment : it->second.data) m_InstanceBuffer->deallocate(segment);

		m_InstanceData.erase(it);
		return true;
	}
	return false;
}
bool data::clear() noexcept
{
	for(auto& [uid, obj] : m_InstanceData)
	{
		for(auto& segment : obj.data) m_InstanceBuffer->deallocate(segment);
	}
	m_InstanceData.clear();
	return true;
}