
#include "psl/ecs/state.h"
#include <numeric>
#include "psl/unique_ptr.h"
//#include "psl/async/algorithm.h"
using namespace psl::ecs;

using psl::ecs::details::component_key_t;
using psl::ecs::details::entity_info;

constexpr size_t min_thread_entities = 1;


state::state(size_t workers, size_t cache_size) : m_Cache(cache_size), m_Scheduler(new psl::async::scheduler((workers == 0) ? std::nullopt : std::optional{ workers }))
{
	m_ModifiedEntities.reserve(65536);
}


psl::array<psl::array<details::dependency_pack>> slice(psl::array<details::dependency_pack>& source,
														 size_t workers = std::numeric_limits<size_t>::max())
{
	psl::array<psl::array<details::dependency_pack>> packs;

	if(source.size() == 0) return packs;

	auto [smallest_batch, largest_batch] =
		std::minmax_element(std::begin(source), std::end(source),
							[](const auto& lhs, const auto& rhs) { return lhs.entities() < rhs.entities(); });
	workers			 = std::min<size_t>(workers, std::thread::hardware_concurrency());
	auto max_workers = std::max<size_t>(1u, std::min(workers, largest_batch->entities() % min_thread_entities));

	// To guard having systems run with concurrent packs that have no data in them.
	// Doing so would seem counter-intuitive to users
	while((float)smallest_batch->entities() / (float)max_workers < 1.0f && max_workers > 1)
	{
		--max_workers;
	}
	workers = max_workers;

	packs.resize(workers);
	for(auto& dep_pack : source)
	{
		if(dep_pack.allow_partial())
		{
			auto batch_size = dep_pack.entities() / workers;
			size_t processed{0};
			for(auto i = 0; i < workers - 1; ++i)
			{
				packs[i].emplace_back(dep_pack.slice(processed, processed + batch_size));
				processed += batch_size;
			}
			packs[packs.size() - 1].emplace_back(dep_pack.slice(processed, dep_pack.entities()));
		}
		else // if packs cannot be split, then emplace the 'full' data
		{
			for(auto i = 0; i < workers; ++i) packs[i].emplace_back(dep_pack);
		}
	}
	return packs;
}

void state::prepare_system(std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
						   std::uintptr_t cache_offset, details::system_information& information)
{
	std::function<void(state&, psl::array<details::dependency_pack>)> write_data =
		[](state& state, psl::array<details::dependency_pack> dep_packs) {
			for(const auto& dep_pack : dep_packs)
			{
				for(auto& binding : dep_pack.m_RWBindings)
				{
					const size_t size	= dep_pack.m_Sizes.at(binding.first);
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

			cache_offset += prepare_bindings(entities, (void*)cache_offset, dep_pack);
		}

		auto multi_pack = slice(pack, m_Scheduler->workers());

		// psl::array<std::future<void>> future_commands;

		auto index = info_buffer.size();
		for(size_t i = 0; i < m_Scheduler->workers(); ++i)
			info_buffer.emplace_back(new info(*this, dTime, rTime, m_Tick));

		auto infoBuffer = std::next(std::begin(info_buffer), index);

		for(auto& mPack : multi_pack)
		{
			auto t1 = m_Scheduler->schedule([& fn = information.system(), infoBuffer, mPack]() mutable {
				return std::invoke(fn, infoBuffer->get(), mPack);
			});
			auto t2 = m_Scheduler->schedule(
				[&write_data, this, mPack = mPack]() { return std::invoke(write_data, *this, mPack); });

			t2.after(t1);

			// future_commands.emplace_back(std::move(t1));
			infoBuffer = std::next(infoBuffer);
		}
		m_Scheduler->execute();
	}
	else
	{
		for(auto& dep_pack : pack)
		{
			auto entities = filter(dep_pack);
			if(entities.size() == 0) continue;

			cache_offset += prepare_bindings(entities, (void*)cache_offset, dep_pack);
		}
		info_buffer.emplace_back(new info(*this, dTime, rTime, m_Tick));
		information.operator()(*info_buffer[info_buffer.size() - 1], pack);

		write_data(*this, pack);
	}
}


void state::tick(std::chrono::duration<float> dTime)
{
	m_LockState			   = 1;
	auto modified_entities = psl::array<entity>{m_ModifiedEntities.indices()};
	std::sort(std::begin(modified_entities), std::end(modified_entities));

	// apply filterings
	for(auto& filter_data : m_Filters)
	{
		filter(filter_data, modified_entities);
	}

	m_ModifiedEntities.clear();

	// tick systems;
	for(auto& system : m_SystemInformations)
	{
		prepare_system(dTime, dTime, (std::uintptr_t)m_Cache.data(), system);
	}

	m_Orphans.insert(std::end(m_Orphans), std::begin(m_ToBeOrphans), std::end(m_ToBeOrphans));
	m_ToBeOrphans.clear();

	for(auto& [key, cInfo] : m_Components) cInfo->purge();

	for(auto& info : info_buffer)
	{
		execute_command_buffer(*info);
	}
	info_buffer.clear();

	// purge;
	++m_Tick;

	if(m_NewSystemInformations.size() > 0)
	{
		for(auto& system : m_NewSystemInformations) m_SystemInformations.emplace_back(std::move(system));
		m_NewSystemInformations.clear();
	}

	if(m_ToRevoke.size() > 0)
	{
		for(auto id : m_ToRevoke)
		{
			revoke(id);
		}
		m_ToRevoke.clear();
	}
	m_LockState = 0;
}

const details::component_info* state::get_component_info(details::component_key_t key) const noexcept
{
	if(auto it = m_Components.find(key); it != std::end(m_Components))
		return &it->second.get();
	else
		return nullptr;
}

details::component_info* state::get_component_info(details::component_key_t key) noexcept
{
	if(auto it = m_Components.find(key); it != std::end(m_Components))
		return &it->second.get();
	else
		return nullptr;
}


psl::array<const details::component_info*>
state::get_component_info(psl::array_view<details::component_key_t> keys) const noexcept
{
	psl::array<const details::component_info*> res{};
	size_t count = keys.size();
	for(const auto& [key, cInfo] : m_Components)
	{
		if(count == 0) break;
		if(auto it = std::find(std::begin(keys), std::end(keys), key); it != std::end(keys))
		{
			res.push_back(&cInfo.get());
			--count;
		}
	}
	return res;
}

// empty construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities)
{
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	cInfo->add(entities);
	for(size_t i = 0; i < entities.size(); ++i) m_ModifiedEntities.try_insert(entities[i]);
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities,
							   std::function<void(std::uintptr_t, size_t)> invocable)
{
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);
	const auto component_size = cInfo->component_size();
	assert(component_size != 0);

	auto offset = cInfo->entities().size();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * component_size);
	std::invoke(invocable, location, entities.size());
	for(size_t i = 0; i < entities.size(); ++i) m_ModifiedEntities.try_insert(entities[i]);
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities,
							   void* prototype, bool repeat)
{
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);
	const auto component_size = cInfo->component_size();
	assert(component_size != 0);

	auto offset = cInfo->entities().size();

	cInfo->add(entities);
	if(repeat)
	{
		for(auto e : entities)
		{
			std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * component_size), prototype, component_size);
			m_ModifiedEntities.try_insert(e);
		}
	}
	else
	{
		for(auto e : entities)
		{
			std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * component_size), prototype, component_size);
			m_ModifiedEntities.try_insert(e);
			prototype = (void*)((std::uintptr_t)prototype + component_size);
		}
	}
}


void state::remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept
{
	m_Components[key]->destroy(entities);
	for(size_t i = 0; i < entities.size(); ++i) m_ModifiedEntities.try_insert(entities[i]);
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void state::destroy(psl::array_view<entity> entities) noexcept
{
	if(entities.size() == 0) return;

	for(auto& [key, cInfo] : m_Components)
	{
		cInfo->destroy(entities);
	}

	m_ToBeOrphans.insert(std::end(m_ToBeOrphans), std::begin(entities), std::end(entities));
	for(size_t i = 0; i < entities.size(); ++i) m_ModifiedEntities.try_insert(entities[i]);
}

void state::destroy(entity entity) noexcept
{
	for(auto& [key, cInfo] : m_Components)
	{
		cInfo->destroy(entity);
	}
	m_ToBeOrphans.emplace_back(entity);
	m_ModifiedEntities.try_insert(entity);
}

psl::array<entity>::iterator state::filter_op(details::component_key_t key, psl::array<entity>::iterator& begin,
												  psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin
							  : std::partition(begin, end, [cInfo](entity e) { return cInfo->has_component(e); });
}

psl::array<entity>::iterator state::on_add_op(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin : std::partition(begin, end, [cInfo](entity e) { return cInfo->has_added(e); });
}
psl::array<entity>::iterator state::on_remove_op(details::component_key_t key,
															psl::array<entity>::iterator& begin,
															psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin : std::partition(begin, end, [cInfo](entity e) { return cInfo->has_removed(e); });
}
psl::array<entity>::iterator state::on_except_op(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? end
							  : std::partition(begin, end, [cInfo](entity e) { return !cInfo->has_component(e); });
}
psl::array<entity>::iterator state::on_break_op(psl::array<details::component_key_t> keys,
														   psl::array<entity>::iterator& begin,
														   psl::array<entity>::iterator& end) const noexcept
{
	auto cInfos = get_component_info(keys);

	return (cInfos.size() != keys.size()) ? begin :
										  // for every entity, remove if...
			   std::partition(begin, end, [&cInfos](entity e) {
				   return
					   // any of them have not had an entity removed
					   !(!std::any_of(std::begin(cInfos), std::end(cInfos),
									  [e](const details::component_info* cInfo) { return cInfo->has_removed(e); }) ||
						 // or all of them do not have a component, or had the entity removed
						 !std::all_of(std::begin(cInfos), std::end(cInfos), [e](const details::component_info* cInfo) {
							 return cInfo->has_component(e) || cInfo->has_removed(e);
						 }));
			   });
}

psl::array<entity>::iterator state::on_combine_op(psl::array<details::component_key_t> keys,
															 psl::array<entity>::iterator& begin,
															 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfos = get_component_info(keys);

	return (cInfos.size() != keys.size()) ? begin : std::remove_if(begin, end, [cInfos](entity e) {
		return !std::any_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_added(e); }) ||
			   !std::all_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_component(e); });
	});
}

psl::array<entity> state::filter(details::dependency_pack& pack) const noexcept
{
	details::filter_group group{pack.filters, pack.on_add, pack.on_remove, pack.except, pack.on_combine, pack.on_break};

	auto it = std::find_if(std::begin(m_Filters), std::end(m_Filters), [&group](const filter_data& data) { return data.group == group; });
	assert(it != std::end(m_Filters));

	auto entities = it->entities;
	auto begin = std::begin(entities);
	auto end = std::end(entities);

	for (const auto& conditional : pack.on_condition)
	{
		end = std::invoke(conditional, begin, end, *this);
	}

	std::invoke(pack.orderby, begin, end, *this);

	entities.erase(end, std::end(entities));

	assert_debug_break(
		std::all_of(std::begin(pack.filters), std::end(pack.filters), [this, &entities](auto filter) {
			auto cInfo = get_component_info(filter);
			return std::all_of(std::begin(entities), std::end(entities),
				[filter, &cInfo](entity e) { return cInfo->has_storage_for(e); });
			}));
	return entities;
}

void state::filter(filter_data& data) const noexcept
{
	std::optional<psl::array_view<entity>> source;

	for (auto filter : data.group.on_remove)
	{
		auto cInfo = get_component_info(filter);
		if (!cInfo)
		{
			data.entities = {};
			return;
		}
		if (!source || cInfo->removed_entities().size() < source.value().size())
		{
			source = cInfo->removed_entities();
		}
	}
	for (auto filter : data.group.on_break)
	{
		auto cInfo = get_component_info(filter);
		if (!cInfo)
		{
			data.entities = {};
			return;
		}
		if (!source || cInfo->entities(true).size() < source.value().size())
		{
			source = cInfo->entities(true);
		}
	}
	for (auto filter : data.group.on_add)
	{
		auto cInfo = get_component_info(filter);
		if (!cInfo)
		{
			data.entities = {};
			return;
		}
		if (!source || cInfo->added_entities().size() < source.value().size())
		{
			source = cInfo->added_entities();
		}
	}
	for (auto filter : data.group.on_combine)
	{
		auto cInfo = get_component_info(filter);
		if (!cInfo)
		{
			data.entities = {};
			return;
		}
		if (!source || cInfo->entities().size() < source.value().size())
		{
			source = cInfo->entities();
		}
	}

	for (auto filter : data.group.filters)
	{
		auto cInfo = get_component_info(filter);
		if (!cInfo)
		{
			data.entities = {};
			return;
		}
		if (!source || cInfo->entities().size() < source.value().size())
		{
			source = cInfo->entities();
		}
	}

	if (source)
	{
		psl::array<entity> result{ source.value() };
		auto begin = std::begin(result);
		auto end = std::end(result);

		for (auto filter : data.group.on_remove)
		{
			end = on_remove_op(filter, begin, end);
		}
		if (data.group.on_break.size() > 0)
		{
			end = on_break_op(data.group.on_break, begin, end);
		}
		for (auto filter : data.group.on_add)
		{
			end = on_add_op(filter, begin, end);
		}
		if (data.group.on_combine.size() > 0) end = on_combine_op(data.group.on_combine, begin, end);


		for (auto filter : data.group.filters)
		{
			end = filter_op(filter, begin, end);
		}
		for (auto filter : data.group.except)
		{
			end = on_except_op(filter, begin, end);
		}

		// todo support order_by and on_condition

		data.entities = { begin, end };
		return;
	}
}

void state::filter(filter_data& data, psl::array_view<entity> source) const noexcept
{
	if(source.size() == 0)
	{
		if(data.group.clear_every_frame())
		{
			data.entities.clear();
		}
	}
	else
	{
		psl::array<entity> result{source};
		auto begin = std::begin(result);
		auto end   = std::end(result);

		for(auto filter : data.group.on_remove)
		{
			end = on_remove_op(filter, begin, end);
		}
		if(data.group.on_break.size() > 0)
		{
			end = on_break_op(data.group.on_break, begin, end);
		}
		for(auto filter : data.group.on_add)
		{
			end = on_add_op(filter, begin, end);
		}
		if(data.group.on_combine.size() > 0) end = on_combine_op(data.group.on_combine, begin, end);


		for(auto filter : data.group.filters)
		{
			end = filter_op(filter, begin, end);
		}
		for(auto filter : data.group.except)
		{
			end = on_except_op(filter, begin, end);
		}

		// todo support order_by and on_condition

		std::sort(begin, end);
		if(data.group.clear_every_frame())
		{
			result.erase(end, std::end(result));
			data.entities = std::move(result);
		}
		else
		{
			std::sort(end, std::end(result));

			psl::array<entity> new_target;
			std::set_difference(std::begin(data.entities), std::end(data.entities), end, std::end(result),
								std::back_inserter(new_target));

			auto size = std::size(new_target);
			new_target.insert(std::end(new_target), begin, end);
			std::inplace_merge(std::begin(new_target), std::next(std::begin(new_target), size), std::end(new_target));
			data.entities = std::move(new_target);
		}
	}
	assert_debug_break(std::unique(std::begin(data.entities), std::end(data.entities)) == std::end(data.entities));
	assert_debug_break(
		std::all_of(std::begin(data.group.on_combine), std::end(data.group.on_combine), [this, &data](auto filter) {
			auto cInfo = get_component_info(filter);
			return std::all_of(std::begin(data.entities), std::end(data.entities),
							   [filter, &cInfo](entity e) { return cInfo->has_storage_for(e); });
		}));
}

size_t state::prepare_data(psl::array_view<entity> entities, void* cache, component_key_t id) const noexcept
{
	if(entities.size() == 0) return 0;
	const auto& cInfo = get_component_info(id);
	assert_debug_break(cInfo != nullptr);
	assert_debug_break(
		std::all_of(std::begin(entities), std::end(entities), [&cInfo](auto e) { return cInfo->has_storage_for(e); }));
	return cInfo->copy_to(entities, cache);
}

size_t state::prepare_bindings(psl::array_view<entity> entities, void* cache, details::dependency_pack& dep_pack) const
	noexcept
{
	size_t offset_start = (std::uintptr_t)cache;

	std::memcpy(cache, entities.data(), sizeof(entity) * entities.size());
	dep_pack.m_Entities =
		psl::array_view<entity>((entity*)cache, (entity*)((std::uintptr_t)cache + (sizeof(entity) * entities.size())));

	cache = (void*)((std::uintptr_t)cache + (sizeof(entity) * entities.size()));


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

size_t state::set(psl::array_view<entity> entities, details::component_key_t key, void* data) noexcept
{
	if(entities.size() == 0) return 0;
	const auto& cInfo = get_component_info(key);
	assert(cInfo != nullptr);
	return cInfo->copy_from(entities, data);
}


void state::execute_command_buffer(info& info)
{
	auto& buffer = info.command_buffer;

	psl::sparse_array<entity> remapped_entities;
	if(buffer.m_Entities.size() > 0)
	{
		psl::array<entity> added_entities;
		std::set_difference(std::begin(buffer.m_Entities), std::end(buffer.m_Entities),
							std::begin(buffer.m_DestroyedEntities), std::end(buffer.m_DestroyedEntities),
							std::back_inserter(added_entities));


		for(auto e : added_entities)
		{
			remapped_entities[e] = create();
		}
	}
	for(auto& component_src : buffer.m_Components)
	{
		if(component_src->entities(true).size() == 0) continue;
		auto component_dst = get_component_info(component_src->id());

		component_src->remap(remapped_entities, [first = buffer.m_First](entity e) -> bool { return e >= first; });
		if(component_dst == nullptr)
		{
			for(auto e : component_src->entities(true)) m_ModifiedEntities.try_insert(e);
			m_Components[component_src->id()] = std::move(component_src);
		}
		else
		{
			component_dst->merge(*component_src);
			for(auto e : component_src->entities(true)) m_ModifiedEntities.try_insert(e);
		}
	}
	auto destroyed_entities = buffer.m_DestroyedEntities;
	auto mid				= std::partition(std::begin(destroyed_entities), std::end(destroyed_entities),
								 [first = buffer.m_First](auto e) { return e >= first; });
	if(mid != std::end(destroyed_entities))
		destroy(psl::array_view<entity>{&*mid, static_cast<size_t>(std::distance(mid, std::end(destroyed_entities)))});
}


size_t state::count(psl::array_view<details::component_key_t> keys) const noexcept
{
	for(auto& key : keys)
	{
		auto cInfo = get_component_info(key);
		return cInfo ? cInfo->size() : 0;
	}
	return 0;
}

void state::clear() noexcept
{
	m_Components.clear();
	m_Entities = 0;
	m_Orphans.clear();
	m_ToBeOrphans.clear();
	m_SystemInformations.clear();
	m_NewSystemInformations.clear();
	m_Filters.clear();
	m_LockState = 0;
}