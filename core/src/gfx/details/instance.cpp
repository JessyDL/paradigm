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

constexpr instancing_size_type default_capacity = 32;

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

			for(auto& [uid, data] : m_GeometryInstanceData) {
				auto res = m_VertexInstanceBuffer->reserve(data.capacity() * d.description.size_of_element);
				if(!res) {
					core::gfx::log->error("could not allocate");
					continue;
				}
				data.instance_data.emplace_back(geometry_instance_data::entry {res.value(), d.description, d.slot});
			}
		} else {
			if(it->first.size_of_element != d.description.size_of_element)
				core::gfx::log->error(
				  "clash in material binding slots, names are unique and should be all the same size");
			it->second += 1;
		}
	}
}

std::vector<instancing_size_type> data::add(core::resource::tag<core::gfx::geometry_t> uid,
											instancing_size_type count) {
	auto it = m_GeometryInstanceData.find(uid);
	if(it == std::end(m_GeometryInstanceData)) {
		auto const size = std::max(count, default_capacity);
		it				= m_GeometryInstanceData.emplace(uid, geometry_instance_data {size}).first;
		it->second.capacity(size);
		for(const auto& b : m_UniqueBindings) {
			auto res = m_VertexInstanceBuffer->reserve(it->second.capacity() * b.first.size_of_element);
			if(!res) {
				core::gfx::log->error("could not allocate");
				continue;
			}
			it->second.instance_data.emplace_back(geometry_instance_data::entry {res.value(), b.first, b.second});
		}
	}

	if(it->second.available() < count) {
		auto const size = it->second.capacity();
		auto new_size	= std::max(size + count, size + (size >> 1));

		for(auto& entry : it->second.instance_data) {
			auto res = m_VertexInstanceBuffer->reserve(new_size * entry.description.size_of_element);
			if(!res) {
				core::gfx::log->error("could not allocate");
				continue;
			}
			auto& value = res.value();
			m_VertexInstanceBuffer->copy_from(
			  m_VertexInstanceBuffer.value(),
			  {core::gfx::memory_copy {entry.memory.range().begin, value.range().begin, entry.memory.range().size()}});
			std::swap(entry.memory, value);
			m_VertexInstanceBuffer->deallocate(value);
		}

		it->second.capacity(new_size);
	}

	return it->second.add(count);
}


bool data::remove(core::resource::handle<material_t> material) noexcept {
	auto it = m_MaterialInstanceData.find(material);
	if(it == std::end(m_MaterialInstanceData))
		return false;

	auto segment = it->second.segment;
	m_MaterialInstanceData.erase(it);
	return m_MaterialInstanceBuffer->region.deallocate(it->second.segment);
}


instancing_size_type data::count(core::resource::tag<core::gfx::geometry_t> uid) const noexcept {
	if(auto it = m_GeometryInstanceData.find(uid.uid()); it != std::end(m_GeometryInstanceData)) {
		return it->second.size();
	}
	return 0;
}


psl::array<std::pair<size_t, std::uintptr_t>> data::bindings(tag<material_t> material,
															 tag<geometry_t> geometry) const noexcept {
	psl::array<std::pair<size_t, std::uintptr_t>> result {};
	if(auto matIt = m_Bindings.find(material); matIt != std::end(m_Bindings)) {
		if(auto geomIt = m_GeometryInstanceData.find(geometry); geomIt != std::end(m_GeometryInstanceData)) {
			size_t count = {0};
			for(const auto& binding : matIt->second) {
				auto it = std::find_if(std::begin(geomIt->second.instance_data),
									   std::end(geomIt->second.instance_data),
									   [&bDescr = binding.description](const geometry_instance_data::entry& entry) {
										   return entry.description == bDescr;
									   });

				result.emplace_back(binding.slot, it->memory.range().begin);
			}
		}
	}
	return result;
}


bool data::has_element(tag<geometry_t> geometry, psl::string_view name) const noexcept {
	if(auto it = m_GeometryInstanceData.find(geometry); it != std::end(m_GeometryInstanceData)) {
		return std::find_if(std::begin(it->second.instance_data),
							std::end(it->second.instance_data),
							[&name](const geometry_instance_data::entry& entry) {
								return entry.description.name == name;
							}) != std::end(it->second.instance_data);
	}
	return false;
}

std::optional<std::pair<memory::segment, uint32_t>> data::segment(tag<geometry_t> geometry,
																  psl::string_view name) const noexcept {
	if(auto it = m_GeometryInstanceData.find(geometry); it != std::end(m_GeometryInstanceData)) {
		auto descrIt =
		  std::find_if(std::begin(it->second.instance_data),
					   std::end(it->second.instance_data),
					   [&name](const geometry_instance_data::entry& entry) { return entry.description.name == name; });
		if(descrIt != std::end(it->second.instance_data)) {
			return std::pair {descrIt->memory, descrIt->description.size_of_element};
		}
	}
	return std::nullopt;
}


bool data::erase(core::resource::tag<core::gfx::geometry_t> geometry,
				 std::span<instancing_size_type const> ids) noexcept {
	if(auto it = m_GeometryInstanceData.find(geometry); it != std::end(m_GeometryInstanceData)) {
		it->second.erase(ids.begin(), ids.end());
		return true;
	}
	return false;
}

bool data::erase(core::resource::tag<core::gfx::geometry_t> geometry, instancing_size_type id) noexcept {
	if(auto it = m_GeometryInstanceData.find(geometry); it != std::end(m_GeometryInstanceData)) {
		it->second.erase(id);
		return true;
	}
	return false;
}
bool data::clear(core::resource::tag<core::gfx::geometry_t> geometry) noexcept {
	if(auto it = m_GeometryInstanceData.find(geometry); it != std::end(m_GeometryInstanceData)) {
		for(auto& data : it->second.instance_data) {
			m_VertexInstanceBuffer->deallocate(data.memory);
		}

		m_GeometryInstanceData.erase(it);
		return true;
	}
	return false;
}
bool data::clear() noexcept {
	for(auto& [uid, geom_data] : m_GeometryInstanceData) {
		for(auto& data : geom_data.instance_data) {
			m_VertexInstanceBuffer->deallocate(data.memory);
		}
	}
	m_GeometryInstanceData.clear();
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

instancing_size_type data::index_of(core::resource::tag<core::gfx::geometry_t> geometry,
									instancing_size_type id) const noexcept {
	if(auto it = m_GeometryInstanceData.find(geometry); it != std::end(m_GeometryInstanceData)) {
		return it->second.index_of(id);
	}
	return std::numeric_limits<instancing_size_type>::max();
}

bool data::set(core::resource::tag<core::gfx::material_t> material,
			   const void* data,
			   size_t size,
			   size_t offset) noexcept {
	auto it = m_MaterialInstanceData.find(material);
	if(it == std::end(m_MaterialInstanceData)) {
		return false;
	}

	return m_MaterialInstanceBuffer->buffer->commit(
	  {core::gfx::commit_instruction {(void*)data, size, it->second.segment, memory::range_t {offset, offset + size}}});
}

bool data::bind_material(core::resource::handle<core::gfx::material_t> material) {
	auto it = m_MaterialInstanceData.find(material);
	if(it == std::end(m_MaterialInstanceData)) {
		return false;
	}

	return material->bind_instance_data(it->second.descriptor.binding(),
										psl::narrow_cast<uint32_t>(it->second.segment.range().begin));
}

core::resource::handle<core::gfx::buffer_t> data::material_buffer() const noexcept {
	return m_MaterialInstanceBuffer->buffer;
}
bool data::has_data(core::resource::handle<core::gfx::material_t> material) const noexcept {
	return m_MaterialInstanceData.find(material) != std::end(m_MaterialInstanceData);
}

void data::apply() {
	for(auto& [uid, geom_data] : m_GeometryInstanceData) {
		auto commands = geom_data.consume();
		if(!commands.empty()) {
			m_VertexInstanceBuffer->copy_from(m_VertexInstanceBuffer.value(), commands);
		}
	}
}


instancing_size_type geometry_instance_data::available() const noexcept {
	return m_Max - (m_Head - psl::narrow_cast<uint32_t>(m_Orphans.size()));
}

instancing_size_type geometry_instance_data::capacity() const noexcept {
	return m_Max;
}

void geometry_instance_data::capacity(instancing_size_type max) {
	psl_assert(max > manager.size(),
			   "Setting a max capacity lower than the current allocated entries will end up in errors.");
	m_Max = max;
}

std::vector<instancing_size_type> geometry_instance_data::add(instancing_size_type count) {
	if(count == 0) {
		return {};
	}

	psl_assert(count <= m_Max,
			   "cannot allocate more instances than the maximum allowed. Requested {}, but only have {} left out of {}",
			   count,
			   available(),
			   m_Max);
	if(count > available()) {
		core::gfx::log->error("cannot allocate {} instances, maximum is {} and we have {} active right now",
							  count,
							  m_Max,
							  m_Head - m_Orphans.size());
		return {};
	}

	std::vector<instancing_size_type> ids {};
	ids.resize(count);
	auto orphans_to_use = std::min(static_cast<instancing_size_type>(m_Orphans.size()), count);
	if(orphans_to_use > 0) {
		auto it = std::end(m_Orphans);
		for(instancing_size_type i = 0; i != orphans_to_use; ++i) {
			it	   = std::prev(it);
			ids[i] = *it;
		}
		m_Orphans.erase(it, std::end(m_Orphans));
	}

	auto remainder = count - orphans_to_use;
	if(remainder > 0) {
		std::iota(ids.begin() + orphans_to_use, ids.end(), m_Head);
		m_Head += remainder;
	}

	manager.insert(std::begin(ids), std::end(ids));
	return ids;
};

instancing_size_type geometry_instance_data::size() const noexcept {
	return manager.size();
}

void geometry_instance_data::erase(instancing_size_type id) {
	psl_assert(manager.contains(id), "Trying to erase an id that does not exist");
	manager.erase(id);
	m_Orphans.push_back(id);
}

void geometry_instance_data::erase(auto&& first, auto&& last) {
	manager.erase(first, last);
	m_Orphans.insert(m_Orphans.end(), first, last);
}

instancing_size_type geometry_instance_data::index_of(instancing_size_type id) const noexcept {
	if(!manager.contains(id)) {
		return std::numeric_limits<instancing_size_type>::max();
	}
	return manager.index_of(id);
}

void geometry_instance_data::clear() noexcept {
	manager.clear();
	m_Orphans.clear();
	m_Head = 0;
}

psl::array<core::gfx::memory_copy> geometry_instance_data::consume() {
	auto commands = std::move(m_Link->m_Commands);
	m_Link->m_Commands.clear();
	if(commands.empty()) {
		return {};
	}

	psl::array<core::gfx::memory_copy> instructions {};
	for(auto const& data : instance_data) {
		for(auto& command : commands) {
			// we can safely ignore resize commands, as they are implicit in the copy commands.
			if(std::holds_alternative<storage_link::swap_command>(command)) {
				auto& swap = std::get<storage_link::swap_command>(command);
				instructions.push_back({data.memory.range().begin + (swap.src * data.description.size_of_element),
										data.memory.range().begin + (swap.dst * data.description.size_of_element),
										swap.count * data.description.size_of_element});
			}
		}
	}
	return instructions;
}
