
#pragma once
#include "command_buffer.hpp"
#include "details/component_container.hpp"
#include "details/component_key.hpp"
#include "details/system_information.hpp"
#include "entity.hpp"
#include "filtering.hpp"
#include "psl/array.hpp"
#include "psl/details/fixed_astring.hpp"
#include "psl/ecs/component_traits.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/pack_view.hpp"
#include "psl/unique_ptr.hpp"
#include "selectors.hpp"
#include <chrono>

#include "psl/serialization/serializer.hpp"

namespace psl::async {
class scheduler;
}

/// \brief Private implementation details for the ECS.
/// \warning Users should not rely on these implementations.
namespace psl::ecs::details {}

/// \brief Entity Component System
///
namespace psl::ecs {
class state_t final {
	friend class psl::serialization::accessor;
	static constexpr auto serialization_name {"ECS"};

	template <typename S>
	void serialize(S& serializer) {
		if constexpr(psl::serialization::details::IsDecoder<S>) {
			if(m_Tick != 0 || m_Entities != 0 || m_ModifiedEntities.size() != 0) {
				throw std::runtime_error("unsupported deserializing into non-empty state");
			}
		}

		serializer.template parse<"ORPHANS">(m_Orphans);
		serializer.template parse<"FUTURE_ORPHANS">(m_ToBeOrphans);
		serializer.template parse<"ENTITIES">(m_Entities);

		std::vector<size_t> component_sizes {};
		std::vector<entity> component_entities {};
		std::vector<size_t> component_data_size {};
		std::vector<size_t> component_data_alignment {};
		std::vector<std::byte> component_data {};
		std::vector<std::string> component_names {};

		if constexpr(psl::serialization::details::IsEncoder<S>) {
			size_t expected_total_datasize {0};
			size_t expected_total_entities {0};
			std::vector<details::component_key_t> all_keys {};
			for(const auto& [key, component] : m_Components) {
				// skip unserializable types, or those that aren't requesting to be serialized
				if(key.type() == component_type::COMPLEX || !component->should_serialize())
					continue;
				expected_total_entities += component->size(true);

				all_keys.emplace_back(key);

				expected_total_datasize +=
				  (component->component_size() > 0) ? component->component_size() * component->size(true) : 0;
			}

			// sort to make the serializations deterministic
			std::sort(std::begin(all_keys), std::end(all_keys));
			component_entities.reserve(expected_total_entities);
			component_data.resize(expected_total_datasize);

			size_t data_offset {0};
			for(const auto& key : all_keys) {
				const auto& component = m_Components[key];

				component_sizes.emplace_back(component->size(true));
				auto entities = component->entities(true);
				component_entities.insert(std::end(component_entities), std::begin(entities), std::end(entities));
				component_data_size.emplace_back(component->component_size());
				component_data_alignment.emplace_back(component->alignment());
				component_names.emplace_back(key.name());

				if(component->component_size() > 0) {
					auto total_size = component->component_size() * component->size(true);
					memcpy(component_data.data() + data_offset, component->data(), total_size);
					data_offset += total_size;
				}
			}
		}

		serializer.template parse<"COMPONENTS">(component_names);
		serializer.template parse<"CDATASIZE">(component_data_size);
		serializer.template parse<"CDATAALIGNMENT">(component_data_alignment);
		serializer.template parse<"CSIZE">(component_sizes);
		serializer.template parse<"CENTITIES">(component_entities);
		serializer.template parse<"CDATA">(component_data);


		if constexpr(psl::serialization::details::IsDecoder<S>) {
			const auto count = component_names.size();
			for(size_t i = 0, entity_offset = 0, data_offset = 0; i < count; entity_offset += component_sizes[i],
					   data_offset += (component_sizes[i] * component_data_size[i]),
					   ++i) {
				details::component_key_t key(
				  component_names[i], (component_data_size[i] == 0) ? component_type::FLAG : component_type::TRIVIAL);
				auto it = m_Components.find(key);
				if(it == m_Components.end()) {
					auto pair = m_Components.emplace(key,
													 details::instantiate_component_container(
													   key, component_data_size[i], component_data_alignment[i], true));

					if(!pair.second) {
						throw std::runtime_error("failed to insert key into map");
					}
					it = pair.first;
				} else {
					throw std::runtime_error("unsupported deserializing into non-empty state");
				}

				psl::array_view<psl::ecs::entity> entities {
				  std::next(std::begin(component_entities), entity_offset),
				  std::next(std::begin(component_entities), entity_offset + component_sizes[i])};

				add_component_impl(key, entities, (void*)(&*std::next(std::begin(component_data), data_offset)), false);
			}
		}
	}


	struct transform_result {
		bool operator==(const transform_result& other) const noexcept { return group == other.group; }
		psl::array<entity> entities;
		psl::array<entity> indices;	   // used in case there is an order_by
		std::shared_ptr<details::transform_group> group;
	};

	struct filter_result {
		bool operator==(const filter_result& other) const noexcept { return group == other.group; }
		bool operator==(const details::filter_group& other) const noexcept { return *group == other; }
		psl::array<entity> entities;
		std::shared_ptr<details::filter_group> group;

		// all transformations that will depend on this result
		psl::array<transform_result> transformations;
	};


  public:
	state_t(size_t workers = 0, size_t cache_size = 1024 * 1024 * 256);
	~state_t();
	state_t(const state_t&)			   = delete;
	state_t(state_t&&)				   = default;
	state_t& operator=(const state_t&) = delete;
	state_t& operator=(state_t&&)	   = default;

	template <IsComponentTypeSerializable T>
	bool override_serialization(bool value) {
		constexpr auto key = details::component_key_t::generate<T>();
		if(auto it = m_Components.find(key); it != std::end(m_Components)) {
			return it->second->should_serialize(value);
		}
		return false;
	}

	template <typename... Ts>
	void add_components(psl::array_view<entity> entities) {
		(add_component<Ts>(entities), ...);
	}

	template <typename... Ts>
	void add_components(psl::array_view<entity> entities, psl::array_view<Ts>... data) {
		(add_component<Ts>(entities, data), ...);
	}
	template <typename... Ts>
	void add_components(psl::array_view<entity> entities, Ts&&... prototype) {
		(add_component(entities, std::forward<Ts>(prototype)), ...);
	}

	template <typename... Ts>
	void remove_components(psl::array_view<entity> entities) noexcept {
		(remove_component(details::component_key_t::generate<Ts>(), entities), ...);
	}

	template <typename T>
	psl::array<T> get_component(psl::array_view<entity> entities) const noexcept {
		auto cInfo = get_component_typed_info<T>();
		psl::array<T> result {};
		result.resize(entities.size());
		cInfo->copy_to(entities, result.data());
		return result;
	}

	void clear(bool release_memory = true) noexcept;

	template <typename T>
	T& get(entity entity) {
		// todo this should support filtering
		auto cInfo = get_component_typed_info<T>();
		return cInfo->entity_data().template at<T>(entity, details::stage_range_t::ALL);
	}

	template <typename T>
	const T& get(entity entity) const noexcept {
		// todo this should support filtering
		auto cInfo = get_component_typed_info<T>();
		return cInfo->entity_data().template at<T>(entity, details::stage_range_t::ALL);
	}
	
	template <typename T>
	psl::array<T> get(psl::array_view<entity> entities) const {
		auto cInfo = get_component_typed_info<T>();
		psl::array<T> result {};
		result.resize(entities.size());
		cInfo->copy_to(entities, result.data());
		return result;
	}

	template <typename... Ts>
	bool has_components(psl::array_view<entity> entities) const noexcept {
		auto cInfos = get_component_container(to_keys<Ts...>());
		if(cInfos.size() == sizeof...(Ts)) {
			return std::all_of(std::begin(cInfos), std::end(cInfos), [&entities](const auto& cInfo) {
				return cInfo && std::all_of(std::begin(entities), std::end(entities), [&cInfo](entity e) {
						   return cInfo->has_component(e);
					   });
			});
		}
		return sizeof...(Ts) == 0;
	}

	[[maybe_unused]] entity create() {
		if(m_Orphans.size() > 0) {
			auto entity = m_Orphans.back();
			m_Orphans.pop_back();
			return entity;
		} else {
			return m_Entities++;
		}
	}

	template <typename... Ts>
	[[maybe_unused]] psl::array<entity> create(entity count) {
		psl::array<entity> entities;
		entities.reserve(count);
		const auto recycled	 = std::min<entity>(count, (entity)m_Orphans.size());
		const auto remainder = count - recycled;

		std::reverse_copy(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans), std::back_inserter(entities));
		m_Orphans.erase(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans));
		entities.resize(count);
		std::iota(std::next(std::begin(entities), recycled), std::end(entities), m_Entities);
		m_Entities += remainder;

		if constexpr(sizeof...(Ts) > 0) {
			(add_components<Ts>(entities), ...);
		}
		return entities;
	}

	template <typename... Ts>
	[[maybe_unused]] psl::array<entity> create(entity count, Ts&&... prototype) {
		psl::array<entity> entities;
		entities.reserve(count);
		const auto recycled	 = std::min<entity>(count, static_cast<entity>(m_Orphans.size()));
		const auto remainder = count - recycled;

		std::reverse_copy(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans), std::back_inserter(entities));
		m_Orphans.erase(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans));
		entities.resize(count);
		std::iota(std::next(std::begin(entities), recycled), std::end(entities), m_Entities);
		m_Entities += remainder;

		add_components(entities, std::forward<Ts>(prototype)...);

		return entities;
	}

	void destroy(psl::array_view<entity> entities) noexcept;
	void destroy(entity entity) noexcept;

	psl::array<entity> all_entities() const noexcept {
		auto orphans = m_Orphans;
		std::sort(std::begin(orphans), std::end(orphans));

		auto orphan_it = std::begin(orphans);
		psl::array<entity> result;
		result.reserve(m_Entities - orphans.size());
		for(entity e = 0; e < m_Entities; ++e) {
			if(orphan_it != std::end(m_Orphans) && e == *orphan_it) {
				orphan_it = std::next(orphan_it);
				continue;
			}
			result.emplace_back(e);
		}
		return result;
	}

	template <typename... Ts>
	psl::array<entity> filter() const noexcept {
		auto filter_group = details::make_filter_group(psl::type_pack_t<psl::ecs::pack<Ts...>> {});
		psl_assert(filter_group.size() == 1, "expected only one filter group");

		auto it = std::find_if(std::begin(m_Filters), std::end(m_Filters), [&filter_group](const filter_result& data) {
			return *data.group == filter_group[0];
		});

		if(it != std::end(m_Filters)) {
			auto modified = psl::array<entity> {m_ModifiedEntities.indices()};
			std::sort(std::begin(modified), std::end(modified));

			filter_result data {it->entities, it->group};
			filter(data, modified);
			return data.entities;
		}
		// run on all entities, as no pre-existing filtering group could be found
		// todo: look into best fit filtering groups to seed this with
		else {
			filter_result data {{}, std::make_shared<details::filter_group>(filter_group[0])};
			filter(data);
			return data.entities;
		}
	}

	template <typename... Ts>
	psl::array<entity> filter(psl::array_view<entity> entities) const noexcept {
		auto filter_group = details::make_filter_group(psl::type_pack_t<psl::ecs::pack<Ts...>> {});
		psl_assert(filter_group.size() == 1, "expected only one filter group");

		filter_result data {{}, std::make_shared<details::filter_group>(filter_group[0])};
		filter(data, entities);
		return data.entities;
	}

	template <typename... Ts>
	void set_components(psl::array_view<entity> entities, psl::array_view<Ts>... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
	}

	template <typename... Ts>
	void set_components(psl::array_view<entity> entities, Ts&&... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
	}

	template <typename T>
	void set_component(psl::array_view<entity> entities, T&& data) noexcept {
		constexpr auto key = details::component_key_t::generate<T>();
		auto cInfo		   = get_component_container(key);
		for(auto e : entities) {
			cInfo->set(e, &data);
		}
	}

	template <typename T>
	void set_component(psl::array_view<entity> entities, psl::array_view<T> data) noexcept {
		psl_assert(entities.size() == data.size(),
				   "incorrect amount of data input compared to entities, expected {} but got {}",
				   entities.size(),
				   data.size());
		constexpr auto key = details::component_key_t::generate<T>();
		auto cInfo		   = get_component_container(key);
		auto d			   = std::begin(data);
		for(auto e : entities) {
			cInfo->set(e, *d);
			d = std::next(d);
		}
	}

	void tick(std::chrono::duration<float> dTime);

	void reset(psl::array_view<entity> entities) noexcept;

	template <typename T>
	psl::array_view<entity> entities() const noexcept {
		constexpr auto key {details::component_key_t::generate<T>()};
		if(auto it = m_Components.find(key); it != std::end(m_Components))
			return it->second->entities();
		return {};
	}

	template <typename T>
	psl::array_view<T> view() {
		constexpr auto key {details::component_key_t::generate<T>()};
		if(auto it = m_Components.find(key); it != std::end(m_Components)) {
			return (details::cast_component_container<T>(it->second.get()))
			  ->entity_data()
			  .template dense<T>(details::stage_range_t::ALIVE);
		}
		return {};
	}

	/// \brief returns the amount of active systems
	size_t systems() const noexcept { return m_SystemInformations.size() - m_ToRevoke.size(); }

	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto declare(Fn&& fn, bool seedWithExisting = false) {
		return declare_impl(threading::sequential, std::forward<Fn>(fn), (void*)nullptr, seedWithExisting, DebugName);
	}


	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto declare(threading threading, Fn&& fn, bool seedWithExisting = false) {
		return declare_impl(threading, std::forward<Fn>(fn), (void*)nullptr, seedWithExisting, DebugName);
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(Fn&& fn, T* ptr, bool seedWithExisting = false) {
		return declare_impl(threading::sequential, std::forward<Fn>(fn), ptr, seedWithExisting, DebugName);
	}
	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(threading threading, Fn&& fn, T* ptr, bool seedWithExisting = false) {
		return declare_impl(threading, std::forward<Fn>(fn), ptr, seedWithExisting, DebugName);
	}

	bool revoke(details::system_token id) noexcept {
		if(m_LockState) {
			if(auto it = std::find_if(std::begin(m_SystemInformations),
									  std::end(m_SystemInformations),
									  [&id](const auto& system) { return system.id() == id; });
			   it != std::end(m_SystemInformations) &&
			   // and we make sure we don't "double delete"
			   std::find(std::begin(m_ToRevoke), std::end(m_ToRevoke), id) == std::end(m_ToRevoke)) {
				m_ToRevoke.emplace_back(id);
				return true;
			}
			return false;
		} else {
			if(auto it = std::find_if(std::begin(m_SystemInformations),
									  std::end(m_SystemInformations),
									  [&id](const auto& system) { return system.id() == id; });
			   it != std::end(m_SystemInformations)) {
				m_SystemInformations.erase(it);
				return true;
			}
			return false;
		}
	}

	size_t capacity() const noexcept { return m_Entities; }

	template <typename... Ts>
	size_t size() const noexcept {
		// todo implement filtering?
		if constexpr(sizeof...(Ts) == 0) {
			return m_Entities - m_Orphans.size();
		} else {
			return size(to_keys<Ts...>());
		}
	}

	size_t size(psl::array_view<details::component_key_t> keys) const noexcept;

  private:
	//------------------------------------------------------------
	// helpers
	//------------------------------------------------------------
	size_t
	prepare_bindings(psl::array_view<entity> entities, void* cache, details::dependency_pack& dep_pack) const noexcept;
	size_t prepare_data(psl::array_view<entity> entities, void* cache, details::component_key_t id) const noexcept;

	void prepare_system(std::chrono::duration<float> dTime,
						std::chrono::duration<float> rTime,
						std::uintptr_t cache_offset,
						details::system_information& information);


	void execute_command_buffer(info_t& info);

	template <typename T>
	inline void create_storage() const noexcept {
		constexpr auto key = details::component_key_t::generate<T>();
		if(auto it = m_Components.find(key); it == std::end(m_Components)) {
			m_Components.emplace(key, details::instantiate_component_container<T>());
		}
	}

	details::component_container_t* get_component_container(const details::component_key_t& key) noexcept;
	const details::component_container_t* get_component_container(const details::component_key_t& key) const noexcept;

	psl::array<const details::component_container_t*>
	get_component_container(psl::array_view<details::component_key_t> keys) const noexcept;
	template <typename T>
	auto get_component_typed_info() const noexcept {
		constexpr auto key {details::component_key_t::generate<T>()};
		return details::cast_component_container<T>(m_Components.at(key).get());
	}
	//------------------------------------------------------------
	// add_component
	//------------------------------------------------------------

	template <typename T>
	void add_component(psl::array_view<entity> entities, T&& prototype) {
		using true_type = std::remove_const_t<std::remove_reference_t<T>>;
		if constexpr(psl::ecs::details::is_empty_container<true_type>::value) {
			using type = typename psl::ecs::details::empty_container<true_type>::type;
			create_storage<type>();
			if constexpr(details::DoesComponentTypeNeedPrototypeCall<type>) {
				type v {details::prototype_for<type>()};
				add_component_impl(details::component_key_t::generate<type>(), entities, &v);
			} else {
				add_component_impl(details::component_key_t::generate<type>(), entities);
			}
		} else if constexpr(psl::templates::is_callable_n<T, 1>::value) {
			using pack_type = typename psl::templates::func_traits<T>::arguments_t;
			static_assert(psl::type_pack_size_v<pack_type> == 1,
						  "only one argument is allowed in the prototype invocable");
			using arg0_t = psl::type_at_index_t<0, pack_type>;
			static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
						  "the argument type should be of 'T&'");
			using type = typename std::remove_reference<arg0_t>::type;
			static_assert(!std::is_empty_v<type>,
						  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
						  "psl::ecs::empty<T>{} to avoid initialization.");
			create_storage<type>();
			add_component_impl(
			  details::component_key_t::generate<type>(), entities, [prototype](std::uintptr_t location, size_t count) {
				  for(auto i = size_t {0}; i < count; ++i) {
					  std::invoke(prototype, *((type*)(location) + i));
				  }
			  });
		} else if constexpr(psl::templates::is_callable_n<T, 2>::value) {
			using pack_type = typename psl::templates::func_traits<T>::arguments_t;
			static_assert(psl::type_pack_size_v<pack_type> == 2, "two arguments required in the prototype invocable");
			using arg0_t = psl::type_at_index_t<0, pack_type>;
			static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
						  "the argument type for arg 0 should be of 'T&'");
			using arg1_t = psl::type_at_index_t<1, pack_type>;
			static_assert(std::is_same_v<arg1_t, entity>,
						  "the argument type for arg 1 should be of 'psl::ecs::entity'");
			using type = typename std::remove_reference<arg0_t>::type;
			static_assert(!std::is_empty_v<type>,
						  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
						  "psl::ecs::empty<T>{} to avoid initialization.");
			create_storage<type>();
			add_component_impl(details::component_key_t::generate<type>(),
							   entities,
							   [prototype, &entities](std::uintptr_t location, size_t count) {
								   for(auto i = size_t {0}; i < count; ++i) {
									   std::invoke(prototype, *((type*)(location) + i), entities[i]);
								   }
							   });
		}
		else if constexpr(/*std::is_trivially_copyable<T>::value && */std::is_standard_layout<T>::value/* &&
							  std::is_trivially_destructible<T>::value*/)
		{
			static_assert(!std::is_empty_v<T>,
						  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
						  "psl::ecs::empty<T>{} to avoid initialization.");


			create_storage<T>();
			add_component_impl(details::component_key_t::generate<T>(), entities, &prototype);
		} else if constexpr(details::is_range_t<true_type>::value) {
			using type = typename psl::ecs::details::is_range_t<true_type>::type;
			create_storage<type>();
			add_component_impl(details::component_key_t::generate<type>(), entities, prototype.data(), false);
		} else {
			static_assert(psl::templates::always_false<T>::value,
						  "could not figure out if the template type was an invocable or a component prototype");
		}
	}

	template <typename T>
	void add_component(psl::array_view<entity> entities) {
		create_storage<T>();
		if constexpr(details::DoesComponentTypeNeedPrototypeCall<T>) {
			T v {details::prototype_for<T>()};
			add_component_impl(details::component_key_t::generate<T>(), entities, &v);
		} else {
			add_component_impl(details::component_key_t::generate<T>(), entities);
		}
	}

	template <typename T>
	void add_component(psl::array_view<entity> entities, psl::array_view<T> data) {
		psl_assert(entities.size() == data.size(),
				   "incorrect amount of data input compared to entities, expected {} but got {}",
				   entities.size(),
				   data.size());
		create_storage<T>();
		static_assert(!std::is_empty_v<T>,
					  "no need to pass an array of tag types through, it's a waste of computing and memory");

		add_component_impl(details::component_key_t::generate<T>(), entities, data.data(), false);
	}


	void add_component_impl(const details::component_key_t& key, psl::array_view<entity> entities);

	// invocable based construction
	template <typename Fn>
	requires(std::is_invocable<Fn, std::uintptr_t, size_t>::value) void add_component_impl(
	  const details::component_key_t& key,
	  psl::array_view<entity> entities,
	  Fn&& invocable) {
		auto cInfo = get_component_container(key);
		psl_assert(cInfo != nullptr, "component info for key {} was not found", key);
		const auto component_size = cInfo->component_size();
		psl_assert(component_size != 0, "component size was 0");

		auto offset = cInfo->entities().size();
		cInfo->add(entities);

		auto location = (std::uintptr_t)cInfo->data() + (offset * component_size);
		std::invoke(invocable, location, entities.size());
		for(size_t i = 0; i < entities.size(); ++i) m_ModifiedEntities.try_insert(entities[i]);
	}

	void add_component_impl(const details::component_key_t& key,
							psl::array_view<entity> entities,
							void* prototype,
							bool repeat = true);

	//------------------------------------------------------------
	// remove_component
	//------------------------------------------------------------
	void remove_component(const details::component_key_t& key, psl::array_view<entity> entities) noexcept;


	//------------------------------------------------------------
	// filter
	//------------------------------------------------------------
	template <typename T>
	psl::array<entity>::iterator filter_op(psl::type_pack_t<T>,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept {
		return filter_op(details::component_key_t::generate<T>(), begin, end);
	}

	template <typename T>
	psl::array<entity>::iterator filter_op(psl::type_pack_t<psl::ecs::filter<T>>,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept {
		return filter_op(details::component_key_t::generate<T>(), begin, end);
	}
	template <typename T>
	psl::array<entity>::iterator filter_op(psl::type_pack_t<psl::ecs::on_add<T>>,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept {
		return on_add_op(details::component_key_t::generate<T>(), begin, end);
	}
	template <typename T>
	psl::array<entity>::iterator filter_op(psl::type_pack_t<psl::ecs::on_remove<T>>,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept {
		return on_remove_op(details::component_key_t::generate<T>(), begin, end);
	}
	template <typename T>
	psl::array<entity>::iterator filter_op(psl::type_pack_t<psl::ecs::except<T>>,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept {
		return on_except_op(details::component_key_t::generate<T>(), begin, end);
	}
	template <typename... Ts>
	psl::array<entity>::iterator filter_op(psl::type_pack_t<psl::ecs::on_break<Ts...>>,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept {
		return on_break_op(to_keys<Ts...>(), begin, end);
	}

	template <typename... Ts>
	psl::array<entity>::iterator filter_op(psl::type_pack_t<psl::ecs::on_combine<Ts...>>,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept {
		return on_combine_op(to_keys<Ts...>(), begin, end);
	}

	psl::array<entity>::iterator filter_op(details::component_key_t key,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept;
	psl::array<entity>::iterator on_add_op(details::component_key_t key,
										   psl::array<entity>::iterator& begin,
										   psl::array<entity>::iterator& end) const noexcept;
	psl::array<entity>::iterator on_remove_op(details::component_key_t key,
											  psl::array<entity>::iterator& begin,
											  psl::array<entity>::iterator& end) const noexcept;
	psl::array<entity>::iterator on_except_op(details::component_key_t key,
											  psl::array<entity>::iterator& begin,
											  psl::array<entity>::iterator& end) const noexcept;
	psl::array<entity>::iterator on_break_op(psl::array<details::component_key_t> keys,
											 psl::array<entity>::iterator& begin,
											 psl::array<entity>::iterator& end) const noexcept;
	psl::array<entity>::iterator on_combine_op(psl::array<details::component_key_t> keys,
											   psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept;

	psl::array<entity> filter(details::dependency_pack& pack, bool seed_with_previous) const noexcept;
	void filter(filter_result& data, psl::array_view<entity> source) const noexcept;
	void filter(filter_result& data, bool seed_with_previous = false) const noexcept;


	//------------------------------------------------------------
	// transformations
	//------------------------------------------------------------

	void transform(transform_result& data, psl::array_view<entity> source) const noexcept;
	void transform(transform_result& data) const noexcept;


	template <typename... Ts>
	psl::array<details::component_key_t> to_keys() const noexcept {
		return psl::array<details::component_key_t> {details::component_key_t::generate<Ts>()...};
	}


	//------------------------------------------------------------
	// set
	//------------------------------------------------------------
	size_t set(psl::array_view<entity> entities, const details::component_key_t& key, void* data) noexcept;


	//------------------------------------------------------------
	// system declare
	//------------------------------------------------------------
	template <typename... Ts>
	struct get_packs {
		using type = psl::type_pack_t<Ts...>;
	};

	template <typename... Ts>
	struct get_packs<psl::type_pack_t<Ts...>> : public get_packs<Ts...> {};

	template <typename... Ts>
	struct get_packs<psl::ecs::info_t&, Ts...> : public get_packs<Ts...> {};

	std::pair<std::shared_ptr<details::filter_group>, std::shared_ptr<details::transform_group>>
	add_filter_group(details::filter_group& filter_group,
					 details::transform_group& transform_group,
					 psl::string_view debugName) {
		std::shared_ptr<details::transform_group> shared_transform_group {};
		auto filter_it =
		  std::find_if(std::begin(m_Filters), std::end(m_Filters), [&filter_group](const filter_result& data) {
			  return *data.group == filter_group;
		  });
		if(filter_it == std::end(m_Filters)) {
			m_Filters.emplace_back(filter_result {{}, std::make_shared<details::filter_group>(filter_group)});
			filter_it = std::prev(std::end(m_Filters));
		}

		filter_it->group->add_debug_system_name(debugName);

		if(transform_group) {
			auto it =
			  std::find_if(std::begin(filter_it->transformations),
						   std::end(filter_it->transformations),
						   [&transform_group](const transform_result& data) { return *data.group == transform_group; });
			if(it == std::end(filter_it->transformations)) {
				filter_it->transformations.emplace_back(
				  transform_result {{}, {}, std::make_shared<details::transform_group>(transform_group)});

				it = std::prev(std::end(filter_it->transformations));
			}

			it->group->add_debug_system_name(debugName);
			return {filter_it->group, it->group};
		}

		return {filter_it->group, {}};
	}

	template <typename Fn, typename T, typename pack_t>
	auto create_system_tick_functional(Fn& fn, T* ptr) const noexcept {
		if constexpr(std::is_member_function_pointer<Fn>::value) {
			return [fn, ptr](psl::ecs::info_t& info, psl::array<details::dependency_pack> packs) -> void {
				auto tuple_argument_list = std::tuple_cat(std::tuple<T*, psl::ecs::info_t&>(ptr, info),
														  details::compress_from_dependency_pack(pack_t {}, packs));

				std::apply(fn, std::move(tuple_argument_list));
			};
		} else {
			return [fn](psl::ecs::info_t& info, psl::array<details::dependency_pack> packs) -> void {
				auto tuple_argument_list = std::tuple_cat(std::tuple<psl::ecs::info_t&>(info),
														  details::compress_from_dependency_pack(pack_t {}, packs));

				std::apply(fn, std::move(tuple_argument_list));
			};
		}
	}

	template <typename Fn, typename T = void>
	auto
	declare_impl(threading threading, Fn&& fn, T* ptr, bool seedWithExisting = false, psl::string_view debugName = "") {
		using function_args	  = typename psl::templates::func_traits<typename std::decay<Fn>::type>::arguments_t;
		using pack_t		  = typename get_packs<function_args>::type;
		auto filter_groups	  = details::make_filter_group(pack_t {});
		auto transform_groups = details::make_transform_group(pack_t {});
		auto pack_generator	  = [](bool seedWithPrevious = false) {
			  return details::expand_to_dependency_pack(pack_t {}, seedWithPrevious);
		};

		auto system_tick = create_system_tick_functional<Fn, T, pack_t>(fn, ptr);
		auto& sys_info	 = (m_LockState) ? m_NewSystemInformations : m_SystemInformations;

		psl::array<std::shared_ptr<details::filter_group>> shared_filter_groups;
		psl::array<std::shared_ptr<details::transform_group>> shared_transform_groups;

		for(auto i = 0; i < filter_groups.size(); ++i) {
			auto [shared_filter, transform_filter] = add_filter_group(filter_groups[i], transform_groups[i], debugName);
			shared_filter_groups.emplace_back(shared_filter);
			shared_transform_groups.emplace_back(transform_filter);
		}


		sys_info.emplace_back(threading,
							  std::move(pack_generator),
							  std::move(system_tick),
							  shared_filter_groups,
							  shared_transform_groups,
							  ++m_SystemCounter,
							  seedWithExisting,
							  debugName);
		return sys_info[sys_info.size() - 1].id();
	}

	::memory::raw_region m_Cache {1024 * 1024 * 256};
	psl::array<psl::unique_ptr<info_t>> info_buffer {};
	psl::array<entity> m_Orphans {};
	psl::array<entity> m_ToBeOrphans {};
	mutable psl::array<filter_result> m_Filters {};
	psl::array<details::system_information> m_SystemInformations {};

	psl::array<details::system_token> m_ToRevoke {};
	psl::array<details::system_information> m_NewSystemInformations {};

	mutable std::unordered_map<details::component_key_t, std::unique_ptr<details::component_container_t>>
	  m_Components {};

	psl::sparse_indice_array<entity> m_ModifiedEntities {};

	psl::unique_ptr<psl::async::scheduler> m_Scheduler {nullptr};

	size_t m_LockState {0};
	size_t m_Tick {0};
	size_t m_SystemCounter {0};
	entity m_Entities {0};
};
}	 // namespace psl::ecs
