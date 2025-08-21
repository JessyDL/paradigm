#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <psl/array.hpp>
#include <psl/assertions.hpp>
#include <psl/collections/indirect_array.hpp>
#include <psl/ecs/command_buffer.hpp>
#include <psl/ecs/details/component_container.hpp>
#include <psl/ecs/details/component_key.hpp>
#include <psl/ecs/details/selectors.hpp>
#include <psl/ecs/details/system_information.hpp>
#include <psl/ecs/entity.hpp>
#include <psl/ecs/selectors.hpp>
#include <psl/serialization/serializer.hpp>
#include <psl/sparse_array.hpp>
#include <unordered_map>

#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
	#include <bitset>
#endif

namespace psl::ecs::details {
class components_cache_t {
  protected:
	components_cache_t();
	components_cache_t(const components_cache_t&)			 = delete;
	components_cache_t(components_cache_t&&)				 = delete;
	components_cache_t& operator=(const components_cache_t&) = delete;
	components_cache_t& operator=(components_cache_t&&)		 = delete;

	template <typename S>
	void serialize(S& serializer) {
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

  public:
	template <IsComponentTypeSerializable T>
	void override_serialization(bool value) {
		if(auto* ptr = create_storage<T>(); ptr != nullptr) {
			ptr->m_Info.serializable = value;
		} else {
			throw std::runtime_error("Cannot override serialization for a component that does not exist.");
		}
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

	template <typename T>
	psl::array_view<entity_t> entities() const noexcept {
		constexpr auto key {details::component_key_t::generate<T>()};
		if(auto ptr = get_component_container(key); ptr) {
			return ptr->entities();
		}
		return {};
	}

	template <typename T>
	psl::array_view<T const> view() const noexcept {
		constexpr auto key {details::component_key_t::generate<T>()};
		if(auto ptr = get_component_container(key); ptr) {
			return (details::cast_component_container<std::remove_const_t<T>>(ptr))
			  ->entity_data()
			  .template dense<std::remove_const_t<T>>(details::stage_range_t::ALIVE);
		}
		return {};
	}

  protected:
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

	auto get_component_container(const component_key_t& key) const -> details::component_container_t* {
		auto it = m_Components.find(key);
		if(it != m_Components.end()) {
			return it->second.get();
		}
		return nullptr;
	}

	template <typename T>
	auto get_component_container() const -> details::component_container_t* {
		constexpr auto key = details::component_key_t::generate<T>();
		return get_component_container(key);
	}
	void execute_command_buffer(info_t& info,
								psl::sparse_array<entity_t::size_type, entity_t::size_type> remapped_entities);
	void clear(bool release_memory = false);
	void purge() noexcept;

	psl::array<details::component_container_t*> apply_mutations();
	size_t
	component_copy_from(psl::array_view<entity_t> entities, const details::component_key_t& key, void* data) noexcept;


	template <typename Ts>
	size_t size() const noexcept {
		constexpr auto key = details::component_key_t::generate<Ts>();
		auto cInfo		   = get_component_container(key);
		return cInfo ? cInfo->size() : 0;
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

	//------------------------------------------------------------
	// add_component_impl
	//------------------------------------------------------------

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

	template <typename... Ts>
	void remove_components(psl::array_view<entity_t> entities) noexcept {
		(remove_component(get_component_untyped_info<Ts>(), entities), ...);
	}

	void remove_component(const details::component_key_t& key, psl::array_view<entity_t> entities) noexcept;
	void remove_component(details::component_container_t* cInfo, psl::array_view<entity_t> entities) noexcept;

	//------------------------------------------------------------
	// destroy_components
	//------------------------------------------------------------

	void destroy_components(psl::array_view<entity_t> entities) noexcept;
	void destroy_components(entity_t entity) noexcept;

  protected:
	template <typename T>
	auto handle_version_migration(psl::ecs::details::component_container_t* container) const noexcept
	  -> psl::ecs::details::component_container_t* {
		auto const& cti = container->component_type_info();
		if(!cti.serializable) {
			return container;
		}

		if constexpr(component_trait_version_t<T>::version != 0) {
			// todo(jdl): this should be possible to support, we'll have to recreate the container to achieve it
			// though.
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

				// todo(jdl): this can be more performant. We could merge internally within the component
				// containers sidestepping this whole `add` behaviour.
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
	inline auto get_component_typed_info() const noexcept -> psl::ecs::details::component_container_type_for_t<T>* {
		return details::cast_component_container<T>(get_component_untyped_info<T>());
	}

	template <typename T>
	inline auto get_component_untyped_info() const noexcept -> psl::ecs::details::component_container_t* {
		constexpr auto key {details::component_key_t::generate<T>()};
#if !defined(PE_ECS_DISABLE_LOOKUP_CACHE)
		static thread_local size_t generation {0};
		static thread_local size_t state_unique_key {0};
		static thread_local psl::ecs::details::component_container_t* container = nullptr;
		static thread_local components_cache_t const* owner {nullptr};
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

	template <typename T>
	inline auto create_storage() const noexcept -> details::component_container_t* {
		constexpr auto key = details::component_key_t::generate<T>();
		using target_type =
		  std::conditional_t<details::IsMutateInstruction<T>, details::mutate_instruction_underlying_t<T>, T>;

		if(auto cInfo = get_component_typed_info<T>(); !cInfo) {
			m_Components.emplace(key, details::instantiate_component_container<target_type>());
#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
			m_ComponentFlags.emplace(key, m_NextComponentFlagIndex);
			m_ComponentLookup.emplace(m_NextComponentFlagIndex, key);
			m_ComponentLookupArray[m_NextComponentFlagIndex] = m_Components[key].get();
			m_NextComponentFlag								 = m_NextComponentFlag << 1;
			m_NextComponentFlagIndex += 1;
#endif
			return get_component_typed_info<T>();
		} else {
			return cInfo;
		}
	}

  private:
	mutable std::unordered_map<details::component_key_t, std::unique_ptr<details::component_container_t>>
	  m_Components {};
#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
	mutable std::unordered_map<details::component_key_t, size_t> m_ComponentFlags {};
	mutable std::unordered_map<size_t, details::component_key_t> m_ComponentLookup {};
	mutable std::array<details::component_container_t*, 256> m_ComponentLookupArray {};
	psl::sparse_array<std::bitset<256>, entity_t::size_type> m_EntityFlags {};
	mutable std::bitset<256> m_NextComponentFlag {1};
	mutable size_t m_NextComponentFlagIndex {0};
#endif

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
}	 // namespace psl::ecs::details
