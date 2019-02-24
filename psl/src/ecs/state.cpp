#include "ecs/state.h"
#include "task.h"
#include <numeric>
#include "unique_ptr.h"
using namespace psl::ecs;

using psl::ecs::details::component_key_t;
using psl::ecs::details::entity_info;


state::state(size_t workers)
	: m_Scheduler(new psl::async::scheduler((workers == 0) ? std::nullopt : std::optional{workers}))
{}

std::vector<std::vector<details::dependency_pack>> slice(std::vector<details::dependency_pack>& source, size_t count)
{
	std::vector<std::vector<details::dependency_pack>> res;

	bool can_split = std::any_of(std::begin(source), std::end(source), [count](const auto& dep_pack) {
		return dep_pack.allow_partial() && dep_pack.entities() > count;
	});
	count		   = (can_split) ? count : 1;
	res.resize(count);
	for(auto& dep_pack : source)
	{
		if(dep_pack.allow_partial() && dep_pack.entities() > count)
		{
			auto batch_size = dep_pack.entities() / count;
			size_t processed{0};
			for(auto i = 0; i < count - 1; ++i)
			{
				res[i].emplace_back(dep_pack.slice(processed, processed + batch_size));
				processed += batch_size;
			}
			res[res.size() - 1].emplace_back(dep_pack.slice(processed, dep_pack.entities()));
		}
		else
		{
			for(auto i = 0; i < count; ++i) res[i].emplace_back(dep_pack);
		}
	}
	return res;
}

std::vector<psl::unique_ptr<info>> info_buffer;
void state::prepare_system(std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
						   std::uintptr_t cache_offset, details::system_information& information)
{
	std::function<void(state&, std::vector<details::dependency_pack>)> write_data =
		[](state& state, std::vector<details::dependency_pack> dep_packs) {
			for(const auto& dep_pack : dep_packs)
			{
				for(auto& binding : dep_pack.m_RWBindings)
				{
					const size_t size   = dep_pack.m_Sizes.at(binding.first);
					std::uintptr_t data = (std::uintptr_t)binding.second.data();
					state.set(dep_pack.m_Entities, binding.first, (void*)data);
				}
			}
		};


	auto pack = information.create_pack();
	bool has_partial =
		std::any_of(std::begin(pack), std::end(pack), [](const auto& dep_pack) { return dep_pack.allow_partial(); });

	if(has_partial && information.threading() == threading::par)
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

		auto multi_pack = slice(pack, m_Scheduler->workers());

		std::vector<std::future<void>> future_commands;

		auto index = info_buffer.size();
		for(size_t i = 0; i < m_Scheduler->workers(); ++i) info_buffer.emplace_back(new info(*this, dTime, rTime));

		auto infoBuffer = std::next(std::begin(info_buffer), index);

		for(auto& mPack : multi_pack)
		{

			auto t1 = m_Scheduler->schedule(information.system(), **infoBuffer, mPack);

			auto t2 = m_Scheduler->schedule(write_data, *this, mPack);

			m_Scheduler->dependency(t2.first, t1.first);

			future_commands.emplace_back(std::move(t1.second));
			infoBuffer = std::next(infoBuffer);
		}

		m_Scheduler->execute().wait();

		for(auto& fCommands : future_commands)
		{
			if(!fCommands.valid()) fCommands.wait();

			// commands.emplace_back(fCommands.get());
		}
	}
	else
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
		info_buffer.emplace_back(new info(*this, dTime, rTime));
		information.operator()(*info_buffer[info_buffer.size() - 1], pack);

		write_data(*this, pack);
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

	if(m_LockOrphans > 0)
	{
		m_Entities[m_LockHead] = m_Next;
		m_Next = m_LockNext;
		m_Orphans += m_LockOrphans;
		m_LockOrphans = 0;
	}

	for(auto& info : info_buffer)
	{
		execute_command_buffer(*info);
	}
	info_buffer.clear();

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


	return (it != std::end(m_Components)) ? it->operator->() : nullptr;
}

details::component_info* state::get_component_info(details::component_key_t key) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });

	return (it != std::end(m_Components)) ? it->operator->() : nullptr;
}


psl::array<const details::component_info*>
state::get_component_info(psl::array_view<details::component_key_t> keys) const noexcept
{
	psl::array<const details::component_info*> res{};
	size_t count = keys.size();
	for(const auto& cInfo : m_Components)
	{
		if(auto it = std::find(std::begin(keys), std::end(keys), cInfo->id()); it != std::end(keys))
		{
			res.push_back(cInfo.operator->());
			--count;
		}
	}
	return (count == 0) ? res : psl::array<const details::component_info*>{};
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
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size, void* prototype)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();
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
	assert(cInfo != nullptr);

	cInfo->add(entities);
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   void* prototype)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();

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

	if(m_LockOrphans == 0) m_LockHead = entities[0].first;

	for(auto range : entities)
	{
		for(auto e = range.first; e < range.second; ++e)
		{
			m_Entities[e] = m_LockNext;
			m_LockNext	= e;
		}
		m_LockOrphans += range.second - range.first;
	}
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void state::destroy(psl::array_view<entity> entities) noexcept
{
	if(entities.size() == 0) return;

	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entities);
	}
	if(m_LockOrphans == 0) m_LockHead = entities[0];
	m_LockOrphans += entities.size();
	for(auto e : entities)
	{
		m_Entities[e] = m_LockNext;
		m_LockNext	= e;
	}
}

void state::destroy(entity entity) noexcept
{
	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entity);
	}
	if(m_LockOrphans == 0) m_LockHead = entity;

	m_LockOrphans += 1;
	m_Entities[entity] = m_LockNext;
	m_LockNext		   = entity;
}


void state::fill_in(details::component_key_t key, psl::array_view<entity> entities,
					psl::array_view<std::uintptr_t>& data)
{}

psl::array<entity>::iterator state::filter_remove(details::component_key_t key, psl::array<entity>::iterator& begin,
												  psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin
							  : std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_component(e); });
}

psl::array<entity>::iterator state::filter_remove_on_add(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin : std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_added(e); });
}
psl::array<entity>::iterator state::filter_remove_on_remove(details::component_key_t key,
															psl::array<entity>::iterator& begin,
															psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin
							  : std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_removed(e); });
}
psl::array<entity>::iterator state::filter_remove_except(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin
							  : std::remove_if(begin, end, [cInfo](entity e) { return cInfo->has_component(e); });
}
psl::array<entity>::iterator state::filter_remove_on_break(psl::array<details::component_key_t> keys,
														   psl::array<entity>::iterator& begin,
														   psl::array<entity>::iterator& end) const noexcept
{
	auto cInfos = get_component_info(keys);

	return (cInfos.size() == 0) ? begin : std::remove_if(begin, end, [cInfos](entity e) {
		return !std::any_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_removed(e); }) ||
			   !std::all_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_component(e) || cInfo->has_removed(e); });
	});
}

psl::array<entity>::iterator state::filter_remove_on_combine(psl::array<details::component_key_t> keys,
															 psl::array<entity>::iterator& begin,
															 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfos = get_component_info(keys);

	return (cInfos.size() == 0) ? begin : std::remove_if(begin, end, [cInfos](entity e) {
		return !std::any_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_added(e); }) ||
			   !std::all_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_component(e); });
	});
}

psl::array<entity> state::filter_seed(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? psl::array<entity>{} : psl::array<entity>{cInfo->entities()};
}

psl::array<entity> state::filter_seed_on_add(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? psl::array<entity>{} : psl::array<entity>{cInfo->added_entities()};
}

psl::array<entity> state::filter_seed_on_remove(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? psl::array<entity>{} : psl::array<entity>{cInfo->removed_entities()};
}

psl::array<entity> state::filter_seed_on_break(psl::array<details::component_key_t> keys) const noexcept
{
	auto cInfos = get_component_info(keys);
	if(cInfos.size() == 0) return psl::array<entity>{};
	psl::array<entity> storage{cInfos[0]->entities()};
	auto begin = std::begin(storage);
	auto end   = std::end(storage);
	end		   = std::remove_if(begin, end, [cInfos](entity e) {
		   return !std::any_of(std::begin(cInfos), std::end(cInfos),
							   [e](const details::component_info* cInfo) { return cInfo->has_removed(e); }) ||
				  !std::all_of(std::begin(cInfos), std::end(cInfos),
							   [e](const details::component_info* cInfo) { return cInfo->has_component(e); });
	});
	storage.erase(end, std::end(storage));
	return storage;
}

psl::array<entity> state::filter_seed_on_combine(psl::array<details::component_key_t> keys) const noexcept
{
	auto cInfos = get_component_info(keys);
	if(cInfos.size() == 0) return psl::array<entity>{};
	psl::array<entity> storage{cInfos[0]->entities()};
	auto begin = std::begin(storage);
	auto end   = std::end(storage);
	end		   = std::remove_if(begin, end, [cInfos](entity e) {
		   return !std::any_of(std::begin(cInfos), std::end(cInfos),
							   [e](const details::component_info* cInfo) { return cInfo->has_added(e); }) ||
				  !std::all_of(std::begin(cInfos), std::end(cInfos),
							   [e](const details::component_info* cInfo) { return cInfo->has_component(e); });
	});
	storage.erase(end, std::end(storage));
	return storage;
}

bool state::filter_seed_best(psl::array_view<details::component_key_t> filters,
							 psl::array_view<details::component_key_t> added,
							 psl::array_view<details::component_key_t> removed, psl::array_view<entity>& out,
							 details::component_key_t& selected) const noexcept
{
	size_t count{std::numeric_limits<size_t>::max()};

	auto selection = [this, &count, &selected, &out](details::component_key_t key) {
		const auto& cInfo = get_component_info(key);
		if(cInfo == nullptr)
		{
			count = 0;
			return;
		}
		if(cInfo->entities().size() < count)
		{
			out		 = cInfo->entities();
			count	= out.size();
			selected = cInfo->id();
		}
	};

	auto selection_add = [this, &count, &selected, &out](details::component_key_t key) {
		const auto& cInfo = get_component_info(key);
		if(cInfo == nullptr)
		{
			count = 0;
			return;
		}
		if(cInfo->added_entities().size() < count)
		{
			out		 = cInfo->added_entities();
			count	= out.size();
			selected = cInfo->id();
		}
	};

	auto selection_remove = [this, &count, &selected, &out](details::component_key_t key) {
		const auto& cInfo = get_component_info(key);
		if(cInfo == nullptr)
		{
			count = 0;
			return;
		}
		if(cInfo->removed_entities().size() < count)
		{
			out		 = cInfo->removed_entities();
			count	= out.size();
			selected = cInfo->id();
		}
	};
	std::for_each(std::begin(filters), std::end(filters), selection);
	std::for_each(std::begin(added), std::end(added), selection_add);
	std::for_each(std::begin(removed), std::end(removed), selection_remove);

	return count != std::numeric_limits<size_t>::max() && count != 0;
}
bool state::filter_seed_best(const details::dependency_pack& pack, psl::array_view<entity>& out,
							 details::component_key_t& selected) const noexcept
{
	psl::array<details::component_key_t> added{pack.on_add};
	added.insert(std::end(added), std::begin(pack.on_combine), std::end(pack.on_combine));
	psl::array<details::component_key_t> removed{pack.on_remove};
	removed.insert(std::end(removed), std::begin(pack.on_break), std::end(pack.on_break));
	return filter_seed_best(pack.filters, added, removed, out, selected);
}

psl::array<entity> state::filter(details::dependency_pack& pack)
{
	psl::array_view<entity> best_pack{};
	details::component_key_t best_key;
	if(!filter_seed_best(pack, best_pack, best_key)) return psl::array<entity>{};
	psl::array<entity> result{best_pack};
	auto begin = std::begin(result);
	auto end   = std::end(result);
	std::unordered_set<details::component_key_t> processed{best_key};
	for(auto filter : pack.on_remove)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove_on_remove(filter, begin, end);
	}
	if(pack.on_break.size() > 0)
	{
		for(auto filter : pack.on_break)
			processed.insert(filter);
		end = filter_remove_on_break(pack.on_break, begin, end);
	}
	for(auto filter : pack.on_add)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove_on_add(filter, begin, end);
	}
	if(pack.on_combine.size() > 0) end = filter_remove_on_combine(pack.on_combine, begin, end);
	for(auto filter : pack.filters)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove(filter, begin, end);
	}
	for(auto filter : pack.except)
	{
		end = filter_remove_except(filter, begin, end);
	}
	result.erase(end, std::end(result));
	return result;
}

size_t state::prepare_data(psl::array_view<entity> entities, void* cache, component_key_t id) const noexcept
{
	const auto& cInfo = get_component_info(id);
	assert_debug_break(cInfo != nullptr);
	return cInfo->copy_to(entities, cache);
}

size_t state::prepare_bindings(psl::array_view<entity> entities, void* cache, details::dependency_pack& dep_pack) const
	noexcept
{
	size_t offset_start = (std::uintptr_t)cache;
	for(auto& binding : dep_pack.m_RBindings)
	{
		std::uintptr_t data_begin = (std::uintptr_t)cache;
		auto write_size			  = prepare_data(entities, cache, binding.first);
		cache					  = (void*)((std::uintptr_t)cache + write_size);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
	}
	for(auto& binding : dep_pack.m_RWBindings)
	{
		std::uintptr_t data_begin = (std::uintptr_t)cache;
		auto write_size			  = prepare_data(entities, cache, binding.first);
		cache					  = (void*)((std::uintptr_t)cache + write_size);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
	}
	return (std::uintptr_t)cache - offset_start;
}

void state::set(psl::array_view<entity> entities, details::component_key_t key, void* data) noexcept
{
	if(entities.size() == 0) return;
	const auto& cInfo = get_component_info(key);
	assert(cInfo != nullptr);
	cInfo->copy_from(entities, data);
}


void state::execute_command_buffer(info& info)
{
	auto& buffer = info.command_buffer;

	psl::sparse_array<entity> remapped_entities;
	if(buffer.m_Entities.size() > 0)
	{
		psl::array<entity> added_entities;
		std::set_difference(std::begin(buffer.m_Entities), std::end(m_Entities), std::begin(buffer.m_DestroyedEntities),
							std::end(buffer.m_DestroyedEntities), std::back_inserter(added_entities));


		for(auto e : added_entities)
		{
			remapped_entities[e] = create();
		}
	}
	for(auto& component_src : buffer.m_Components)
	{
		if(!component_src->changes()) continue;
		auto component_dst = get_component_info(component_src->id());
		if(component_dst == nullptr)
		{
			m_Components.emplace_back(component_src->create_storage(remapped_entities));
		}
		else
		{
			component_src->merge_into(component_dst, remapped_entities);
		}
	}
	for(auto e : buffer.m_DestroyedEntities)
	{
		if(e < buffer.m_First) destroy(e);
	}
}