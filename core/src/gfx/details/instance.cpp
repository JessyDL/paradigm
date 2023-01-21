#include "core/gfx/details/instance.hpp"
#include "core/data/buffer.hpp"
#include "core/data/material.hpp"
#include "core/gfx/buffer.hpp"
#include "core/gfx/limits.hpp"
#include "core/gfx/material.hpp"
#include "core/gfx/shader.hpp"
#include "core/gfx/types.hpp"
#include "core/meta/shader.hpp"
#include "core/resource/resource.hpp"

using namespace core::gfx;
using namespace core::gfx::details::instance;
using namespace core::resource;

constexpr uint32_t default_capacity = 32;

data::data(core::resource::handle<core::gfx::buffer_t> vertexBuffer,
		   core::resource::handle<core::gfx::shader_buffer_binding> materialBuffer) noexcept
	: m_VertexInstanceBuffer(vertexBuffer), m_MaterialInstanceBuffer(materialBuffer) {}

data::~data() {
	for(auto& it : m_MaterialInstanceData) {
		m_MaterialInstanceBuffer->region.deallocate(it.second.segment);
	}
}
void data::add(core::resource::handle<material_t> material) {
	if(m_Bindings.find(material) != std::end(m_Bindings))
		return;

	auto& data = m_Bindings[material];

	auto align_to = [](auto value, auto alignment) {
		auto remainder = value % alignment;
		return (remainder) ? value + (alignment - remainder) : value;
	};

	auto alignment_requirement = m_VertexInstanceBuffer->data().region().alignment();
	for(const auto& stage : material->data().stages()) {
		core::meta::shader* meta =
		  material.cache()->library().get<core::meta::shader>(stage.shader()).value_or(nullptr);

		if(!meta) {
			core::gfx::log->critical("could not find the metadata associated to shader {}", stage.shader());
			return;
		}

		for(const auto& descriptor : meta->descriptors()) {
			if(descriptor.name() == core::data::material_t::MATERIAL_DATA) {
				m_MaterialDataSizes.emplace_back(align_to(descriptor.size(), alignment_requirement));
				auto segment = m_MaterialInstanceBuffer->region.allocate(descriptor.size());
				if(!segment) {
					core::gfx::log->critical("could not allocate a segment for the material instance data {}",
											 stage.shader());
					return;
				}
				m_MaterialInstanceData.emplace(material.uid(),
											   material_instance_data {descriptor, descriptor.size(), segment.value()});
			}
		}

		if(stage.shader_stage() != core::gfx::shader_stage::vertex)
			continue;
		for(const auto& attribute : stage.attributes()) {
			if(attribute.input_rate().value_or(vertex_input_rate::vertex) != vertex_input_rate::instance)
				continue;

			auto shader_attribute = std::find_if(
			  std::begin(meta->inputs()),
			  std::end(meta->inputs()),
			  [location = attribute.location()](const auto& attribute) { return attribute.location() == location; });

			data.emplace_back(
			  binding {binding::header {psl::string {attribute.tag()}, static_cast<uint32_t>(shader_attribute->size())},
					   attribute.location()});
		}
	}

	auto accum = std::accumulate(std::begin(m_MaterialDataSizes),
								 std::end(m_MaterialDataSizes),
								 size_t {0},
								 [](auto sum, auto rhs) { return sum + rhs; });
	if(accum > 0) {
		m_MaterialDataSizes.emplace_back(accum);
	}
	for(const auto& d : data) {
		auto it = std::find_if(std::begin(m_UniqueBindings), std::end(m_UniqueBindings), [&d](const auto& pair) {
			return pair.first == d.description;
		});
		if(it == std::end(m_UniqueBindings)) {
			m_UniqueBindings.emplace_back(std::pair<binding::header, uint32_t> {d.description, 0});

			for(auto& [uid, obj] : m_InstanceData) {
				auto res = m_VertexInstanceBuffer->reserve(obj.id_generator.capacity() * d.description.size_of_element);
				if(!res)
					core::gfx::log->error("could not allocate");

				obj.data.emplace_back(res.value());
				obj.description.emplace_back(d.description);
			}
		} else {
			if(it->first.size_of_element != d.description.size_of_element)
				core::gfx::log->error(
				  "clash in material binding slots, names are unique and should be all the same size");
			it->second += 1;
		}
	}
}

std::vector<std::pair<uint32_t, uint32_t>> data::add(core::resource::tag<core::gfx::geometry_t> uid, uint32_t count) {
	auto it = m_InstanceData.find(uid);
	if(it == std::end(m_InstanceData)) {
		auto size {(count > default_capacity) ? count << 2 : default_capacity};
		it = m_InstanceData.emplace(uid, object {uid, size}).first;
		for(const auto& b : m_UniqueBindings) {
			auto res = m_VertexInstanceBuffer->reserve(it->second.id_generator.capacity() * b.first.size_of_element);
			if(!res)
				core::gfx::log->error("could not allocate");
			it->second.data.emplace_back(res.value());
			it->second.description.emplace_back(b.first);
		}
	}

	if(it->second.id_generator.available() < count) {
		auto size	  = it->second.id_generator.size();
		auto new_size = (it->second.id_generator.available() + count) << 2;
		if(!it->second.id_generator.resize(new_size))
			core::gfx::log->error("could not increase the id_generator size from {} to {}", size, new_size);
		else {
			for(auto& d : it->second.data) {
				auto res =
				  m_VertexInstanceBuffer->reserve(it->second.id_generator.capacity() * d.range().size() / size);
				if(!res)
					core::gfx::log->error("could not allocate");
				m_VertexInstanceBuffer->copy_from(
				  m_VertexInstanceBuffer.value(),
				  {core::gfx::memory_copy {d.range().begin, res.value().range().begin, d.range().size()}});
				std::swap(d, res.value());
				m_VertexInstanceBuffer->deallocate(res.value());
			}
		}
	}

	return it->second.id_generator.create_multi(count);
}


bool data::remove(core::resource::handle<material_t> material) noexcept {
	auto it = m_MaterialInstanceData.find(material);
	if(it == std::end(m_MaterialInstanceData))
		return false;

	auto segment = it->second.segment;
	m_MaterialInstanceData.erase(it);
	return m_MaterialInstanceBuffer->region.deallocate(it->second.segment);
}


uint32_t data::count(core::resource::tag<core::gfx::geometry_t> uid) const noexcept {
	if(auto it = m_InstanceData.find(uid.uid()); it != std::end(m_InstanceData)) {
		return it->second.id_generator.size();
	}
	return 0;
}


psl::array<std::pair<size_t, std::uintptr_t>> data::bindings(tag<material_t> material,
															 tag<geometry_t> geometry) const noexcept {
	psl::array<std::pair<size_t, std::uintptr_t>> result {};
	if(auto matIt = m_Bindings.find(material); matIt != std::end(m_Bindings)) {
		if(auto geomIt = m_InstanceData.find(geometry); geomIt != std::end(m_InstanceData)) {
			size_t count = {0};
			for(const auto& binding : matIt->second) {
				auto it = std::find_if(
				  std::begin(geomIt->second.description),
				  std::end(geomIt->second.description),
				  [&bDescr = binding.description](const binding::header& descr) { return descr == bDescr; });

				auto index	  = std::distance(std::begin(geomIt->second.description), it);
				auto& segment = *std::next(std::begin(geomIt->second.data), index);

				result.emplace_back(binding.slot, segment.range().begin);
			}
		}
	}
	return result;
}


bool data::has_element(tag<geometry_t> geometry, psl::string_view name) const noexcept {
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData)) {
		return std::find_if(std::begin(it->second.description),
							std::end(it->second.description),
							[&name](const auto& descr) { return descr.name == name; }) !=
			   std::end(it->second.description);
	}
	return false;
}

std::optional<std::pair<memory::segment, uint32_t>> data::segment(tag<geometry_t> geometry,
																  psl::string_view name) const noexcept {
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData)) {
		auto descrIt = std::find_if(std::begin(it->second.description),
									std::end(it->second.description),
									[&name](const auto& descr) { return descr.name == name; });
		if(descrIt != std::end(it->second.description)) {
			auto index = std::distance(std::begin(it->second.description), descrIt);

			return std::pair {*std::next(std::begin(it->second.data), index), descrIt->size_of_element};
		}
	}
	return std::nullopt;
}


bool data::erase(core::resource::tag<core::gfx::geometry_t> geometry, uint32_t id) noexcept {
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData)) {
		it->second.id_generator.destroy(id);

		if(it->second.id_generator.size() == 0) {
			for(auto& segment : it->second.data) m_VertexInstanceBuffer->deallocate(segment);

			m_InstanceData.erase(it);
		}
		return true;
	}
	return false;
}
bool data::clear(core::resource::tag<core::gfx::geometry_t> geometry) noexcept {
	if(auto it = m_InstanceData.find(geometry); it != std::end(m_InstanceData)) {
		for(auto& segment : it->second.data) m_VertexInstanceBuffer->deallocate(segment);

		m_InstanceData.erase(it);
		return true;
	}
	return false;
}
bool data::clear() noexcept {
	for(auto& [uid, obj] : m_InstanceData) {
		for(auto& segment : obj.data) m_VertexInstanceBuffer->deallocate(segment);
	}
	m_InstanceData.clear();
	return true;
}


size_t data::offset_of(core::resource::tag<core::gfx::material_t> material, psl::string_view name) const noexcept {
	auto it = m_MaterialInstanceData.find(material);
	if(it == std::end(m_MaterialInstanceData))
		return std::numeric_limits<size_t>::max();

	size_t res {};
	auto members = it->second.descriptor.members();
	size_t last_index {0};
	size_t index = name.find('.');
	do {
		res			   = std::numeric_limits<size_t>::max();
		auto substring = name.substr(last_index, name.find('.') - last_index);
		if(name[name.size() - 1] == ']')	// array
		{
			// auto start = name.rfind('[', last_index + substring.size());
			// auto index_of_array_str = name.substr(start, name.size() - 1 - start);
		} else {
			for(const auto& member : members) {
				if(member.name() == substring) {
					if(member.members().size() > 1)
						members = member.members();
					res = member.offset();
				}
			}
		}
		last_index = index;
		index	   = name.find('.', last_index + 1);
	} while(index != psl::string_view::npos);

	return res;
}

bool data::set(core::resource::tag<core::gfx::material_t> material,
			   const void* data,
			   size_t size,
			   size_t offset) noexcept {
	auto it = m_MaterialInstanceData.find(material);
	if(it == std::end(m_MaterialInstanceData))
		return false;

	return m_MaterialInstanceBuffer->buffer->commit(
	  {core::gfx::commit_instruction {(void*)data, size, it->second.segment, memory::range_t {offset, offset + size}}});
}

bool data::bind_material(core::resource::handle<core::gfx::material_t> material) {
	auto it = m_MaterialInstanceData.find(material);
	if(it == std::end(m_MaterialInstanceData))
		return false;

	return material->bind_instance_data(it->second.descriptor.binding(),
										static_cast<uint32_t>(it->second.segment.range().begin));
}

core::resource::handle<core::gfx::buffer_t> data::material_buffer() const noexcept {
	return m_MaterialInstanceBuffer->buffer;
}
bool data::has_data(core::resource::handle<core::gfx::material_t> material) const noexcept {
	return m_MaterialInstanceData.find(material) != std::end(m_MaterialInstanceData);
}
