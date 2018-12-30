#pragma once
#include "stdafx.h"
#include "entity.h"
#include "range.h"
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

	struct tick {};
	struct pre_tick {};
	struct post_tick {};

	class state;


	namespace details
	{

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

		template<typename T>
		using remove_all = typename std::remove_pointer<typename std::remove_reference<typename std::remove_cv<T>::type>::type>::type;

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
		struct mf_pre_tick<T, std::void_t<decltype(std::declval<T&>().pre_tick(std::declval<core::ecs::state&>()))>>
			: std::true_type
		{};

		template <typename T, typename SFINEA = void>
		struct mf_post_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `post_tick` listener.
		template <typename T>
		struct mf_post_tick<T, std::void_t<decltype(std::declval<T&>().post_tick(std::declval<core::ecs::state&>()))>>
			: std::true_type
		{};


		template <typename T, typename SFINEA = void>
		struct mf_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `tick` listener.
		template <typename T>
		struct mf_tick<T, std::void_t<decltype(std::declval<T&>().tick(std::declval<core::ecs::state&>(), std::declval<std::chrono::duration<float>>(), std::declval<std::chrono::duration<float>>()))>>
			: std::true_type
		{};
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
		template<typename KeyT, typename ValueT>
		using key_value_container = ska::bytell_hash_map<KeyT, ValueT>;
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
				(add(t.reference_get<Is>()), ...);
			}

			template<typename F>
			void filter_impl()
			{
				if constexpr (!std::is_same<details::remove_all<F>, core::ecs::entity>::value)
				{
					using component_t = F;
					constexpr details::component_key_t int_id = details::component_key<details::remove_all<component_t>>;
					filters.emplace_back(int_id);
					m_Sizes[int_id] = sizeof(component_t);
				}
			}

			template <std::size_t... Is, typename T>
			auto filter(std::index_sequence<Is...>, T& t)
			{
				(filter_impl<typename std::tuple_element<Is, typename T::filter_t>::type>(), ...);
			}

		public:
			template<typename... Ts>
			dependency_pack(core::ecs::pack<Ts...>& pack)
			{
				create_dependency_filters(std::make_index_sequence<std::tuple_size_v<typename core::ecs::pack<Ts...>::pack_t::range_t>>{}, pack);
				using filter_t = typename core::ecs::pack<Ts...>::filter_t;
				filter(std::make_index_sequence<std::tuple_size<filter_t>::value>{}, pack);
			}
			dependency_pack() {};
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

			void add(psl::array_view<core::ecs::entity>& vec) noexcept
			{
				m_Entities = &vec;
			}

			void add(psl::array_view<const core::ecs::entity>& vec) noexcept
			{
				m_Entities = (psl::array_view<core::ecs::entity>*)&vec;
			}

			template <typename... Ts>
			void add(Ts&&... args) noexcept
			{
				(add(args), ...);
			}
		private:
			psl::array_view<core::ecs::entity>* m_Entities{ nullptr };
			psl::array_view<core::ecs::entity> m_StoredEnts{  };
			key_value_container<details::component_key_t, size_t> m_Sizes;
			key_value_container<details::component_key_t, psl::array_view<std::uintptr_t>*> m_RBindings;
			key_value_container<details::component_key_t, psl::array_view<std::uintptr_t>*> m_RWBindings;
			std::vector<details::component_key_t> filters;
		};



	private:
		struct system_description
		{
			template<typename T>
			system_description(T& target)
			{
				if constexpr(details::mf_tick<T>::value)
				{
					tick = [&target](core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
					{
						target.tick(state, dTime, rTime);
					};
				}
				if constexpr(details::mf_pre_tick<T>::value)
				{
					pre_tick = [&target](core::ecs::state& state)
					{
						target.pre_tick(state);
					};
				}
				if constexpr(details::mf_post_tick<T>::value)
				{
					post_tick = [&target](core::ecs::state& state)
					{
						target.post_tick(state);
					};
				}
			}

			std::vector<void*> external_dependencies;
			std::function<void(core::ecs::state&, std::chrono::duration<float>, std::chrono::duration<float>)> tick;
			std::function<void(core::ecs::state&)> pre_tick;
			std::function<void(core::ecs::state&)> post_tick;

			std::vector<dependency_pack> tick_dependencies;
			std::vector<dependency_pack> pre_tick_dependencies;
			std::vector<dependency_pack> post_tick_dependencies;
		};

		// -----------------------------------------------------------------------------
		// add component
		// -----------------------------------------------------------------------------

	  private:
		template <typename T>
		typename std::enable_if<!std::is_invocable<T, size_t>::value>::type
		initialize_component(void* location, const std::vector<size_t>& indices, T&& data) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			for(auto i = 0; i < indices.size(); ++i)
			{
				std::memcpy((void*)((std::uintptr_t)location + indices[i] * sizeof(T)), &data, sizeof(T));
			}
		}

		template <typename T>
		typename std::enable_if<std::is_invocable<T, size_t>::value>::type
		initialize_component(void* location, const std::vector<size_t>& indices, T&& invokable) noexcept
		{
			constexpr size_t size = sizeof(typename std::invoke_result<T, size_t>::type);
			PROFILE_SCOPE(core::profiler)
			for(auto i = 0; i < indices.size(); ++i)
			{
				auto v{std::invoke(invokable, i)};
				std::memcpy((void*)((std::uintptr_t)location + indices[i] * size), &v, size);
			}
		}

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

	  public:
		template <typename T>
		void add_component(const std::vector<entity>& entities, T&& _template) noexcept
		{
			using component_type = typename get_component_type<T>::type;
			using forward_type   = typename get_forward_type<T>::type;


			PROFILE_SCOPE(core::profiler)
			static_assert(
				std::is_trivially_copyable<component_type>::value && std::is_standard_layout<component_type>::value,
				"the component type must be trivially copyable and standard layout "
				"(std::is_trivially_copyable<T>::value == true && std::is_standard_layout<T>::value == true)");
			constexpr details::component_key_t int_id = details::component_key<details::remove_all<component_type>>;
			core::profiler.scope_begin("duplicate_check");
			auto ent_cpy = entities;
			auto end	 = std::remove_if(std::begin(ent_cpy), std::end(ent_cpy), [this, int_id](const entity& e) {
				auto eMapIt = m_EntityMap.find(e);
				if(eMapIt == std::end(m_EntityMap)) return true;

				for(auto eComp : eMapIt->second)
				{
					if(eComp.first == int_id) return true;
				}
				return false;
			});
			core::profiler.scope_end();
			ent_cpy.resize(std::distance(std::begin(ent_cpy), end));
			std::sort(std::begin(ent_cpy), end);

			auto it = m_Components.find(int_id);
			if(it == m_Components.end())
			{
				core::profiler.scope_begin("create backing storage");
				if constexpr(std::is_empty<component_type>::value)
				{
					m_Components.emplace(int_id, details::component_info{{}, int_id});
				}
				else
				{
					m_Components.emplace(
						int_id, details::component_info{
									memory::raw_region{1024 * 1024 * 128}, {}, int_id, (size_t)sizeof(component_type)});
				}
				it = m_Components.find(int_id);
				core::profiler.scope_end();
			}
			auto& pair{it->second};

			m_EntityMap.reserve(m_EntityMap.size() + std::distance(std::begin(ent_cpy), end));

			if constexpr(std::is_empty<component_type>::value)
			{
				core::profiler.scope_begin("emplace empty components");
				for(auto ent_it = std::begin(ent_cpy); ent_it != end; ++ent_it)
				{
					const entity& e{*ent_it};
					m_EntityMap[e].emplace_back(int_id, 0);
					pair.entities.emplace(std::upper_bound(std::begin(pair.entities), std::end(pair.entities), e), e);
				}
				core::profiler.scope_end();
			}
			else
			{
				core::profiler.scope_begin("emplace components");
				std::vector<uint64_t> indices;
				uint64_t id_range;
				const auto count = std::distance(std::begin(ent_cpy), end);
				core::profiler.scope_begin("reserve");
				indices.reserve(count);
				m_EntityMap.reserve(m_EntityMap.size() + count);


				std::vector<entity> merged;
				merged.reserve(pair.entities.size() + count);
				std::merge(std::begin(pair.entities), std::end(pair.entities), std::begin(ent_cpy), end,
						   std::back_inserter(merged));
				pair.entities = std::move(merged);

				core::profiler.scope_end();

				if(pair.generator.CreateRangeID(id_range, count))
				{
					core::profiler.scope_begin("fast path");
					for(auto ent_it = std::begin(ent_cpy); ent_it != end; ++ent_it)
					{
						const entity& e{*ent_it};
						indices.emplace_back(id_range);
						m_EntityMap[e].emplace_back(int_id, id_range);
						++id_range;
					}
					core::profiler.scope_end();
				}
				else
				{
					core::profiler.scope_begin("slow path");
					for(auto ent_it = std::begin(ent_cpy); ent_it != end; ++ent_it)
					{
						const entity& e{*ent_it};
						auto index = pair.generator.CreateID().second;
						indices.emplace_back(index);
						m_EntityMap[e].emplace_back(int_id, index);
					}
					core::profiler.scope_end();
				}
				core::profiler.scope_end();

				if constexpr(core::ecs::details::is_tag<T>::value)
				{
					if constexpr(!std::is_trivially_constructible<component_type>::value)
					{
						component_type v{};
						initialize_component(pair.region.data(), indices, std::move(v));
					}
				}
				else
				{
					initialize_component(pair.region.data(), indices, std::forward<forward_type>(_template));
				}
			}

			m_AddedEntities.insert(std::end(m_AddedEntities), std::begin(ent_cpy), std::end(ent_cpy));
			auto& addedComponentsRange = m_AddedComponents[int_id];
			addedComponentsRange.insert(std::end(addedComponentsRange), std::begin(ent_cpy), std::end(ent_cpy));
		}

		template <typename T>
		void add_component(const std::vector<entity>& entities) noexcept
		{
			return add_component(entities, core::ecs::tag<T>{});
		}

		template <typename... Ts>
		void add_components(const std::vector<entity>& entities, Ts&&... args) noexcept
		{
			(add_component(entities, std::forward<Ts>(args)), ...);
		}

		template <typename... Ts>
		void add_components(const std::vector<entity>& entities) noexcept
		{
			(add_component<Ts>(entities), ...);
		}

		template <typename... Ts>
		void add_components(entity e, Ts&&... args) noexcept
		{
			(add_component(std::vector<entity>{e}, std::forward<Ts>(args)), ...);
		}
		template <typename... Ts>
		void add_components(entity e) noexcept
		{
			(add_component<Ts>(std::vector<entity>{e}), ...);
		}

		// -----------------------------------------------------------------------------
		// remove component
		// -----------------------------------------------------------------------------
		template <typename T>
		void remove_component(const std::vector<entity>& entities) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			constexpr details::component_key_t int_id = details::component_key<details::remove_all<T>>;
			std::vector<entity> ent_cpy;
			ent_cpy.reserve(entities.size());
			for(auto e : entities)
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::remove_if(eMapIt->second.begin(), eMapIt->second.end(),
											  [&int_id](const std::pair<details::component_key_t, size_t>& pair) {
												  return pair.first == int_id;
											  });

				if(foundIt == std::end(eMapIt->second)) continue;

				ent_cpy.emplace_back(e);
				if constexpr(std::is_empty<T>::value)
				{
					eMapIt->second.erase(foundIt, eMapIt->second.end());

					const auto& eCompIt = m_Components.find(int_id);
					eCompIt->second.entities.erase(
						std::remove(eCompIt->second.entities.begin(), eCompIt->second.entities.end(), e),
						eCompIt->second.entities.end());
				}
				else
				{
					auto index = foundIt->second;

					eMapIt->second.erase(foundIt, eMapIt->second.end());

					const auto& eCompIt = m_Components.find(int_id);
					eCompIt->second.entities.erase(
						std::remove(eCompIt->second.entities.begin(), eCompIt->second.entities.end(), e),
						eCompIt->second.entities.end());

					void* loc = (void*)((std::uintptr_t)eCompIt->second.region.data() + eCompIt->second.size * index);
					std::memset(loc, 0, eCompIt->second.size);
				}
			}

			m_RemovedEntities.insert(std::end(m_RemovedEntities), std::begin(ent_cpy), std::end(ent_cpy));
			auto& removedComponentsRange = m_RemovedComponents[int_id];
			removedComponentsRange.insert(std::end(removedComponentsRange), std::begin(ent_cpy), std::end(ent_cpy));
		}

		template <typename... Ts>
		void remove_components(const std::vector<entity>& entities) noexcept
		{
			(remove_component<Ts>(entities), ...);
		}


		template <typename T>
		void remove_component(entity e) noexcept
		{
			remove_component<T>(std::vector<entity>{e});
		}

		template <typename... Ts>
		void remove_components(entity e) noexcept
		{
			(remove_component<Ts>(std::vector<entity>{e}), ...);
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
			return dynamic_filter({details::component_key<details::remove_all<Ts>>...});
		}

		template <typename... Ts>
		std::vector<entity> filter(std::vector<Ts>&... out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");

			auto entities{dynamic_filter({details::component_key<details::remove_all<Ts>>...})};

			(fill_in(entities, out), ...);

			return entities;
		}

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
			PER_TICK = 0,
			ON_CHANGE = 1,
			ON_ADD = 2,
			ON_REMOVE = 3
		};

		enum class call_policy_type
		{
			TICK = 0x1,
			PRE_TICK = 0x2,
			POST_TICK = 0x4,
			ALL = TICK | PRE_TICK | POST_TICK
		};
		template <typename T>
		void register_system(T& target, call_policy_type policy = call_policy_type::TICK, listener_type type = listener_type::PER_TICK)
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

		template <typename T, typename Y, typename... Ts >
		void register_dependency(T& system, Y method, core::ecs::pack<Ts...>& pack)
		{
			PROFILE_SCOPE(core::profiler)
				auto it = m_Systems.find(&system);
			if(it == std::end(m_Systems))
			{
				m_Systems.emplace(&system, system_description{system});
				it = m_Systems.find(&system);
			}
			dependency_pack p{ pack };
			if constexpr (std::is_same<std::remove_cv_t<Y>, core::ecs::tick>::value)
			{
				it->second.tick_dependencies.emplace_back(std::move(p));
			}
			else if constexpr (std::is_same<std::remove_cv_t<Y>, core::ecs::pre_tick>::value)
			{
				it->second.pre_tick_dependencies.emplace_back(std::move(p));
			}
			else if constexpr (std::is_same<std::remove_cv_t<Y>, core::ecs::tick>::value)
			{
				it->second.post_tick_dependencies.emplace_back(std::move(p));
			}
			else
			{
				static_assert(utility::templates::always_false_v<Y>, "the method should be one of the pre-approved types. Either `tick`, `pre_tick`, or `post_tick`");
			}
		}

		template <typename T>
		void set(const std::vector<entity>& entities, const std::vector<T>& data)
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

	  private:
		void set(psl::array_view<entity> entities, void* data, size_t size, details::component_key_t id);
		void set(const core::ecs::vector<entity>& entities, void* data, size_t size, details::component_key_t id);
		std::vector<entity> dynamic_filter(const std::vector<details::component_key_t>& keys) const noexcept;
		void fill_in(const std::vector<entity>& entities, details::component_key_t int_id, void* out) const noexcept;

		template <typename T>
		void fill_in(const std::vector<entity>& entities, std::vector<T>& out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			out.resize(entities.size());
			return fill_in(entities, details::component_key<details::remove_all<T>>, out.data());
		}

		uint64_t mID{0u};
		/// \brief gets what components this entity uses, and which index it lives on.
		key_value_container<entity, std::vector<std::pair<details::component_key_t, size_t>>> m_EntityMap;

		/// \brief backing memory
		key_value_container<details::component_key_t, details::component_info> m_Components;
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
		std::vector<entity> m_AddedEntities;
		std::vector<entity> m_RemovedEntities;
		key_value_container<details::component_key_t, std::vector<entity>> m_AddedComponents;
		key_value_container<details::component_key_t, std::vector<entity>> m_RemovedComponents;

		// std::unordered_map<details::component_key_t,
	};
} // namespace core::ecs