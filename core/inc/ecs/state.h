#pragma once
#include "stdafx.h"
#include "entity.h"
#include "range.h"
#include "selectors.h"
#include "pack.h"
#include "memory/raw_region.h"
#include "IDGenerator.h"
#include <type_traits>
#include "event.h"
#include "bytell_hash_map.hpp"

/// \brief Entity Component System
///
/// The ECS namespace contains a fully functioning ECS
namespace core::ecs
{

	template <typename T>
	class tag
	{};

	struct all
	{};

	struct par
	{};

	struct tick
	{};
	struct pre_tick
	{};
	struct post_tick
	{};

	class state;
	class commands;


	namespace details
	{
		template <typename KeyT, typename ValueT>
		//using key_value_container_t = ska::bytell_hash_map<KeyT, ValueT>;
		using key_value_container_t = std::unordered_map<KeyT, ValueT>;

		template <typename T>
		struct is_tag : std::false_type
		{
			using type = T;
		};

		template <typename T>
		struct is_tag<core::ecs::tag<T>> : std::true_type
		{
			using type = T;
		};

		// added to trick the compiler to not throw away the results at compile time
		template <typename T>
		constexpr const std::uintptr_t component_key_var{0u};

		template <typename T>
		constexpr const std::uintptr_t* component_key() noexcept
		{
			return &component_key_var<T>;
		}

		template <typename T>
		using remove_all =
			typename std::remove_pointer<typename std::remove_reference<typename std::remove_cv<T>::type>::type>::type;

		using component_key_t = const std::uintptr_t* (*)();


		struct component_info
		{
			component_info() = default;
			component_info(memory::raw_region&& region, std::vector<entity>&& entities, component_key_t id, size_t size)
				: region(std::move(region)), entities(std::move(entities)), id(id), size(size),
				  generator(region.size() / size){};

			component_info(std::vector<entity>&& entities, component_key_t id)
				: region(128), entities(std::move(entities)), id(id), size(1), generator(1){};
			memory::raw_region region;
			std::vector<entity> entities;
			component_key_t id;
			size_t size;
			psl::IDGenerator<uint64_t> generator;
		};

		template <typename T, typename SFINEA = void>
		struct mf_pre_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `pre_tick` listener.
		template <typename T>
		struct mf_pre_tick<T, std::void_t<decltype(std::declval<T&>().pre_tick(std::declval<core::ecs::commands&>()))>>
			: std::true_type
		{};

		template <typename T, typename SFINEA = void>
		struct mf_post_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `post_tick` listener.
		template <typename T>
		struct mf_post_tick<T, std::void_t<decltype(std::declval<T&>().post_tick(std::declval<core::ecs::commands&>()))>>
			: std::true_type
		{};


		template <typename T, typename SFINEA = void>
		struct mf_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `tick` listener.
		template <typename T>
		struct mf_tick<T, std::void_t<decltype(std::declval<T&>().tick(std::declval<core::ecs::commands&>(),
																	   std::declval<std::chrono::duration<float>>(),
																	   std::declval<std::chrono::duration<float>>()))>>
			: std::true_type
		{};


		template <typename T, typename SFINEA = void>
		struct get_component_type
		{
			using type = typename std::invoke_result<T, size_t>::type;
		};
		template <typename T>
		struct get_component_type<T, typename std::enable_if<!std::is_invocable<T, size_t>::value>::type>
		{
			using type = typename core::ecs::details::is_tag<T>::type;
		};

		template <typename T, typename SFINEA = void>
		struct get_forward_type
		{
			using type = T;
		};
		template <typename T>
		struct get_forward_type<T, typename std::enable_if<!std::is_invocable<T, size_t>::value>::type>
		{
			using type = typename core::ecs::details::is_tag<T>::type;
		};


		template <typename T>
		typename std::enable_if<!std::is_invocable<T, size_t>::value>::type
			initialize_component(void* location, psl::array_view<size_t> indices, T&& data) noexcept
		{
			core::profiler.scope();
				for(auto i = 0; i < indices.size(); ++i)
				{
					std::memcpy((void*)((std::uintptr_t)location + indices[i] * sizeof(T)), &data, sizeof(T));
				}
		}

		template <typename T>
		typename std::enable_if<std::is_invocable<T, size_t>::value>::type
			initialize_component(void* location, psl::array_view<size_t> indices, T&& invokable) noexcept
		{
			constexpr size_t size = sizeof(typename std::invoke_result<T, size_t>::type);
			core::profiler.scope();
				for(auto i = 0; i < indices.size(); ++i)
				{
					auto v{std::invoke(invokable, i)};
					std::memcpy((void*)((std::uintptr_t)location + indices[i] * size), &v, size);
				}
		}

		template <typename T>
		std::vector<entity> duplicate_component_check(psl::array_view<entity> entities, const details::key_value_container_t<entity, std::vector<std::pair<details::component_key_t, size_t>>>& entityMap)
		{
			core::profiler.scope_begin("duplicate_check");
			constexpr details::component_key_t key = details::component_key<details::remove_all<T>>;
			std::vector<entity> ent_cpy = entities;
			auto end = std::remove_if(std::begin(ent_cpy), std::end(ent_cpy), [&entityMap, key](const entity& e) {
				auto eMapIt = entityMap.find(e);
				if (eMapIt == std::end(entityMap))
				{
					return true;
				}

				for(auto eComp : eMapIt->second)
				{
					if(eComp.first == key) return true;
				}
				return false;
			});
			core::profiler.scope_end();
			ent_cpy.resize(std::distance(std::begin(ent_cpy), end));
			return ent_cpy;
		}



		static component_info& get_component_info(details::component_key_t key, size_t component_size, details::key_value_container_t<details::component_key_t, details::component_info>& components)
		{
			auto it = components.find(key);
			if(it == components.end())
			{
				core::profiler.scope_begin("create backing storage");
				if (component_size == 1)
				{
					components.emplace(key, details::component_info{{}, key});
				}
				else
				{
					components.emplace(
						key, details::component_info{
							memory::raw_region{1024 * 1024 * 128}, {}, key, component_size});
				}
				it = components.find(key);
				core::profiler.scope_end();
			}
			return it->second;
		}

		template<typename T>
		static component_info& get_component_info(details::key_value_container_t<details::component_key_t, details::component_info>& components)
		{
			constexpr auto key = details::component_key<details::remove_all<T>>;
			auto it = components.find(key);
			if(it == components.end())
			{
				core::profiler.scope_begin("create backing storage");
				if (std::is_empty<T>::value)
				{
					components.emplace(key, details::component_info{{}, key});
				}
				else
				{
					components.emplace(
						key, details::component_info{
							memory::raw_region{1024 * 1024 * 128}, {}, key, sizeof(T)});
				}
				it = components.find(key);
				core::profiler.scope_end();
			}
			return it->second;
		}

		template <typename T>
		void add_component(
			details::key_value_container_t<entity, std::vector<std::pair<details::component_key_t, size_t>>>& entityMap,
			details::key_value_container_t<details::component_key_t, details::component_info>& components,
			psl::array_view<entity> entities, T&& _template,
			std::optional<std::reference_wrapper<details::key_value_container_t<details::component_key_t, std::vector<entity>>>> addedComponents = std::nullopt) noexcept
		{
			using component_type = typename get_component_type<T>::type;
			using forward_type   = typename get_forward_type<T>::type;


			core::profiler.scope();
				static_assert(
					std::is_trivially_copyable<component_type>::value && std::is_standard_layout<component_type>::value,
					"the component type must be trivially copyable and standard layout "
					"(std::is_trivially_copyable<T>::value == true && std::is_standard_layout<T>::value == true)");
			constexpr details::component_key_t key = details::component_key<details::remove_all<component_type>>;

			std::vector<entity> ent_cpy = duplicate_component_check<component_type>(entities, entityMap);
			std::sort(std::begin(ent_cpy), std::end(ent_cpy));

			auto& componentInfo{get_component_info<component_type>(components)};

			entityMap.reserve(entityMap.size() + std::distance(std::begin(ent_cpy), std::end(ent_cpy)));

			if constexpr(std::is_empty<component_type>::value)
			{
				core::profiler.scope_begin("emplace empty components");
				for(auto ent_it = std::begin(ent_cpy); ent_it != std::end(ent_cpy); ++ent_it)
				{
					const entity& e{*ent_it};
					entityMap[e].emplace_back(key, 0);
					componentInfo.entities.emplace(std::upper_bound(std::begin(componentInfo.entities), std::end(componentInfo.entities), e), e);
				}
				core::profiler.scope_end();
			}
			else
			{
				core::profiler.scope_begin("emplace components");
				std::vector<uint64_t> indices;
				uint64_t id_range;
				const auto count = std::distance(std::begin(ent_cpy), std::end(ent_cpy));
				core::profiler.scope_begin("reserve");
				indices.reserve(count);
				entityMap.reserve(entityMap.size() + count);


				std::vector<entity> merged;
				merged.reserve(componentInfo.entities.size() + count);
				std::merge(std::begin(componentInfo.entities), std::end(componentInfo.entities), std::begin(ent_cpy), std::end(ent_cpy),
					std::back_inserter(merged));
				componentInfo.entities = std::move(merged);

				core::profiler.scope_end();

				if(componentInfo.generator.CreateRangeID(id_range, count))
				{
					core::profiler.scope_begin("fast path");
					for(auto ent_it = std::begin(ent_cpy); ent_it != std::end(ent_cpy); ++ent_it)
					{
						const entity& e{*ent_it};
						indices.emplace_back(id_range);
						entityMap[e].emplace_back(key, id_range);
						++id_range;
					}
					core::profiler.scope_end();
				}
				else
				{
					core::profiler.scope_begin("slow path");
					for(auto ent_it = std::begin(ent_cpy); ent_it != std::end(ent_cpy); ++ent_it)
					{
						const entity& e{*ent_it};
						auto index = componentInfo.generator.CreateID().second;
						indices.emplace_back(index);
						entityMap[e].emplace_back(key, index);
					}
					core::profiler.scope_end();
				}
				core::profiler.scope_end();

				if constexpr(core::ecs::details::is_tag<T>::value)
				{
					if constexpr(!std::is_trivially_constructible<component_type>::value)
					{
						component_type v{};
						initialize_component(componentInfo.region.data(), indices, std::move(v));
					}
				}
				else
				{
					initialize_component(componentInfo.region.data(), indices, std::forward<forward_type>(_template));
				}
			}

			if (addedComponents)
			{
				auto& addedComponentsRange = addedComponents.value().get()[key];
				addedComponentsRange.insert(std::end(addedComponentsRange), std::begin(ent_cpy), std::end(ent_cpy));
			}
		}

			template <typename T>
			void remove_component(
				details::key_value_container_t<entity, std::vector<std::pair<details::component_key_t, size_t>>>& entityMap,
				details::key_value_container_t<details::component_key_t, details::component_info>& components,
				psl::array_view<entity> entities,
				std::optional<std::reference_wrapper<details::key_value_container_t<details::component_key_t, std::vector<entity>>>> removedComponents = std::nullopt) noexcept
			{
				PROFILE_SCOPE_STATIC(core::profiler)
					constexpr details::component_key_t key = details::component_key<details::remove_all<T>>;
				std::vector<entity> ent_cpy;
				ent_cpy.reserve(entities.size());
				for(auto e : entities)
				{
					auto eMapIt  = entityMap.find(e);
					auto foundIt = std::remove_if(eMapIt->second.begin(), eMapIt->second.end(),
						[&key](const std::pair<details::component_key_t, size_t>& pair) {
						return pair.first == key;
					});

					if(foundIt == std::end(eMapIt->second)) continue;

					ent_cpy.emplace_back(e);
					if constexpr(std::is_empty<T>::value)
					{
						eMapIt->second.erase(foundIt, eMapIt->second.end());

						const auto& eCompIt = components.find(key);
						eCompIt->second.entities.erase(
							std::remove(eCompIt->second.entities.begin(), eCompIt->second.entities.end(), e),
							eCompIt->second.entities.end());
					}
					else
					{
						auto index = foundIt->second;

						eMapIt->second.erase(foundIt, eMapIt->second.end());

						const auto& eCompIt = components.find(key);
						eCompIt->second.entities.erase(
							std::remove(eCompIt->second.entities.begin(), eCompIt->second.entities.end(), e),
							eCompIt->second.entities.end());

						void* loc = (void*)((std::uintptr_t)eCompIt->second.region.data() + eCompIt->second.size * index);
						std::memset(loc, 0, eCompIt->second.size);
					}
				}

				if (removedComponents)
				{
					auto& removedComponentsRange = removedComponents.value().get()[key];
					removedComponentsRange.insert(std::end(removedComponentsRange), std::begin(ent_cpy), std::end(ent_cpy));
				}
			}
	} // namespace details
} // namespace core::ecs

namespace std
{
	template <>
	struct hash<core::ecs::details::component_info>
	{
		std::size_t operator()(core::ecs::details::component_info const& ci) const noexcept
		{
			static_assert(sizeof(size_t) == sizeof(core::ecs::details::component_key_t), "should be castable");
			return (size_t)ci.id;
		}
	};
} // namespace std

namespace core::ecs
{
	class state
	{

	  public:
		/// \brief describes a set of dependencies for a given system
		///
		/// systems can have various dependencies, for example a movement system could have
		/// dependencies on both a core::ecs::components::transform component and a core::ecs::components::renderable
		/// component. This dependency will output a set of core::ecs::entity's that have all required
		/// core::ecs::components present. Certain systems could have sets of dependencies, for example the render
		/// system requires knowing about both all `core::ecs::components::renderable` that have a
		/// `core::ecs::components::transform`, but also needs to know all `core::ecs::components::camera's`. So that
		/// system would require several dependency_pack's.
		class dependency_pack
		{
			friend class core::ecs::state;
			template <std::size_t... Is, typename T>
			auto create_dependency_filters(std::index_sequence<Is...>, T& t)
			{
				(add(t.template reference_get<Is>()), ...);
			}

			template <typename F>
			void select_impl(std::vector<details::component_key_t>& target)
			{
				if constexpr(!std::is_same<details::remove_all<F>, core::ecs::entity>::value)
				{
					using component_t = F;
					constexpr details::component_key_t key =
						details::component_key<details::remove_all<component_t>>;
					target.emplace_back(key);
					m_Sizes[key] = sizeof(component_t);
				}
			}

			template <std::size_t... Is, typename T>
			auto select(std::index_sequence<Is...>, T, std::vector<details::component_key_t>& target)
			{
				(select_impl<typename std::tuple_element<Is, T>::type>(target), ...);
			}

		  public:
			template <typename... Ts>
			dependency_pack(core::ecs::pack<Ts...>& pack)
			{
				create_dependency_filters(
					std::make_index_sequence<std::tuple_size_v<typename core::ecs::pack<Ts...>::pack_t::range_t>>{},
					pack);
				using pack_t = core::ecs::pack<Ts...>;
				select(std::make_index_sequence<std::tuple_size<typename pack_t::filter_t>::value>{}, typename pack_t::filter_t{}, filters);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::add_t>::value>{}, typename pack_t::add_t{}, on_add);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::remove_t>::value>{}, typename pack_t::remove_t{}, on_remove);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::break_t>::value>{}, typename pack_t::break_t{}, on_break);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::combine_t>::value>{}, typename pack_t::combine_t{}, on_combine);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::except_t>::value>{}, typename pack_t::except_t{}, except);
			}
			dependency_pack(){};
			~dependency_pack() noexcept					  = default;
			dependency_pack(const dependency_pack& other) = default;
			dependency_pack(dependency_pack&& other)	  = default;
			dependency_pack& operator=(const dependency_pack&) = default;
			dependency_pack& operator=(dependency_pack&&) = default;

		  private:
			template <typename T>
			void add(psl::array_view<T>& vec) noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<details::remove_all<T>>;
				m_RWBindings.emplace(int_id, (psl::array_view<std::uintptr_t>*)&vec);
			}

			template <typename T>
			void add(psl::array_view<const T>& vec) noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<details::remove_all<T>>;
				m_RBindings.emplace(int_id, (psl::array_view<std::uintptr_t>*)&vec);
			}

			void add(psl::array_view<core::ecs::entity>& vec) noexcept { m_Entities = &vec; }

			void add(psl::array_view<const core::ecs::entity>& vec) noexcept
			{
				m_Entities = (psl::array_view<core::ecs::entity>*)&vec;
			}

		  private:
			psl::array_view<core::ecs::entity>* m_Entities{nullptr};
			psl::array_view<core::ecs::entity> m_StoredEnts{};
			details::key_value_container_t<details::component_key_t, size_t> m_Sizes;
			details::key_value_container_t<details::component_key_t, psl::array_view<std::uintptr_t>*> m_RBindings;
			details::key_value_container_t<details::component_key_t, psl::array_view<std::uintptr_t>*> m_RWBindings;

			std::vector<details::component_key_t> filters;
			std::vector<details::component_key_t> on_add;
			std::vector<details::component_key_t> on_remove;
			std::vector<details::component_key_t> except;
			std::vector<details::component_key_t> on_combine;
			std::vector<details::component_key_t> on_break;
		};


	  private:
		struct system_description
		{
			template <typename T>
			system_description(T& target)
			{
				if constexpr(details::mf_tick<T>::value)
				{
					tick = [&target](core::ecs::commands& state, std::chrono::duration<float> dTime,
									 std::chrono::duration<float> rTime) { target.tick(state, dTime, rTime); };
				}
				if constexpr(details::mf_pre_tick<T>::value)
				{
					pre_tick = [&target](core::ecs::commands& state) { target.pre_tick(state); };
				}
				if constexpr(details::mf_post_tick<T>::value)
				{
					post_tick = [&target](core::ecs::commands& state) { target.post_tick(state); };
				}
			}

			std::vector<void*> external_dependencies;
			std::function<void(core::ecs::commands&, std::chrono::duration<float>, std::chrono::duration<float>)> tick;
			std::function<void(core::ecs::commands&)> pre_tick;
			std::function<void(core::ecs::commands&)> post_tick;

			std::vector<dependency_pack> tick_dependencies;
			std::vector<dependency_pack> pre_tick_dependencies;
			std::vector<dependency_pack> post_tick_dependencies;
		};

		template<typename Fn>
		void copy_components(details::component_key_t key, psl::array_view<entity> entities, const size_t component_size, Fn&& function)
		{
			static_assert(std::is_same<typename std::invoke_result<Fn, entity>::type, std::uintptr_t>::value, "the return value must be a location in memory");

			if (entities.size() == 0)
				return;

			auto& cInfo{ details::get_component_info(key, component_size, m_Components) };
			uint64_t component_location{0};
			if (component_size == 1)
			{
				cInfo.entities.insert(std::end(cInfo.entities), std::begin(entities), std::end(entities));
				for (auto e : entities)
				{
					m_EntityMap[e].emplace_back(std::pair<details::component_key_t, size_t>{key, component_location});
				}
				std::sort(std::begin(cInfo.entities), std::end(cInfo.entities));
			}
			else if ((entities.size() == 1 && cInfo.generator.CreateID(component_location)) || cInfo.generator.CreateRangeID(component_location, entities.size()))
			{
				cInfo.entities.insert(std::end(cInfo.entities), std::begin(entities), std::end(entities));
				for (auto e : entities)
				{
					std::uintptr_t source = std::invoke(function, e);
					std::uintptr_t destination = (std::uintptr_t)cInfo.region.data() + component_location * component_size;
					std::memcpy((void*)destination, (void*)source, component_size);
					m_EntityMap[e].emplace_back(std::pair<details::component_key_t, size_t>{key, component_location});
					++component_location;
				}
				std::sort(std::begin(cInfo.entities), std::end(cInfo.entities));
			}
			else
			{
				auto halfway_it = std::next(std::begin(entities), entities.size() / 2);
				if (halfway_it != std::end(entities) && halfway_it != std::begin(entities))
				{
					copy_components(key, psl::array_view<entity>(std::begin(entities), halfway_it), component_size, std::forward<Fn>(function));
					copy_components(key, psl::array_view<entity>(halfway_it, std::end(entities)), component_size, std::forward<Fn>(function));
				}
				else
				{
					auto it = std::begin(entities);
					for (auto e : entities)
					{
						copy_components(key, psl::array_view<entity>(it, std::next(it)), component_size, std::forward<Fn>(function));
						it = std::next(it);
					}
				}
			}
		}
		void destroy_component_generator_ids(details::component_info& cInfo, psl::array_view<entity> entities);
		public:
		// -----------------------------------------------------------------------------
		// add component
		// -----------------------------------------------------------------------------

		template <typename T>
		void add_component(psl::array_view<entity> entities, T&& _template) noexcept
		{
			details::add_component(m_EntityMap, m_Components, entities, std::forward<T>(_template), m_StateChange[(m_Tick + 1 )%2].added_components);
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities) noexcept
		{
			return add_component(entities, core::ecs::tag<T>{});
		}

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities, Ts&&... args) noexcept
		{
			(add_component(entities, std::forward<Ts>(args)), ...);
		}

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities) noexcept
		{
			(add_component<Ts>(entities), ...);
		}

		template <typename... Ts>
		void add_components(entity e, Ts&&... args) noexcept
		{
			(add_component(psl::array_view<entity>{&e, &e + 1}, std::forward<Ts>(args)), ...);
		}
		template <typename... Ts>
		void add_components(entity e) noexcept
		{
			(add_component<Ts>(psl::array_view<entity>{&e, &e + 1}), ...);
		}

		// -----------------------------------------------------------------------------
		// remove component
		// -----------------------------------------------------------------------------	private:
		template <typename T>
		void remove_component(psl::array_view<entity> entities) noexcept
		{
			details::remove_component<T>(m_EntityMap, m_Components, entities, m_StateChange[(m_Tick + 1 )%2].removed_components);
		}

		template <typename... Ts>
		void remove_components(psl::array_view<entity> entities) noexcept
		{
			(remove_component<Ts>(entities), ...);
		}


		template <typename T>
		void remove_component(entity e) noexcept
		{
			remove_component<T>(psl::array_view<entity>{&e, &e + 1});
		}

		template <typename... Ts>
		void remove_components(entity e) noexcept
		{
			(remove_component<Ts>(psl::array_view<entity>{&e, &e + 1}), ...);
		}


		// -----------------------------------------------------------------------------
		// create entities
		// -----------------------------------------------------------------------------


		template <typename... Ts>
		std::vector<entity> create(size_t count) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
				m_EntityMap.emplace(++mID, std::vector<std::pair<details::component_key_t, size_t>>{});
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components<Ts...>(result);
			}
			return result;
		}

		template <typename... Ts>
		std::vector<entity> create(size_t count, Ts&&... args) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
				m_EntityMap.emplace(++mID, std::vector<std::pair<details::component_key_t, size_t>>{});
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components(result, std::forward<Ts>(args)...);
			}
			return result;
		}

		template <typename... Ts>
		entity create_one(Ts&&... args) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			auto e = m_EntityMap.emplace(entity{++mID}, std::vector<std::pair<details::component_key_t, size_t>>{})
						 .first->first;
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components(e, std::forward<Ts>(args)...);
			}
			return e;
		}


		template <typename... Ts>
		entity create_one() noexcept
		{
			PROFILE_SCOPE(core::profiler)
			auto e = m_EntityMap.emplace(entity{++mID}, std::vector<std::pair<details::component_key_t, size_t>>{})
						 .first->first;
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components<Ts...>(e);
			}
			return e;
		}


		void destroy(psl::array_view<entity> entities) noexcept;
		void destroy(entity e) noexcept;


		// -----------------------------------------------------------------------------
		// filter on components
		// -----------------------------------------------------------------------------
		template <typename... Ts>
		std::vector<entity> filter() const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");
			return filter_foreach(std::make_index_sequence<sizeof...(Ts)>(), std::tuple<Ts...>());
		}

		template <typename... Ts>
		std::vector<entity> filter(std::vector<Ts>&... out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			auto entities{filter<Ts...>()};
			(fill_in(entities, out), ...);
			return entities;
		}

	  private:
		  std::vector<entity> filter(const dependency_pack& pack) const
		  {
			  std::optional<std::vector<entity>> result{ std::nullopt };

			  auto merge = [](std::optional<std::vector<entity>> out, std::vector<entity> to_merge) -> std::vector<entity>
			  {
				  if (!out)
					  return to_merge;

				  std::vector<entity> v_intersection;
				  v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				  std::set_intersection(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
					  std::end(to_merge), std::back_inserter(v_intersection));

				  return v_intersection;
			  };

			  auto difference = [](std::optional<std::vector<entity>> out, std::vector<entity> to_merge) -> std::vector<entity>
			  {
				  if (!out)
					  return to_merge;

				  std::vector<entity> v_intersection;
				  v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				  std::set_difference(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
					  std::end(to_merge), std::back_inserter(v_intersection));

				  return v_intersection;
			  };

			  if (pack.filters.size() > 0)
			  {
				  result = merge(result, filter_default(pack.filters));
			  }
			  if (pack.on_add.size() > 0)
			  {
				  result = merge(result, filter_on_add(pack.on_add));
			  }
			  if (pack.on_remove.size() > 0)
			  {
				  result =  merge(result, filter_on_remove(pack.on_remove));
			  }
			  if (pack.on_combine.size() > 0)
			  {
				  result = merge(result, filter_on_combine(pack.on_combine));
			  }
			  if (pack.on_break.size() > 0)
			  {
				  result = merge(result, filter_on_break(pack.on_break));
			  }
			  if (pack.except.size() > 0)
			  {
				  result = difference(result, filter_except(pack.except));
			  }

			  return result.value_or(std::vector<entity>{});
		  }

		template <std::size_t... Is, typename... Ts>
		std::vector<entity> filter_foreach(std::index_sequence<Is...>, std::tuple<Ts...>) const
		{
			std::optional<std::vector<entity>> result{ std::nullopt };
			auto merge = [](std::optional<std::vector<entity>> out, std::vector<entity> to_merge) -> std::vector<entity>
			{
				if (!out)
					return to_merge;

				std::vector<entity> v_intersection;
				v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				std::set_intersection(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
					std::end(to_merge), std::back_inserter(v_intersection));

				return v_intersection;
			};

			auto difference = [](std::optional<std::vector<entity>> out, std::vector<entity> to_merge) -> std::vector<entity>
			{
				if (!out)
					return to_merge;

				std::vector<entity> v_intersection;
				v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				std::set_difference(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
					std::end(to_merge), std::back_inserter(v_intersection));

				return v_intersection;
			};


			(std::invoke([&]() {
				using component_t = typename std::tuple_element<Is, std::tuple<Ts...>>::type;
				if constexpr (!details::is_exception<component_t>::value)
				{
					result = merge(result, filter_impl(component_t{}));
				}
			}),
				...);

			(std::invoke([&]() {
				using component_t = typename std::tuple_element<Is, std::tuple<Ts...>>::type;
				if constexpr (details::is_exception<component_t>::value)
				{
					result = difference(result, filter_impl(component_t{}));
				}
			}),
				...);

			return result.value_or(std::vector<entity>{});
		}
		
		std::vector<details::component_key_t> filter_keys(psl::array_view<details::component_key_t> keys, const details::key_value_container_t<details::component_key_t, std::vector<entity>>& key_list) const
		{
			std::vector<details::component_key_t> keys_out{};
			for (auto key : keys)
			{
				if (key_list.find(key) != std::end(key_list))
					keys_out.emplace_back(key);
			}
			return keys;
		}

		std::vector<entity> filter_on_add(std::vector<details::component_key_t> keys) const
		{
			return dynamic_filter(keys, m_StateChange[m_Tick%2].added_components);
		}

		std::vector<entity> filter_on_remove(std::vector<details::component_key_t> keys) const
		{
			return dynamic_filter(keys, m_StateChange[m_Tick%2].removed_components);
		}

		std::vector<entity> filter_except(std::vector<details::component_key_t> keys) const
		{
			return dynamic_filter(keys);
		}

		std::vector<entity> filter_on_combine(std::vector<details::component_key_t> keys) const
		{
			std::sort(std::begin(keys), std::end(keys));
			std::vector<details::component_key_t> added_keys{filter_keys(keys, m_StateChange[m_Tick%2].added_components)};
			if(added_keys.size() == 0) // at least 1 should be present
				return {};
			std::vector<details::component_key_t> remaining_keys{};

			std::set_difference(std::begin(keys), std::end(keys), std::begin(added_keys), std::end(added_keys),
				std::back_inserter(remaining_keys));

			auto entities = dynamic_filter(added_keys, m_StateChange[m_Tick%2].added_components);
			if(remaining_keys.size() > 0)
				return dynamic_filter(remaining_keys, entities);

			return entities;
		}

		std::vector<entity> filter_on_break(std::vector<details::component_key_t> keys) const
		{
			std::sort(std::begin(keys), std::end(keys));
			std::vector<details::component_key_t> added_keys{filter_keys(keys, m_StateChange[m_Tick%2].removed_components)};
			if(added_keys.size() == 0) // at least 1 should be present
				return {};
			std::vector<details::component_key_t> remaining_keys{};

			std::set_difference(std::begin(keys), std::end(keys), std::begin(added_keys), std::end(added_keys),
				std::back_inserter(remaining_keys));
			auto entities = dynamic_filter(added_keys, m_StateChange[m_Tick%2].removed_components);
			if(remaining_keys.size() > 0)
				return dynamic_filter(remaining_keys, entities);

			return entities;
		}

		std::vector<entity> filter_default(std::vector<details::component_key_t> keys) const
		{
			return dynamic_filter(keys);
		}

		template <typename... Ts>
		std::vector<entity> filter_impl(on_add<Ts...>) const
		{
			std::vector<details::component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_add(keys);
		}

		template <typename... Ts>
		std::vector<entity> filter_impl(on_remove<Ts...>) const
		{
			std::vector<details::component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_remove(keys);
		}

		template <typename... Ts>
		std::vector<entity> filter_impl(except<Ts...>) const
		{
			std::vector<details::component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_except(keys);
		}


		template <typename... Ts>
		std::vector<entity> filter_impl(on_combine<Ts...>) const
		{
			std::vector<details::component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_combine(keys);
		}

		template <typename... Ts>
		std::vector<entity> filter_impl(on_break<Ts...>) const
		{
			std::vector<details::component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_break(keys);
		}

		template <typename T>
		std::vector<entity> filter_impl(T) const
		{
			std::vector<details::component_key_t> keys{{details::component_key<details::remove_all<T>>}};
			return filter_default(keys);
		}


	  public:
		// -----------------------------------------------------------------------------
		// get component
		// -----------------------------------------------------------------------------
		template <typename T>
		T& get_component(entity e)
		{
			PROFILE_SCOPE(core::profiler)
			constexpr details::component_key_t int_id = details::component_key<details::remove_all<T>>;
			auto eMapIt								  = m_EntityMap.find(e);
			auto foundIt							  = std::find_if(
				 eMapIt->second.begin(), eMapIt->second.end(),
				 [&int_id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == int_id; });

			if(foundIt == std::end(eMapIt->second))
			{
				throw std::runtime_error("missing component");
			}
			const auto& index	= foundIt->second;
			const auto& mem_pair = m_Components.find(int_id);
			return *(T*)((std::uintptr_t)mem_pair->second.region.data() + mem_pair->second.size * index);
		}

		template <typename T>
		const T& get_component(entity e) const
		{
			PROFILE_SCOPE(core::profiler)
			constexpr details::component_key_t int_id = details::component_key<details::remove_all<T>>;
			auto eMapIt								  = m_EntityMap.find(e);
			auto foundIt							  = std::find_if(
				 eMapIt->second.begin(), eMapIt->second.end(),
				 [&int_id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == int_id; });

			if(foundIt == std::end(eMapIt->second))
			{
				throw std::runtime_error("missing component");
			}
			const auto& index	= foundIt->second;
			const auto& mem_pair = m_Components.find(int_id);
			return *(T*)((std::uintptr_t)mem_pair->second.region.data() + mem_pair->second.size * index);
		}

		// -----------------------------------------------------------------------------
		// systems
		// -----------------------------------------------------------------------------
		void tick(std::chrono::duration<float> dTime = std::chrono::duration<float>{0.0f});

		enum class listener_type
		{
			PER_TICK  = 0,
			ON_CHANGE = 1,
			ON_ADD	= 2,
			ON_REMOVE = 3
		};

		enum class call_policy_type
		{
			TICK	  = 0x1,
			PRE_TICK  = 0x2,
			POST_TICK = 0x4,
			ALL		  = TICK | PRE_TICK | POST_TICK
		};
		template <typename T>
		void register_system(T& target, call_policy_type policy = call_policy_type::TICK,
							 listener_type type = listener_type::PER_TICK)
		{
			PROFILE_SCOPE(core::profiler)
			m_Systems.emplace(&target, system_description{target});
		}

		/*template <typename T>
		void register_dependency(T& system, dependency_pack&& pack)
		{
			PROFILE_SCOPE(core::profiler)
			auto it = m_Systems.find(&system);
			if(it == std::end(m_Systems))
			{
				m_Systems.emplace(&system, system_description{system});
				it = m_Systems.find(&system);
			}
			it->second.tick_dependencies.emplace_back(std::move(pack));
		}*/

		template <typename T, typename Y, typename... Ts>
		void register_dependency(T& system, Y method, core::ecs::pack<Ts...>& pack)
		{
			PROFILE_SCOPE(core::profiler)
			auto it = m_Systems.find(&system);
			if(it == std::end(m_Systems))
			{
				m_Systems.emplace(&system, system_description{system});
				it = m_Systems.find(&system);
			}
			dependency_pack p{pack};
			if constexpr(std::is_same<std::remove_cv_t<Y>, core::ecs::tick>::value)
			{
				static_assert(details::mf_tick<T>::value, "cannot register tick as there is not method that has a suitable signature or the method itself does not exist.");
				it->second.tick_dependencies.emplace_back(std::move(p));
			}
			else if constexpr(std::is_same<std::remove_cv_t<Y>, core::ecs::pre_tick>::value)
			{
				static_assert(details::mf_pre_tick<T>::value, "cannot register pre_tick as there is not method that has a suitable signature or the method itself does not exist.");
				it->second.pre_tick_dependencies.emplace_back(std::move(p));
			}
			else if constexpr(std::is_same<std::remove_cv_t<Y>, core::ecs::post_tick>::value)
			{
				static_assert(details::mf_post_tick<T>::value, "cannot register post_tick as there is not method that has a suitable signature or the method itself does not exist.");
				it->second.post_tick_dependencies.emplace_back(std::move(p));
			}
			else
			{
				static_assert(
					utility::templates::always_false_v<Y>,
					"the method should be one of the pre-approved types. Either `tick`, `pre_tick`, or `post_tick`");
			}
		}

		template <typename T>
		void set(psl::array_view<entity> entities, psl::array_view<T> data)
		{
			PROFILE_SCOPE(core::profiler)
			constexpr details::component_key_t id = details::component_key<details::remove_all<T>>;
			const auto& mem_pair				  = m_Components.find(id);
			auto size							  = sizeof(T);

			for(const auto& [i, e] : psl::enumerate(entities))
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::find_if(
					eMapIt->second.begin(), eMapIt->second.end(),
					[&id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == id; });

				auto index = foundIt->second;
				void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
				std::memcpy(loc, &data[i], size);
			}
		}

		bool exists(entity e) const noexcept
		{
			return m_EntityMap.find(e) != std::end(m_EntityMap);
		}
		template<typename T>
		bool has_component(entity e) const noexcept
		{
			constexpr details::component_key_t key = details::component_key<details::remove_all<T>>;
			if (auto eIt = m_EntityMap.find(e); eIt != std::end(m_EntityMap))
			{
				return std::any_of(std::begin(eIt->second), std::end(eIt->second), [&key](const std::pair<details::component_key_t, size_t> comp_pair) { return comp_pair.first == key; });
			}

			return false;
		}

		template<typename... Ts>
		bool has_components(entity e) const noexcept
		{
			std::vector<details::component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			if (auto eIt = m_EntityMap.find(e); eIt != std::end(m_EntityMap))
			{
				return std::all_of(std::begin(keys), std::end(keys), [&eIt](const details::component_key_t& key)
				{
					return std::any_of(std::begin(eIt->second), std::end(eIt->second), [&key](const std::pair<details::component_key_t, size_t> comp_pair) { return comp_pair.first == key; });
				});			
			}
			return false;
		}


		// will return false when the entity does not exist either
		template<typename T>
		bool is_owned_by(entity e, const T& component)
		{
			constexpr details::component_key_t key = details::component_key<details::remove_all<T>>;
			if (auto eIt = m_EntityMap.find(e); eIt != std::end(m_EntityMap))
			{
				if (auto compIt = std::find_if(std::begin(eIt->second), std::end(eIt->second), [&key](const std::pair<details::component_key_t, size_t> comp_pair) { return comp_pair.first == key; });
					compIt != std::end(eIt->second))
				{
					auto compDataIt = m_Components.find(key);
					auto diff = &component - (T*)compDataIt->second.region.data();
					return compIt->second == diff;
				}
				return false;
			}
			return false;
		}

		template<typename... Ts>
		bool is_owned_by(entity e, const Ts&... components)
		{
			return false;
		}

	  private:
		void set(psl::array_view<entity> entities, void* data, size_t size, details::component_key_t id);
		std::vector<entity> dynamic_filter(psl::array_view<details::component_key_t> keys,
										   std::optional<psl::array_view<entity>> pre_selection = std::nullopt) const
			noexcept;
		std::vector<entity>
		dynamic_filter(psl::array_view<details::component_key_t> keys,
					   const details::key_value_container_t<details::component_key_t, std::vector<entity>>& container,
					   std::optional<psl::array_view<entity>> pre_selection = std::nullopt) const noexcept;
		void fill_in(psl::array_view<entity> entities, details::component_key_t int_id, void* out) const noexcept;

		template <typename T>
		void fill_in(psl::array_view<entity> entities, std::vector<T>& out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			out.resize(entities.size());
			return fill_in(entities, details::component_key<details::remove_all<T>>, out.data());
		}

		void execute_commands(commands& cmds);

		uint64_t mID{0u};
		/// \brief gets what components this entity uses, and which index it lives on.
		details::key_value_container_t<entity, std::vector<std::pair<details::component_key_t, size_t>>> m_EntityMap;

		/// \brief backing memory
		details::key_value_container_t<details::component_key_t, details::component_info> m_Components;
		/// overhead is
		/// sizeof(component) * Nc + sizeof(entity) * Ne +
		/// // store ever component as well as entity (sizeof(entity) + ((sizeof(size_t) /* component ID */ +
		/// sizeof(size_t) /* component index*/) * Nce)) * Ne +	// for every entity store the component ID, and index
		/// that it uses (sizeof(size_t) /* component ID */ + sizeof(entity) * Nec) * nct
		/// // for every component, store the entity that uses them.
		///
		/// where Nc is the count of unique components that exists (not types)
		/// where Ne is the count of the unique entities that exists
		/// where Nce is the count of components this entity has
		/// where Nec is the count of entities that uses this component type.
		/// where Nct is the count of unique component types.

		std::unordered_map<void*, system_description> m_Systems;
		memory::raw_region m_Cache{1024 * 1024 * 32};

		// keep track of changed data
		struct difference_set
		{
			void clear()
			{
				added_entities.clear();
				removed_entities.clear();
				added_components.clear();
				removed_components.clear();
			}
			std::vector<entity> added_entities;
			std::vector<entity> removed_entities;
			details::key_value_container_t<details::component_key_t, std::vector<entity>> added_components;
			details::key_value_container_t<details::component_key_t, std::vector<entity>> removed_components;
		};

		std::array<difference_set, 2> m_StateChange;
		size_t m_Tick{ 0 };

		// std::unordered_map<details::component_key_t,
	};

	/// \brief contains commands for the core::ecs::state to process at a later time
	///
	/// this class will be passed to all ecs systems that wish to get access to editing
	/// the ECS state while the system is being executed.
	/// There is no promise when these commands get executed, except that they will be synchronized
	/// by the next tick event.
	/// \warn there is an order to the commands, the command with least precedence is the add_components
	/// commands, followed by remove_components commands. After which the create entity command has precedence
	/// and finally the destroy entity. This means adding an entity, adding components, and then destroying it 
	/// turns into a no-op.
	/// \warn the precedence of commands persists between several command blocks. This means that even if command
	/// block 'A' adds components, if command block 'B' removes them, it will be as if they never existed.
	/// \warn entities and components that are created and destroyed immediately are not visible to the systems.
	/// you will not receive on_add and other filter instructions from these objects.
	class commands final
	{
		friend class ecs::state;
		// only our good friend ecs::state should be able to create us. This is to prevent misuse.
		commands(state& state, uint64_t id_offset);

	private:

		/// \brief verify the entities exist locally
		///
		/// Entities might exist, but not be present in the command queue's local data containers,
		/// this command makes sure that entities that exist in the ecs::state are then replicated 
		/// locally so that components can be added.
		/// \param[in] entities the entity list to verify
		void verify_entities(psl::array_view<entity> entities)
		{
			for (auto entity : entities)
			{
				if (entity <= m_StartID && m_State.exists(entity))
					m_EntityMap.emplace(entity, std::vector<std::pair<details::component_key_t, size_t>>{});
			}
		}
	public:
		// -----------------------------------------------------------------------------
		// add component
		// -----------------------------------------------------------------------------

		template <typename T>
		void add_component(psl::array_view<entity> entities, T&& _template) noexcept
		{
			if (entities.size() == 0)
				return;
			verify_entities(entities);
			details::add_component(m_EntityMap, m_Components, entities, std::forward<T>(_template));
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities) noexcept
		{
			return add_component(entities, core::ecs::tag<T>{});
		}

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities, Ts&&... args) noexcept
		{
			(add_component(entities, std::forward<Ts>(args)), ...);
		}

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities) noexcept
		{
			(add_component<Ts>(entities), ...);
		}

		template <typename... Ts>
		void add_components(entity e, Ts&&... args) noexcept
		{
			(add_component(psl::array_view<entity>{&e, &e + 1}, std::forward<Ts>(args)), ...);
		}
		template <typename... Ts>
		void add_components(entity e) noexcept
		{
			(add_component<Ts>(psl::array_view<entity>{&e, &e + 1}), ...);
		}

		// -----------------------------------------------------------------------------
		// remove component
		// -----------------------------------------------------------------------------
		template <typename T>
		void remove_component(psl::array_view<entity> entities) noexcept
		{
			if (entities.size() == 0)
				return;
			constexpr details::component_key_t key = details::component_key<details::remove_all<T>>;
			m_ErasedComponents[key].insert(std::end(m_ErasedComponents[key]), std::begin(entities), std::end(entities));
		}

		template <typename... Ts>
		void remove_components(psl::array_view<entity> entities) noexcept
		{
			(remove_component<Ts>(entities), ...);
		}


		template <typename T>
		void remove_component(entity e) noexcept
		{
			remove_component<T>(psl::array_view<entity>{&e, &e + 1});
		}

		template <typename... Ts>
		void remove_components(entity e) noexcept
		{
			(remove_component<Ts>(psl::array_view<entity>{&e, &e + 1}), ...);
		}

		// -----------------------------------------------------------------------------
		// create entities
		// -----------------------------------------------------------------------------
		template <typename... Ts>
		std::vector<entity> create(size_t count) noexcept
		{
			if (count == 0)
				return {};
			PROFILE_SCOPE(core::profiler)
				m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for (size_t i = 0u; i < count; ++i)
			{
				m_NewEntities.emplace_back(mID);
				m_EntityMap.emplace(++mID, std::vector<std::pair<details::component_key_t, size_t>>{});
			}
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components<Ts...>(result);
			}
			return result;
		}

		template <typename... Ts>
		std::vector<entity> create(size_t count, Ts&&... args) noexcept
		{
			if (count == 0)
				return {};
			PROFILE_SCOPE(core::profiler)
				m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for (size_t i = 0u; i < count; ++i)
			{
				m_NewEntities.emplace_back(mID);
				m_EntityMap.emplace(++mID, std::vector<std::pair<details::component_key_t, size_t>>{});
			}
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components(result, std::forward<Ts>(args)...);
			}
			return result;
		}

		// -----------------------------------------------------------------------------
		// destroy entities
		// -----------------------------------------------------------------------------
		void destroy(psl::array_view<entity> entities)
		{
			if (entities.size() == 0)
				return;
			m_MarkedForDestruction.insert(std::end(m_MarkedForDestruction), std::begin(entities), std::end(entities));
		}

	private:
		/// \brief applies the changeset to the local data for processing
		///
		/// conflicting commands, such as adding and removing components get resolved locally first
		/// before we process the final commands.
		void apply(size_t id_difference_n);
		std::vector<entity> m_MarkedForDestruction;
		std::vector<entity> m_NewEntities;

		details::key_value_container_t<details::component_key_t, std::vector<entity>> m_ErasedComponents;

		// these are reserved for added components only, not to be confused with dynamic editing of components
		details::key_value_container_t<entity, std::vector<std::pair<details::component_key_t, size_t>>> m_EntityMap;
		details::key_value_container_t<details::component_key_t, details::component_info> m_Components;

		state& m_State;
		uint64_t mID{0u};
		uint64_t m_StartID{0u};
	};
} // namespace core::ecs