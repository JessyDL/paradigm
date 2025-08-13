
#pragma once
#include "command_buffer.hpp"
#include "details/component_container.hpp"
#include "details/component_key.hpp"
#include "details/mutate_instruction.hpp"
#include "details/system_information.hpp"
#include "entity.hpp"
#include "filtering.hpp"
#include "psl/array.hpp"
#include "psl/collections/indirect_array.hpp"
#include "psl/details/fixed_astring.hpp"
#include "psl/ecs/component_traits.hpp"
#include "psl/ecs/details/stage_range.hpp"
#include "psl/memory/raw_region.hpp"
#include "psl/pack_view.hpp"
#include "psl/string_utils.hpp"
#include "psl/unique_ptr.hpp"
#include "selectors.hpp"
#include <chrono>

#include "psl/serialization/serializer.hpp"

namespace psl::async {
class scheduler;
}

/// \brief Private implementation details for the ECS.
/// \warning Users should not rely on these implementations.
namespace psl::ecs::details {
template <typename... Ts>
struct get_packs {
	using type = psl::type_pack_t<Ts...>;
};

template <typename... Ts>
struct get_packs<psl::type_pack_t<Ts...>> : public get_packs<Ts...> {};

template <typename... Ts>
struct get_packs<psl::ecs::info_t&, Ts...> : public get_packs<Ts...> {};
}	 // namespace psl::ecs::details

namespace psl::utility {
template <>
struct converter<psl::ecs::entity_t> {
	using value_t	 = psl::ecs::entity_t;
	using view_t	 = psl::string8::view;
	using encoding_t = psl::string8_t;

	static encoding_t to_string(const value_t& x) {
		return converter<psl::ecs::entity_t::size_type> {}.to_string(static_cast<psl::ecs::entity_t::size_type>(x));
	}

	static value_t from_string(view_t str) {
		return psl::ecs::details::make_entity(converter<psl::ecs::entity_t::size_type> {}.from_string(str));
	}

	static void from_string(value_t& out, view_t str) {
		out = from_string(str);
	}

	static bool is_valid(view_t str) {
		return true;
	}
};
}	 // namespace psl::utility

/// \brief Entity Component System
///
namespace psl::ecs {
class state_t;

struct transient_system_tag_t {};

constexpr transient_system_tag_t transient_system_tag {};

class system_group_t final {
	friend class state_t;
	system_group_t(size_t id, psl::string_view debugName = "") : m_Id(id), m_DebugName(debugName) {}

  public:
	system_group_t() = default;

  private:
	size_t m_Id {std::numeric_limits<size_t>::max()};
	psl::string_view m_DebugName;
};

class state_t final {
	friend class psl::serialization::accessor;
	static constexpr auto serialization_name {"ECS"};


	// double linked list-like structure. Good enough for now, but could be refactored
	struct entity_relationship_t {
		entity_t parent {};
		entity_t first_child {};
		entity_t next_sibling {};
		entity_t prev_sibling {};
		entity_t::size_type children {0};	 // number of direct children this entity has
	};

	struct entity_relationship_component_t {
		std::vector<entity_t> children {};	  // direct children of this entity
		entity_t parent {};
		hierarchy_change_event change_event {};

		bool is_root() const noexcept {
			return parent == invalid_entity;
		}
	};

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
		std::vector<entity_t> component_entities {};
		std::vector<size_t> component_data_size {};
		std::vector<size_t> component_data_alignment {};
		std::vector<std::byte> component_data {};
		std::vector<std::string> component_names {};
		std::vector<component_mutability_behaviour_t> component_mutability {};
		std::vector<size_t> component_versions {};

		if constexpr(psl::serialization::details::IsEncoder<S>) {
			size_t expected_total_datasize {0};
			size_t expected_total_entities {0};
			std::vector<details::component_key_t> all_keys {};
			for(const auto& [key, component] : m_Components) {
				// skip unserializable types, or those that aren't requesting to be serialized
				if(key.type() == component_type::COMPLEX || !component->component_type_info().serializable ||
				   key != component->id() /* mutation */) {
					continue;
				}
				expected_total_entities += component->size(true);

				all_keys.emplace_back(key);

				expected_total_datasize += (component->component_type_info().size > 0)
											 ? component->component_type_info().size * component->size(true)
											 : 0;
			}

			// sort to make the serializations deterministic
			std::sort(std::begin(all_keys), std::end(all_keys));
			component_entities.reserve(expected_total_entities);
			component_data.resize(expected_total_datasize);

			size_t data_offset {0};
			for(const auto& key : all_keys) {
				const auto& component = m_Components[key];

				if(component->size(true) == 0) {
					continue;	 // skip empty components
				}

				auto cti = component->component_type_info();

				// extra check, this shouldn't be possible to hit unless a contract was broken earlier.
				psl_assert(cti.serializable, "{} was not serializable", cti.id.name());

				component_sizes.emplace_back(component->size(true));
				auto entities = component->entities(true);
				component_entities.insert(std::end(component_entities), std::begin(entities), std::end(entities));
				component_data_size.emplace_back(cti.size);
				component_data_alignment.emplace_back(cti.alignment);
				component_names.emplace_back(key.name());
				component_mutability.emplace_back(cti.mutability);
				component_versions.emplace_back(cti.version);

				if(cti.size > 0) {
					auto total_size = cti.size * component->size(true);
					memcpy(component_data.data() + data_offset, component->data(), total_size);
					data_offset += total_size;
				}
			}
		}

		serializer.template parse<"COMPONENTS">(component_names);
		serializer.template parse<"CTI_VERSION">(component_versions);
		serializer.template parse<"CTI_DATASIZE">(component_data_size);
		serializer.template parse<"CTI_DATAALIGNMENT">(component_data_alignment);
		serializer.template parse<"CTI_MUTABILITY">(component_mutability);
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
					auto pair =
					  m_Components.emplace(key,
										   details::instantiate_component_container(details::component_type_info_t {
											 .id		   = key,
											 .version	   = component_versions[i],
											 .size		   = component_data_size[i],
											 .alignment	   = component_data_alignment[i],
											 .mutability   = component_mutability[i],
											 .serializable = true,
										   }));

					if(!pair.second) {
						throw std::runtime_error("failed to insert key into map");
					}
					it = pair.first;
				} else {
					// todo(jdl): this should also handle the version migration, see the lookup for the component
					// container how to do so.
					throw std::runtime_error("unsupported deserializing into non-empty state");
				}

				psl::array_view<psl::ecs::entity_t> entities {
				  std::next(std::begin(component_entities), entity_offset),
				  std::next(std::begin(component_entities), entity_offset + component_sizes[i])};

				add_component_impl(key, entities, (void*)(&*std::next(std::begin(component_data), data_offset)), false);
			}
		}
	}


	struct transform_result {
		bool operator==(const transform_result& other) const noexcept {
			return group == other.group;
		}
		psl::array<entity_t> entities;
		psl::array<entity_t::size_type> indices;	// used in case there is an order_by
		std::shared_ptr<details::transform_group> group;
	};

	struct filter_result {
		filter_result(psl::array<entity_t> entities				   = {},
					  std::shared_ptr<details::filter_group> group = {},
					  psl::array<transform_result> transformations = {})
			: entities(entities), group(group), transformations(transformations) {}
		filter_result(filter_result const& rhs)
			: entities(rhs.entities), direct_entities(rhs.direct_entities), group(rhs.group),
			  transformations(rhs.transformations) {}
		filter_result(filter_result&& rhs) noexcept
			: entities(std::move(rhs.entities)), direct_entities(std::move(rhs.direct_entities)), group(rhs.group),
			  transformations(std::move(rhs.transformations)) {}
		filter_result& operator=(filter_result const& rhs) {
			if(this == &rhs) {
				return *this;
			}

			entities		= rhs.entities;
			direct_entities = rhs.direct_entities;
			group			= rhs.group;
			transformations = rhs.transformations;
			return *this;
		}
		filter_result& operator=(filter_result&& rhs) noexcept {
			if(this == &rhs) {
				return *this;
			}
			entities		= std::move(rhs.entities);
			direct_entities = std::move(rhs.direct_entities);
			group			= std::move(rhs.group);
			transformations = std::move(rhs.transformations);
			return *this;
		}

		psl::array<entity_t> entities;
		std::optional<psl::array<entity_t>>
		  direct_entities;	  // activated when the grouping has relationship filtering
		// this way the entities contain _all_ entities opaquely for the systems (and system preparation), but for the
		// filtering we can be sure that we're not going to get frame drift (f.e. when the parent is added, the next
		// frame the .entities will now believe the parent is a valid source entry, and so their parents will now be
		// added.
		std::shared_ptr<details::filter_group> group;

		// all transformations that will depend on this result
		psl::array<transform_result> transformations;
	};


  public:
	state_t(size_t workers								= 0,
			size_t cache_size							= 1024 * 1024 * 128,
			entity_t::size_type min_entities_per_worker = 1024);
	~state_t();
	state_t(const state_t&)			   = delete;
	state_t(state_t&&)				   = delete;
	state_t& operator=(const state_t&) = delete;
	state_t& operator=(state_t&&)	   = delete;

	template <IsComponentTypeSerializable T>
	void override_serialization(bool value) {
		constexpr auto key = details::component_key_t::generate<T>();
		if(auto it = m_Components.find(key); it != std::end(m_Components)) {
			it->second->should_serialize(value);
		}
	}


	template <typename T>
		requires((!IsFilteringOp<T> && IsRestrictedMutable<T>))
	void mutate_components(psl::array_view<entity_t> entities, auto&& prototype) {
		add_component<details::mutate_instruction_t<T>>(entities, std::forward<decltype(prototype)>(prototype));
	}

	template <typename... Ts>
	void add_components(psl::array_view<entity_t> entities) {
		(add_component<Ts>(entities), ...);
	}

	template <typename... Ts>
	void add_components(psl::array_view<entity_t> entities, psl::array_view<Ts>... data) {
		(add_component<Ts>(entities, data), ...);
	}
	template <typename... Ts>
	void add_components(psl::array_view<entity_t> entities, Ts&&... prototype) {
		(add_component<Ts>(entities, std::forward<Ts>(prototype)), ...);
	}

	template <typename... Ts>
	void remove_components(psl::array_view<entity_t> entities) noexcept {
		(remove_component(get_component_untyped_info<Ts>(), entities), ...);
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	psl::array<T> get_component(psl::ecs::direct_t, psl::array_view<entity_t> entities) const noexcept {
		auto cInfo = get_component_typed_info<T>();
		psl::array<T> result {};
		result.resize(entities.size());
		cInfo->copy_to(entities, result.data());
		return result;
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	auto get_component(psl::ecs::indirect_t, psl::array_view<entity_t> entities) const noexcept
	  -> psl::indirect_array_t<T, entity_t::size_type> {
		auto cInfo = get_component_typed_info<T>();
		psl::array<psl::ecs::entity_t::size_type> indices {};
		indices.resize(entities.size());
		cInfo->write_memory_location_offsets_for(entities, indices.data());
		return psl::indirect_array_t<T, entity_t::size_type> {std::move(indices), (T*)cInfo->data()};
	}

	template <typename T, IsAccessType access = psl::ecs::indirect_t>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	inline auto get_component(psl::array_view<entity_t> entities) const noexcept {
		return get_component<T>(access {}, entities);
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	auto get_component(psl::ecs::direct_t, entity_t entity) const noexcept -> T& {
		auto cInfo = get_component_typed_info<T>();
		return *(T*)(cInfo->get_if(entity));
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	auto get_component(psl::ecs::indirect_t, entity_t entity) const noexcept -> T& {
		auto cInfo = get_component_typed_info<T>();
		return *(T*)(cInfo->get_if(entity));
	}

	template <typename T, IsAccessType access = psl::ecs::indirect_t>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	inline auto get_component(entity_t entity) const noexcept {
		return get_component<T>(access {}, entity);
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	auto try_get_component(psl::ecs::direct_t, psl::array_view<entity_t> entities) const noexcept -> psl::array<T*> {
		auto cInfo = get_component_typed_info<T>();
		psl::array<T*> result {};
		result.reserve(entities.size());
		for(auto entity : entities) {
			result.emplace_back((T*)(cInfo->get_if(entity)));
		}
		return result;
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	auto try_get_component(psl::ecs::indirect_t, psl::array_view<entity_t> entities) const noexcept -> psl::array<T*> {
		auto cInfo = get_component_typed_info<T>();
		psl::array<T*> result {};
		result.reserve(entities.size());
		for(auto entity : entities) {
			result.emplace_back((T*)(cInfo->get_if(entity)));
		}
		return result;
	}

	template <typename T, IsAccessType access = psl::ecs::indirect_t>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	inline auto try_get_component(psl::array_view<entity_t> entities) const noexcept {
		return try_get_component<T>(access {}, entities);
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	auto try_get_component(psl::ecs::direct_t, entity_t entity) const noexcept -> T* {
		auto cInfo = get_component_typed_info<T>();
		return (T*)(cInfo->get_if(entity));
	}

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	auto try_get_component(psl::ecs::indirect_t, entity_t entity) const noexcept -> T* {
		auto cInfo = get_component_typed_info<T>();
		return (T*)(cInfo->get_if(entity));
	}

	template <typename T, IsAccessType access = psl::ecs::indirect_t>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	inline auto try_get_component(entity_t entity) const noexcept {
		return try_get_component<T>(access {}, entity);
	}

	void clear(bool release_memory = true) noexcept;

	template <typename T>
		requires(!IsFilteringOp<T> && (IsUnrestrictedMutable<T> || std::is_const_v<T>))
	T& get(entity_t entity) {
		// todo this should support filtering
		auto cInfo = get_component_typed_info<T>();
		return cInfo->entity_data().template at<T>(static_cast<entity_t::size_type>(entity),
												   details::stage_range_t::ALL);
	}

	template <typename T>
		requires(!IsFilteringOp<T> && IsUnrestrictedMutable<T>)
	const T& get(entity_t entity) const noexcept {
		// todo this should support filtering
		auto cInfo = get_component_typed_info<T>();
		return cInfo->entity_data().template at<T>(static_cast<entity_t::size_type>(entity),
												   details::stage_range_t::ALL);
	}

	template <typename T>
		requires(!IsFilteringOp<T>)
	psl::array<T> get(psl::array_view<entity_t> entities) const {
		auto cInfo = get_component_typed_info<T>();
		psl::array<T> result {};
		result.resize(entities.size());
		cInfo->copy_to(entities, result.data());
		return result;
	}

	template <typename... Ts>
	bool has_components(psl::array_view<entity_t> entities) const noexcept {
		std::array<psl::ecs::details::component_container_t*, sizeof...(Ts)> cInfos {
		  get_component_untyped_info<Ts>()...};
		if(std::none_of(cInfos.begin(), cInfos.end(), [](auto* info) { return info == nullptr; })) {
			return std::all_of(std::begin(cInfos), std::end(cInfos), [&entities](const auto& cInfo) {
				return cInfo && std::all_of(std::begin(entities), std::end(entities), [&cInfo](entity_t e) {
						   return cInfo->has_component(e);
					   });
			});
		}
		return sizeof...(Ts) == 0;
	}

	[[maybe_unused]] entity_t create() {
		if(m_Orphans.size() > 0) {
			auto entity = m_Orphans.back();
			m_Orphans.pop_back();
			return entity;
		} else {
			return m_Entities++;
		}
	}

	template <typename... Ts>
	[[maybe_unused]] psl::array<entity_t> create(auto count) {
		entity_t::size_type const count_sz = static_cast<entity_t::size_type>(count);
		psl::array<entity_t> entities;
		entities.reserve(count_sz);
		const auto recycled =
		  std::min<entity_t::size_type>(count_sz, static_cast<entity_t::size_type>(m_Orphans.size()));
		const auto remainder = count_sz - recycled;

		std::reverse_copy(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans), std::back_inserter(entities));
		m_Orphans.erase(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans));
		entities.reserve(remainder);
		for(auto i = m_Entities, end = remainder + m_Entities; i < end; ++i) {
			entities.emplace_back(details::make_entity(i));
		}
		m_Entities += remainder;

		if constexpr(sizeof...(Ts) > 0) {
			(add_components<Ts>(entities), ...);
		}
		return entities;
	}

	template <typename... Ts>
	[[maybe_unused]] psl::array<entity_t> create(auto count, Ts&&... prototype) {
		entity_t::size_type const count_sz = static_cast<entity_t::size_type>(count);
		psl::array<entity_t> entities;
		entities.reserve(count_sz);
		const auto recycled =
		  std::min<entity_t::size_type>(count_sz, static_cast<entity_t::size_type>(m_Orphans.size()));
		const auto remainder = count_sz - recycled;

		std::reverse_copy(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans), std::back_inserter(entities));
		m_Orphans.erase(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans));
		entities.reserve(remainder);
		for(auto i = m_Entities, end = remainder + m_Entities; i < end; ++i) {
			entities.emplace_back(details::make_entity(i));
		}
		m_Entities += remainder;

		add_components(entities, std::forward<Ts>(prototype)...);

		return entities;
	}

	void destroy(psl::array_view<entity_t> entities) noexcept;
	void destroy(entity_t entity) noexcept;

	psl::array<entity_t> all_entities() const noexcept {
		auto orphans = m_Orphans;
		auto count	 = orphans.end() - orphans.begin();
		if(count > 0) {
			auto* data = (entity_t::size_type*)orphans.data();
			std::sort(data, data + count);
		}

		auto orphan_it = std::begin(orphans);
		psl::array<entity_t> result;
		result.reserve(m_Entities - orphans.size());
		for(entity_t::size_type e = 0; e < m_Entities; ++e) {
			if(orphan_it != std::end(orphans) && e == static_cast<entity_t::size_type>(*orphan_it)) {
				orphan_it = std::next(orphan_it);
				continue;
			}
			result.emplace_back(details::make_entity(e));
		}
		return result;
	}

	template <typename... Ts>
	psl::array<entity_t> filter() const noexcept {
		auto filter_group = make_filter_group<psl::ecs::pack_direct_full_t<Ts...>>();
		psl_assert(filter_group.size() == 1, "expected only one filter group");

		auto it = std::find_if(std::begin(m_Filters), std::end(m_Filters), [&filter_group](const filter_result& data) {
			return *data.group == filter_group[0];
		});

		filter_result data {};

		if(it == std::end(m_Filters)) {
			data.group = std::make_shared<details::filter_group>(filter_group[0]);
			initialize_filter(data);
		} else {
			data.entities = it->entities;
			data.group	  = it->group;
		}

		psl::array<entity_t> modified {};
		if(data.group->hierarchy_change == hierarchy_change_event::none) {
			modified = psl::array<entity_t> {(entity_t*)m_ModifiedEntities.indices().data(),
											 (entity_t*)m_ModifiedEntities.indices().data() +
											   m_ModifiedEntities.indices().size()};
		} else {
			modified = psl::array<entity_t> {(entity_t*)m_ModifiedHierarchy.indices().data(),
											 (entity_t*)m_ModifiedHierarchy.indices().data() +
											   m_ModifiedHierarchy.indices().size()};
		}
		std::sort(std::begin(modified), std::end(modified));
		filter(data, modified);
		return data.entities;
	}

	template <typename... Ts>
	psl::array<entity_t> filter(psl::array_view<entity_t> entities) const noexcept {
		auto filter_group = make_filter_group<psl::ecs::pack_direct_full_t<Ts...>>();
		psl_assert(filter_group.size() == 1, "expected only one filter group");

		filter_result data {{}, std::make_shared<details::filter_group>(filter_group[0])};
		filter(data, entities);
		return data.entities;
	}

	template <typename... Ts>
	void set_components(psl::array_view<entity_t> entities, psl::array_view<Ts>... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
	}

	template <typename... Ts>
	void set_components(psl::array_view<entity_t> entities, Ts&&... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
	}

	template <typename T>
	void set_component(psl::array_view<entity_t> entities, T&& data) noexcept {
		auto cInfo = get_component_typed_info<T>();
		psl_assert(cInfo != nullptr,
				   "there was no component storage for the given type. You cannot set components for components that "
				   "don't exist in the state.");
		for(auto e : entities) {
			cInfo->set(e, &data);
		}
	}

	template <typename T>
	void set_component(psl::array_view<entity_t> entities, psl::array_view<T> data) noexcept {
		psl_assert(entities.size() == data.size(),
				   "incorrect amount of data input compared to entities, expected {} but got {}",
				   entities.size(),
				   data.size());
		auto cInfo = get_component_typed_info<T>();
		psl_assert(cInfo != nullptr,
				   "there was no component storage for the given type. You cannot set components for components that "
				   "don't exist in the state.");
		auto d = std::begin(data);
		for(auto e : entities) {
			cInfo->set(e, *d);
			d = std::next(d);
		}
	}

	template <typename... Ts>
	void assign_components(psl::array_view<entity_t> entities, psl::array_view<Ts>... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
	}

	template <typename... Ts>
	void assign_components(psl::array_view<entity_t> entities, Ts&&... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
	}

	template <typename T>
	void assign_component(psl::array_view<entity_t> entities, T&& data) noexcept {
		auto cInfo = get_component_typed_info<T>();
		psl_assert(cInfo != nullptr,
				   "there was no component storage for the given type. You cannot set components for components that "
				   "don't exist in the state.");
		for(auto e : entities) {
			if(!cInfo->has_component(e)) {
				throw std::runtime_error(
				  "cannot assign component to entity that does not have the component, use add_component instead");
			}
			cInfo->set(e, &data);
		}
	}

	template <typename T>
	void assign_component(psl::array_view<entity_t> entities, psl::array_view<T> data) noexcept {
		psl_assert(entities.size() == data.size(),
				   "incorrect amount of data input compared to entities, expected {} but got {}",
				   entities.size(),
				   data.size());
		auto cInfo = get_component_typed_info<T>();
		psl_assert(cInfo != nullptr,
				   "there was no component storage for the given type. You cannot set components for components that "
				   "don't exist in the state.");
		auto d = std::begin(data);
		for(auto e : entities) {
			if(!cInfo->has_component(e)) {
				throw std::runtime_error(
				  "cannot assign component to entity that does not have the component, use add_component instead");
			}
			cInfo->set(e, *d);
			d = std::next(d);
		}
	}

	void tick(std::chrono::duration<float> dTime);
	void tick(std::chrono::duration<float> dTime, system_group_t group);
	void tick(std::chrono::duration<float> dTime, psl::array_view<system_group_t> groups);

	void reset(psl::array_view<entity_t> entities) noexcept;

	template <typename T>
	psl::array_view<entity_t> entities() const noexcept {
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

	template <typename T>
		requires(details::IsRangeType<T>)
	void set_parent(entity_t parent, T const& children) noexcept {
		for(auto child : children) {
			state_t::set_parent(parent, child);
		}
	}

	bool has_parent(entity_t target) const noexcept;
	bool has_siblings(entity_t target) const noexcept;
	bool has_children(entity_t target) const noexcept;
	bool is_child_of(entity_t parent, entity_t child) const noexcept;
	bool is_parent_of(entity_t parent, entity_t child) const noexcept;
	bool is_sibling(entity_t first, entity_t second) const noexcept;
	bool is_indirect_parent_of(entity_t parent, entity_t child) const noexcept;
	bool is_root(entity_t target) const noexcept;
	entity_t get_root(entity_t target) const noexcept;
	void set_parent(entity_t parent, entity_t child) noexcept;
	void unparent(entity_t target);
	psl::array<entity_t> get_children(entity_t parent, bool direct_only = false) const noexcept;
	psl::array<entity_t> get_direct_children(entity_t parent) const noexcept;
	psl::array<entity_t> get_all_children(entity_t parent) const noexcept;
	psl::array<entity_t> get_all_parents(entity_t child) const noexcept;
	entity_t get_parent(entity_t child) const noexcept;
	psl::array<entity_t> get_siblings(entity_t target) const noexcept;

	/// \brief returns the amount of active systems
	size_t systems() const noexcept {
		return m_SystemInformations.size() - m_ToRevoke.size();
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn>
		requires(!std::is_same_v<Fn, transient_system_tag_t>)
	auto declare(Fn&& fn, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(
		  threading::sequential, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, std::nullopt);
	}


	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto
	declare(threading threading, Fn&& fn, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(threading, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, std::nullopt);
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
		requires(!std::is_same_v<Fn, transient_system_tag_t>)
	auto declare(Fn&& fn, T* ptr, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(threading::sequential, std::forward<Fn>(fn), ptr, DebugName, systemGroup, std::nullopt);
	}
	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(threading threading, Fn&& fn, T* ptr, std::optional<system_group_t> systemGroup = std::nullopt)
	  -> system_token {
		return declare_impl(threading, std::forward<Fn>(fn), ptr, DebugName, systemGroup, std::nullopt);
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto
	declare(transient_system_tag_t, Fn&& fn, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(
		  threading::sequential, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, transient_system_tag);
	}


	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto declare(transient_system_tag_t,
				 threading threading,
				 Fn&& fn,
				 std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(
		  threading, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, transient_system_tag);
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(transient_system_tag_t, Fn&& fn, T* ptr, std::optional<system_group_t> systemGroup = std::nullopt)
	  -> system_token {
		return declare_impl(
		  threading::sequential, std::forward<Fn>(fn), ptr, DebugName, systemGroup, transient_system_tag);
	}
	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(transient_system_tag_t,
				 threading threading,
				 Fn&& fn,
				 T* ptr,
				 std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(threading, std::forward<Fn>(fn), ptr, DebugName, systemGroup, transient_system_tag);
	}

	template <psl::details::fixed_astring DebugName = "">
	[[nodiscard]] auto create_system_group() noexcept {
		m_SystemGroups.emplace(m_SystemGroupCounter, psl::array<details::system_token> {});
		auto group = system_group_t {m_SystemGroupCounter, DebugName};
		++m_SystemGroupCounter;
		return group;
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

				for(auto& group : m_SystemGroups) {
					group.second.erase(std::remove_if(std::begin(group.second),
													  std::end(group.second),
													  [&id](const auto& token) { return token == id; }),
									   std::end(group.second));
				}
				return true;
			}
			return false;
		}
	}

	size_t capacity() const noexcept {
		return m_Entities;
	}

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
	template <IsPack... Ts>
	auto make_filter_group() const noexcept -> psl::array<details::filter_group> {
		psl::array<details::filter_group> result {};
		result.reserve(sizeof...(Ts));
		(
		  [&result, this]<typename... Ys>(psl::utility::templates::type_pack_t<Ys...>) mutable {
			  result.emplace_back(make_filter_group<Ys...>());
		  }(decode_pack_types_t<Ts> {}),
		  ...);
		return result;
	}

	template <IsPack... Ts>
	auto make_filter_group(psl::type_pack_t<Ts...>) const noexcept -> psl::array<details::filter_group> {
		psl::array<details::filter_group> result {};
		result.reserve(sizeof...(Ts));
		(
		  [&result, this]<typename... Ys>(psl::utility::templates::type_pack_t<Ys...>) mutable {
			  result.emplace_back(make_filter_group<Ys...>());
		  }(decode_pack_types_t<Ts> {}),
		  ...);
		return result;
	}

	template <typename... Ts>
	details::filter_group make_filter_group() const noexcept {
		return details::filter_group {
		  psl::type_pack_t<Ts...> {},
		  [this]<typename T>() -> details::component_container_t* { return get_component_untyped_info<T>(); }};
	}

	size_t prepare_bindings(psl::array_view<entity_t> entities,
							void* cache,
							details::dependency_pack& dep_pack) const noexcept;
	size_t prepare_data(psl::array_view<entity_t> entities, void* cache, details::component_key_t id) const noexcept;

	void prepare_system(std::chrono::duration<float> dTime,
						std::chrono::duration<float> rTime,
						std::uintptr_t cache_offset,
						details::system_information& information);


	void execute_command_buffer(info_t& info);

	template <typename T>
	inline void create_storage() const noexcept {
		constexpr auto key = details::component_key_t::generate<T>();
		using target_type =
		  std::conditional_t<details::IsMutateInstruction<T>, details::mutate_instruction_underlying_t<T>, T>;

		if(auto cInfo = get_component_typed_info<T>(); !cInfo) {
			m_Components.emplace(key, details::instantiate_component_container<target_type>());
			get_component_typed_info<T>();
		}
	}

	details::component_container_t* get_component_container(const details::component_key_t& key) noexcept;
	details::component_container_t* get_component_container(const details::component_key_t& key) const noexcept;

	psl::array<const details::component_container_t*>
	get_component_container(psl::array_view<details::component_key_t> keys) const noexcept;
	psl::array<details::component_container_t*>
	get_component_container(psl::array_view<details::cached_container_entry_t> entries) const noexcept;
	template <typename T>
	inline auto get_component_typed_info() const noexcept -> psl::ecs::details::component_container_type_for_t<T>* {
		return details::cast_component_container<T>(get_component_untyped_info<T>());
	}

	template <typename T>
	auto handle_version_migration(psl::ecs::details::component_container_t* container) const noexcept
	  -> psl::ecs::details::component_container_t* {
		auto const& cti = container->component_type_info();
		if(!cti.serializable) {
			return container;
		}

		if constexpr(component_trait_version_t<T>::version != 0) {
			// todo(jdl): this should be possible to support, we'll have to recreate the container to achieve it though.
			psl_assert(
			  !IsComponentComplexType<T>,
			  "component used to be a trivial type, is now a complex type, we don't support this migration yet");

			static constexpr auto compiled_version = component_trait_version_t<T>::version;

			if(cti.version != compiled_version) {
				// have to make a new container
				auto new_container = psl::ecs::details::instantiate_component_container<T>();
				new_container->reserve(container->size(true));

				auto removed_entities = container->removed_entities();
				auto added_entities	  = container->added_entities();
				auto stable_entities  = container->entities(details::stage_range_t::SETTLED);

				auto fn = [&new_container, &container, &cti](auto entities, std::byte* data) {
					for(auto entity : entities) {
						auto temp {psl::ecs::component_updater_t<T> {}(cti.version, (void*)data)};
						new_container->add(entity, &temp);
						data += cti.size;
					}
				};

				// todo(jdl): this can be more performant. We could merge internally within the component containers
				// sidestepping this whole `add` behaviour.
				fn(removed_entities,
				   (std::byte*)container->data() + ((added_entities.size() + stable_entities.size()) * cti.size));
				fn(stable_entities, (std::byte*)container->data());
				new_container->purge();
				new_container->destroy(removed_entities);
				fn(added_entities, (std::byte*)container->data() + (stable_entities.size() * cti.size));

				// next operation will kill the cti variable, so we cache the key
				auto key			 = cti.id;
				m_Components[cti.id] = std::move(new_container);

				return m_Components[key].get();
			}
		} else {
			psl_assert(
			  cti.version == 0,
			  "The serialized version appears to be a higher version than the component indicates it supports");
		}
		return container;
	}

	template <typename T>
	inline auto get_component_untyped_info() const noexcept -> psl::ecs::details::component_container_t* {
		constexpr auto key {details::component_key_t::generate<T>()};
#if !defined(PE_ECS_DISABLE_LOOKUP_CACHE)
		static thread_local size_t generation {0};
		static thread_local size_t state_unique_key {0};
		static thread_local psl::ecs::details::component_container_t* container = nullptr;
		static thread_local state_t const* owner {nullptr};
		if(this != owner || generation != m_ComponentGeneration || container == nullptr ||
		   state_unique_key != m_StateUniqueKey) {
			auto it	   = m_Components.find(key);
			container  = nullptr;
			owner	   = this;
			generation = m_ComponentGeneration;
			if(it == std::end(m_Components)) {
				return nullptr;
			}
			container		 = handle_version_migration<T>(it->second.get());
			state_unique_key = m_StateUniqueKey;
		}
		return container;
#else
		if(auto it = m_Components.find(key); it != std::end(m_Components)) {
			return handle_version_migration<T>(it->second.get());
		}
		return nullptr;
#endif
	}
	//------------------------------------------------------------
	// add_component
	//------------------------------------------------------------

	template <typename T>
	void add_component(psl::array_view<entity_t> entities, auto&& prototype) {
		using behavior_t = details::decode_add_component_behaviour_t<T, std::remove_cvref_t<decltype(prototype)>>;
		static constexpr auto mode = behavior_t::mode;
		using type				   = typename behavior_t::type;
		using underlying_t		   = typename behavior_t::underlying_t;

		if constexpr(mode == details::add_component_behaviour_mode_t::empty_container) {
			create_storage<type>();
			if constexpr(details::DoesComponentTypeNeedPrototypeCall<underlying_t>) {
				underlying_t v {details::prototype_for<underlying_t>()};
				add_component_impl(get_component_untyped_info<type>(), entities, &v);
			} else {
				add_component_impl(get_component_untyped_info<type>(), entities);
			}
		} else if constexpr(mode == details::add_component_behaviour_mode_t::callable_1) {
			create_storage<type>();
			add_component_impl(
			  get_component_untyped_info<type>(), entities, [prototype](std::uintptr_t location, size_t count) {
				  for(auto i = size_t {0}; i < count; ++i) {
					  std::invoke(prototype, *((underlying_t*)(location) + i));
				  }
			  });
		} else if constexpr(mode == details::add_component_behaviour_mode_t::callable_2) {
			create_storage<type>();
			add_component_impl(get_component_untyped_info<type>(),
							   entities,
							   [prototype, &entities](std::uintptr_t location, size_t count) {
								   for(auto i = size_t {0}; i < count; ++i) {
									   std::invoke(prototype, *((underlying_t*)(location) + i), entities[i]);
								   }
							   });
		} else if constexpr(mode == details::add_component_behaviour_mode_t::range) {
			psl_assert(entities.size() == prototype.size(),
					   "incorrect amount of data input compared to entities, expected {} but got {}",
					   entities.size(),
					   prototype.size());
			create_storage<type>();
			add_component_impl(get_component_untyped_info<type>(), entities, prototype.data(), false);
		} else if constexpr(mode == details::add_component_behaviour_mode_t::standard_layout) {
			create_storage<type>();
			add_component_impl(get_component_untyped_info<type>(), entities, &prototype);
		}
	}

	template <typename T>
	void add_component(psl::array_view<entity_t> entities) {
		add_component<T>(entities, empty<T> {});
	}


	void add_component_impl(const details::component_key_t& key, psl::array_view<entity_t> entities);
	void add_component_impl(details::component_container_t* cInfo, psl::array_view<entity_t> entities);

	// invocable based construction
	template <typename Fn>
		requires(std::is_invocable<Fn, std::uintptr_t, size_t>::value)
	void add_component_impl(details::component_container_t* cInfo, psl::array_view<entity_t> entities, Fn&& invocable) {
		psl_assert(cInfo != nullptr, "component info for key {} was not found", cInfo->component_type_info().id);
		const auto component_size = cInfo->component_type_info().size;
		psl_assert(component_size != 0, "component size was 0");

		auto offset = cInfo->entities().size();
		cInfo->add(entities);

		auto location = (std::uintptr_t)cInfo->data() + (offset * component_size);
		std::invoke(invocable, location, entities.size());
		for(size_t i = 0; i < entities.size(); ++i)
			m_ModifiedEntities.try_insert(static_cast<entity_t::size_type>(entities[i]));
	}

	void add_component_impl(const details::component_key_t& key,
							psl::array_view<entity_t> entities,
							void* prototype,
							bool repeat = true);
	void add_component_impl(details::component_container_t* cInfo,
							psl::array_view<entity_t> entities,
							void* prototype,
							bool repeat = true);

	//------------------------------------------------------------
	// remove_component
	//------------------------------------------------------------
	void remove_component(const details::component_key_t& key, psl::array_view<entity_t> entities) noexcept;
	void remove_component(details::component_container_t* cInfo, psl::array_view<entity_t> entities) noexcept;


	//------------------------------------------------------------
	// filter
	//------------------------------------------------------------
	template <typename T>
	psl::array<entity_t>::iterator filter_op(psl::type_pack_t<T>,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept {
		return filter_op(get_component_untyped_info<T>(), begin, end);
	}

	template <typename T>
	psl::array<entity_t>::iterator filter_op(psl::type_pack_t<psl::ecs::filter<T>>,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept {
		return filter_op(get_component_untyped_info<T>(), begin, end);
	}
	template <typename T>
	psl::array<entity_t>::iterator filter_op(psl::type_pack_t<psl::ecs::on_add<T>>,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept {
		return on_add_op(get_component_untyped_info<T>(), begin, end);
	}
	template <typename T>
	psl::array<entity_t>::iterator filter_op(psl::type_pack_t<psl::ecs::on_remove<T>>,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept {
		return on_remove_op(get_component_untyped_info<T>(), begin, end);
	}
	template <typename T>
	psl::array<entity_t>::iterator filter_op(psl::type_pack_t<psl::ecs::except<T>>,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept {
		return on_except_op(get_component_untyped_info<T>(), begin, end);
	}
	template <typename... Ts>
	psl::array<entity_t>::iterator filter_op(psl::type_pack_t<psl::ecs::on_break<Ts...>>,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept {
		psl::array<details::cached_container_entry_t> entries {get_component_untyped_info<Ts>()...};
		return on_break_op(entries, begin, end);
	}

	template <typename... Ts>
	psl::array<entity_t>::iterator filter_op(psl::type_pack_t<psl::ecs::on_combine<Ts...>>,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept {
		psl::array<details::cached_container_entry_t> entries {get_component_untyped_info<Ts>()...};
		return on_combine_op(entries, begin, end);
	}

	psl::array<entity_t>::iterator filter_op(details::cached_container_entry_t const& entry,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_add_op(details::cached_container_entry_t const& entry,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_remove_op(details::cached_container_entry_t const& entry,
												psl::array<entity_t>::iterator begin,
												psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_except_op(details::cached_container_entry_t const& entry,
												psl::array<entity_t>::iterator begin,
												psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_break_op(psl::array<details::cached_container_entry_t> const& entries,
											   psl::array<entity_t>::iterator begin,
											   psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_combine_op(psl::array<details::cached_container_entry_t> const& entries,
												 psl::array<entity_t>::iterator begin,
												 psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_mutate_op(details::cached_container_entry_t const& entry,
												psl::array<entity_t>::iterator begin,
												psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_hierarchy_op(hierarchy_change_event event,
												   psl::array<entity_t>::iterator begin,
												   psl::array<entity_t>::iterator end) const noexcept;
	psl::array<entity_t>::iterator on_hierarchy_with_preseed_op(hierarchy_change_event event,
																psl::array<entity_t>::iterator begin,
																psl::array<entity_t>::iterator end) const noexcept;

	void filter(filter_result& data, psl::array_view<entity_t> source) const noexcept;
	psl::array<entity_t>::iterator filter(details::filter_group const& group,
										  psl::array<entity_t>::iterator begin,
										  psl::array<entity_t>::iterator end) const noexcept;

	// returns the smallest set of entities that is in the filter
	psl::array_view<entity_t> get_source_for(filter_result const& data) const noexcept;
	void initialize_filter(filter_result& data) const noexcept;

	psl::array<entity_t> get_all_relationships_unfiltered(psl::array_view<entity_t> source,
														  entity_relationship relationship) const noexcept;


	//------------------------------------------------------------
	// transformations
	//------------------------------------------------------------

	void transform(transform_result& data, psl::array_view<entity_t> source) const noexcept;
	void transform(transform_result& data) const noexcept;


	template <typename... Ts>
	psl::array<details::component_key_t> to_keys() const noexcept {
		return psl::array<details::component_key_t> {details::component_key_t::generate<Ts>()...};
	}


	//------------------------------------------------------------
	// set
	//------------------------------------------------------------
	size_t set(psl::array_view<entity_t> entities, const details::component_key_t& key, void* data) noexcept;

	psl::array<details::component_container_t*> apply_mutations();

	//------------------------------------------------------------
	// system declare
	//------------------------------------------------------------

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
			initialize_filter(*filter_it);
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

	template <typename Fn, typename T, typename pack_type>
	auto create_system_tick_functional(Fn& fn, T* ptr) const noexcept {
		if constexpr(std::is_member_function_pointer<Fn>::value) {
			return [fn, ptr](psl::ecs::info_t& info, psl::array<details::dependency_pack> packs) -> void {
				auto tuple_argument_list = std::tuple_cat(std::tuple<T*, psl::ecs::info_t&>(ptr, info),
														  details::compress_from_dependency_pack(pack_type {}, packs));

				std::apply(fn, std::move(tuple_argument_list));
			};
		} else {
			return [fn](psl::ecs::info_t& info, psl::array<details::dependency_pack> packs) -> void {
				auto tuple_argument_list = std::tuple_cat(std::tuple<psl::ecs::info_t&>(info),
														  details::compress_from_dependency_pack(pack_type {}, packs));

				std::apply(fn, std::move(tuple_argument_list));
			};
		}
	}

	template <typename Fn, typename T = void>
	auto declare_impl(threading threading,
					  Fn&& fn,
					  T* ptr,
					  psl::string_view debugName					  = "",
					  std::optional<system_group_t> systemGroup		  = std::nullopt,
					  std::optional<transient_system_tag_t> transient = std::nullopt) {
		using function_args	  = typename psl::templates::func_traits<typename std::decay<Fn>::type>::arguments_t;
		using pack_type		  = typename details::get_packs<function_args>::type;
		auto filter_groups	  = make_filter_group(pack_type {});
		auto transform_groups = []<typename... Ts>(psl::type_pack_t<Ts...>) -> psl::array<details::transform_group> {
			return psl::array<details::transform_group> {details::transform_group(decode_pack_types_t<Ts> {})...};
		}(pack_type {});
		auto pack_generator = [this]() {
			return details::expand_to_dependency_pack(
			  pack_type {},
			  [this]<typename Z>() -> details::component_container_t* { return get_component_untyped_info<Z>(); });
		};

		// make sure systems don't have any non-basic filter operations if they are part of a system group
		// the reason for this is that the tracking for component lifetime events is not done on a per system level
		// but handled by the storage of the components themselves. This means that if a group is ticked on a different
		// cadence than every tick, the system would miss out on component lifetime events.
		if(systemGroup != std::nullopt) {
			for(auto const& filter_group : filter_groups) {
				if(!filter_group.is_basic_filter()) {
					throw std::runtime_error(
					  "system groups can only contain basic filter operations, no on_add, on_break, on_combine, or "
					  "on_remove");
				}
			}
		}

		auto system_tick = create_system_tick_functional<Fn, T, pack_type>(fn, ptr);
		auto& sys_info	 = (m_LockState) ? m_NewSystemInformations : m_SystemInformations;

		psl::array<std::shared_ptr<details::filter_group>> shared_filter_groups;
		psl::array<std::shared_ptr<details::transform_group>> shared_transform_groups;

		for(size_t i = 0; i < filter_groups.size(); ++i) {
			auto [shared_filter, transform_filter] = add_filter_group(filter_groups[i], transform_groups[i], debugName);
			shared_filter_groups.emplace_back(shared_filter);
			shared_transform_groups.emplace_back(transform_filter);
		}


		auto system_id = sys_info
						   .emplace_back(threading,
										 std::move(pack_generator),
										 std::move(system_tick),
										 shared_filter_groups,
										 shared_transform_groups,
										 ++m_SystemCounter,
										 debugName)
						   .id();

		if(systemGroup != std::nullopt) {
			auto it = m_SystemGroups.find(systemGroup->m_Id);
			psl_assert(it != std::end(m_SystemGroups),
					   "system group not found, are you sure you registered it with this container? Do note that "
					   "groups do not persist a reset.");

			m_SystemGroupIndices.emplace(system_id);
			it->second.push_back(system_id);
		}

		m_SystemGroups[0].push_back(system_id);	   // always add to the default group
		if(transient.has_value()) {
			m_ToRevoke.push_back(system_id);
		}
		return system_id;
	}

	::memory::raw_region m_Cache {1024 * 1024 * 256};
	psl::array<psl::unique_ptr<info_t>> info_buffer {};
	psl::array<entity_t> m_Orphans {};
	psl::array<entity_t> m_ToBeOrphans {};
	mutable psl::array<filter_result> m_Filters {};
	psl::array<details::system_information> m_SystemInformations {};

	psl::array<details::system_token> m_ToRevoke {};
	psl::array<details::system_information> m_NewSystemInformations {};
	std::unordered_map<size_t, psl::array<details::system_token>> m_SystemGroups {};
	std::unordered_set<details::system_token> m_SystemGroupIndices {};
	size_t m_SystemGroupCounter {1};

	mutable std::unordered_map<details::component_key_t, std::unique_ptr<details::component_container_t>>
	  m_Components {};

	psl::sparse_indice_array<entity_t::size_type> m_ModifiedEntities {};
	psl::sparse_array<hierarchy_change_event, entity_t::size_type> m_ModifiedHierarchy {};

	psl::sparse_array<entity_relationship_t, entity_t::size_type> m_ParentRelationship {};

	psl::unique_ptr<psl::async::scheduler> m_Scheduler {nullptr};

	size_t m_LockState {0};
	size_t m_Tick {0};
	size_t m_SystemCounter {0};
	entity_t::size_type m_Entities {0};
	entity_t::size_type m_MinEntitiesPerWorker {1024};
#if !defined(PE_ECS_DISABLE_LOOKUP_CACHE)
	// Used by the local cache to improve lookup speed. Every time the state get's cleared this is incremented so the
	// cache can be regenerated.
	std::atomic<size_t> m_ComponentGeneration {1};
	// used by the local cache to improve lookup speed. Every instance of state increments a global that is used to
	// distinguish that instance. This is to protect ourselves from the (rare) occassion a state_t gets deleted and
	// recreated on the same memory location, which would result in the cache not correctly getting rejected.
	std::atomic<size_t> m_StateUniqueKey {0};
#endif
};
}	 // namespace psl::ecs
