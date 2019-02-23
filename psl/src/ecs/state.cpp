#include "ecs/state.h"
#include "task.h"
#include <numeric>

using namespace psl::ecs;

using psl::ecs::details::component_key_t;
using psl::ecs::details::entity_info;


state::state(size_t workers)
	: m_Scheduler(new psl::async::scheduler((workers == 0) ? std::nullopt : std::optional{workers}))
{}


void state::prepare_system(std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
						   std::uintptr_t cache_offset, details::system_information& information)
{
	auto write_data = [](state& state, std::vector<details::dependency_pack> dep_packs) {
		for(const auto& dep_pack : dep_packs)
		{
			for(auto& binding : dep_pack.m_RWBindings)
			{
				const size_t size   = dep_pack.m_Sizes.at(binding.first);
				std::uintptr_t data = (std::uintptr_t)binding.second.data();
				state.set(dep_pack.m_Entities, binding.first,(void*)data);
			}
		}
	};

	info info{*this, dTime, rTime};

	auto pack = information.create_pack();
	bool has_partial =
		std::any_of(std::begin(pack), std::end(pack), [](const auto& dep_pack) { return dep_pack.allow_partial(); });

	std::vector<command_buffer> commands;
	// if(has_partial && system.threading == threading::par)
	//{
	//	for(auto& dep_pack : pack)
	//	{
	//		core::profiler.scope_begin("filtering");
	//		auto entities = filter(dep_pack);
	//		core::profiler.scope_end();
	//		if(entities.size() == 0) continue;
	//		core::profiler.scope_begin("copy");
	//		std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
	//		dep_pack.m_Entities = psl::array_view<core::ecs::entity>(
	//			(entity*)cache_offset, (entity*)(cache_offset + sizeof(entity) * entities.size()));

	//		cache_offset += sizeof(entity) * entities.size();

	//		cache_offset += prepare_bindings(entities, cache, cache_offset, dep_pack);
	//		core::profiler.scope_end();
	//	}

	//	auto multi_pack = slice(pack, m_Scheduler->workers());

	//	std::vector<std::future<command_buffer>> future_commands;

	//	for(auto& mPack : multi_pack)
	//	{
	//		auto t1 = m_Scheduler->schedule(system.invocable, *this, dTime, rTime, mPack);

	//		auto t2 = m_Scheduler->schedule(write_data, *this, mPack);

	//		m_Scheduler->dependency(t2.first, t1.first);

	//		future_commands.emplace_back(std::move(t1.second));
	//	}

	//	m_Scheduler->execute().wait();
	//	for(auto& fCommands : future_commands)
	//	{
	//		if(!fCommands.valid()) fCommands.wait();
	//		auto cmd = fCommands.get();
	//		execute_command_buffer(cmd);
	//		// commands.emplace_back(fCommands.get());
	//	}
	//}
	// else
	{
		for(auto& dep_pack : pack)
		{
			auto entities = filter(dep_pack);
			if(entities.size() == 0) continue;
			std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
			dep_pack.m_Entities = psl::array_view<entity>((entity*)cache_offset,
														  (entity*)(cache_offset + sizeof(entity) * entities.size()));

			cache_offset += sizeof(entity) * entities.size();

			cache_offset += prepare_bindings(entities, (void*)cache_offset, dep_pack);
		}

		information.operator()(info, pack);

		write_data(*this, pack);
		//execute_command_buffer(info.cmd);
	}
}
void state::tick(std::chrono::duration<float> dTime)
{
	for(auto& cInfo : m_Components) cInfo->lock();
	// tick systems;
	for(auto& system : m_SystemInformations)
	{
		prepare_system(dTime, dTime, (std::uintptr_t)m_Cache.data(), system);
	}

	// purge;
	for(auto& cInfo : m_Components) cInfo->unlock_and_purge();

	++m_Tick;
}

psl::array_view<entity> state::entities_for(details::component_key_t key) const noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	return (it != std::end(m_Components)) ? (*it)->entities() : psl::array_view<entity>{};
}

psl::array_view<entity> state::entities_added_for(details::component_key_t key) const noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	return (it != std::end(m_Components)) ? (*it)->added_entities() : psl::array_view<entity>{};
}

psl::array_view<entity> state::entities_removed_for(details::component_key_t key) const noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	return (it != std::end(m_Components)) ? (*it)->removed_entities() : psl::array_view<entity>{};
}

const details::component_info* state::get_component_info(details::component_key_t key) const noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });

	assert(it != std::end(m_Components));
	return it->operator->();
}

details::component_info* state::get_component_info(details::component_key_t key) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });

	assert(it != std::end(m_Components));
	return it->operator->();
}


psl::array<const details::component_info*>
state::get_component_info(psl::array_view<details::component_key_t> keys) const noexcept
{
	psl::array<const details::component_info*> res{};
	for(const auto& cInfo : m_Components)
	{
		if(auto it = std::find(std::begin(keys), std::end(keys), cInfo->id()); it != std::end(keys))
		{
			res.push_back(cInfo.operator->());
		}
	}
	return res;
}
// empty construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size)
{
	auto cInfo = get_component_info(key);

	cInfo->add(entities);
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size, std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);

	cInfo->add(entities);
	for(const auto& id_range : entities)
	{
		auto location = (std::uintptr_t)cInfo->data() + (id_range.first * size);
		std::invoke(invocable, location, id_range.second - id_range.first);
	}
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size, void* prototype)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);

	auto offset = cInfo->entities().size();
	cInfo->add(entities);
	for(const auto& id_range : entities)
	{
		for(auto i = id_range.first; i < id_range.second; ++i)
		{
			std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
		}
	}
}


// empty construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size)
{
	auto cInfo = get_component_info(key);

	cInfo->add(entities);
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);

	cInfo->add(entities);
	for(auto e : entities)
	{
		auto location = (std::uintptr_t)cInfo->data() + (e * size);
		std::invoke(invocable, location, 1);
	}
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   void* prototype)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);

	auto offset = cInfo->entities().size();

	cInfo->add(entities);
	for(auto e : entities)
	{
		std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
	}
}


void state::remove_component(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	assert(it != std::end(m_Components));
	(*it)->destroy(entities);
}


void state::remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	assert(it != std::end(m_Components));


	(*it)->destroy(entities);
}


void state::destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	auto count = std::accumulate(std::begin(entities), std::end(entities), entity{0},
								 [](entity sum, const auto& range) { return sum + (range.second - range.first); });
	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entities);
	}
	m_Orphans += count;

	for(auto range : entities)
	{
		for(auto e = range.first; e < range.second; ++e)
		{
			m_Entities[e] = m_Next;
			m_Next		  = e;
		}
	}
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void state::destroy(psl::array_view<entity> entities) noexcept
{
	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entities);
	}

	m_Orphans += entities.size();
	for(auto e : entities)
	{
		m_Entities[e] = m_Next;
		m_Next		  = e;
	}
}

void state::destroy(entity entity) noexcept
{
	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entity);
	}
	m_Entities[entity] = m_Next;
	m_Next			   = entity;

	++m_Orphans;
}


void state::fill_in(details::component_key_t key, psl::array_view<entity> entities,
					psl::array_view<std::uintptr_t>& data)
{}

psl::array<entity>::iterator state::filter_remove(details::component_key_t key, psl::array<entity>::iterator& begin,
												  psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_component(e); });
}

psl::array<entity>::iterator state::filter_remove_on_add(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_added(e); });
}
psl::array<entity>::iterator state::filter_remove_on_remove(details::component_key_t key,
															psl::array<entity>::iterator& begin,
															psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_removed(e); });
}
psl::array<entity>::iterator state::filter_remove_except(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return std::remove_if(begin, end, [cInfo](entity e) { return cInfo->has_component(e); });
}
psl::array<entity>::iterator state::filter_remove_on_break(psl::array<details::component_key_t> keys,
														   psl::array<entity>::iterator& begin,
														   psl::array<entity>::iterator& end) const noexcept
{
	return end;
}

psl::array<entity>::iterator state::filter_remove_on_combine(psl::array<details::component_key_t> keys,
															 psl::array<entity>::iterator& begin,
															 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfos = get_component_info(keys);

	return std::remove_if(begin, end, [cInfos](entity e) {
		return !std::any_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_added(e); }) ||
			   !std::all_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_component(e); });
	});
}

psl::array<entity> state::filter_seed(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return psl::array<entity>{cInfo->entities()};
}

psl::array<entity> state::filter_seed_on_add(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return psl::array<entity>{cInfo->added_entities()};
}

psl::array<entity> state::filter_seed_on_remove(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return psl::array<entity>{cInfo->removed_entities()};
}

psl::array<entity> state::filter_seed_on_break(psl::array<details::component_key_t> keys) const noexcept
{
	return {};
}

psl::array<entity> state::filter_seed_on_combine(psl::array<details::component_key_t> keys) const noexcept
{
	auto cInfos = get_component_info(keys);
	psl::array<entity> storage{cInfos[0]->entities()};
	auto begin = std::begin(storage);
	auto end = std::end(storage);
	end = std::remove_if(begin, end, [cInfos](entity e)
						 {
							 return !std::any_of(std::begin(cInfos), std::end(cInfos),
												 [e](const details::component_info* cInfo) { return cInfo->has_added(e); }) ||
								 !std::all_of(std::begin(cInfos), std::end(cInfos),
											  [e](const details::component_info* cInfo) { return cInfo->has_component(e); });
						 });
	storage.erase(end, std::end(storage));
	return storage;
}

psl::array<entity> state::filter(details::dependency_pack& pack)
{
	auto result {filter_seed(pack.filters[0])};
	auto begin = std::begin(result);
	auto end = std::end(result);
	for(auto filter : pack.on_remove)
	{
		end = filter_remove_on_remove(filter, begin, end);
	}
	end = filter_remove_on_break(pack.on_break, begin, end);
	for(auto filter : pack.filters)
	{
		end = filter_remove(filter, begin, end);
	}
	for(auto filter : pack.on_add)
	{
		end = filter_remove_on_add(filter, begin, end);
	}
	end = filter_remove_on_combine(pack.on_combine, begin, end);
	for(auto filter : pack.except)
	{
		end = filter_remove_except(filter, begin, end);
	}
	return result;
}

size_t state::prepare_data(psl::array_view<entity> entities, void* cache,
						   component_key_t id) const noexcept
{
	const auto& cInfo = get_component_info(id);
	return cInfo->copy_to(entities, cache);
}

size_t state::prepare_bindings(psl::array_view<entity> entities, void* cache,
							   details::dependency_pack& dep_pack) const noexcept
{
	size_t offset_start = (std::uintptr_t)cache;
	for(auto& binding : dep_pack.m_RBindings)
	{
		std::uintptr_t data_begin = (std::uintptr_t)cache;
		auto write_size = prepare_data(entities, cache, binding.first);
		cache = (void*)((std::uintptr_t)cache + write_size);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
	}
	for(auto& binding : dep_pack.m_RWBindings)
	{
		std::uintptr_t data_begin = (std::uintptr_t)cache;
		auto write_size = prepare_data(entities, cache, binding.first);
		cache = (void*)((std::uintptr_t)cache + write_size);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
	}
	return (std::uintptr_t)cache - offset_start;
}

void state::set(psl::array_view<entity> entities, details::component_key_t key, void* data) noexcept
{
	const auto& cInfo = get_component_info(key);
	cInfo->copy_from(entities, data);
}