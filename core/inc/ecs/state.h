#pragma once
#include "stdafx.h"
#include "entity.h"
#include "range.h"
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

	struct  par
	{};

	template<bool has_entities = true, typename... Ts>
	class pack
	{
		using range_t = std::tuple<core::ecs::vector<entity>, core::ecs::vector<Ts>...>;
		using range_element_t = std::tuple<entity, Ts...>;
		using iterator_element_t = std::tuple<typename core::ecs::vector<entity>::iterator, typename core::ecs::vector<Ts>::iterator ...>;

		template <typename Tuple, typename F, std::size_t ...Indices>
		static void for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>)
		{
			using swallow = int[];
			(void)swallow
			{
				1,
					(f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...
			};
		}

		template <typename Tuple, typename F>
		static void for_each(Tuple&& tuple, F&& f)
		{
			constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
			for_each_impl(std::forward<Tuple>(tuple), std::forward<F>(f),
						  std::make_index_sequence<N>{});
		}
	public:
		class iterator
		{
		public:
			constexpr iterator(const range_t& range) noexcept 
			{
			};
			constexpr iterator(iterator_element_t data) noexcept : data(data) {};
			constexpr const iterator_element_t& operator*() const noexcept
			{
				return data;
			}
			constexpr bool operator!=(iterator other) const noexcept
			{
				return std::get<0>(data) != std::get<0>(other.data);
			}
			constexpr iterator operator++() const noexcept
			{
				auto next = iterator(data);
				++next;
				return next;
			}
			constexpr iterator& operator++() noexcept
			{
				for_each(data, [](auto& element){++element;});
				return *this;
			}
		private:
			iterator_element_t data;
		};
		range_t read()
		{
			return m_Pack;
		}

		template<typename T>
		const core::ecs::vector<T>& get()
		{
			return std::get<core::ecs::vector<T>>(m_Pack);
		}

		iterator begin() const noexcept
		{
			return iterator{m_Pack};
		}

		iterator end() const noexcept
		{
			return iterator{m_Pack};
		}
		constexpr size_t size() const noexcept
		{
			return std::get<0>(m_Pack).size();
		}

	private:
		range_t m_Pack;
	};


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
	template <typename T>
	struct filter
	{};

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

		  public:
			/// \brief constructs a unique set of dependencies
			dependency_pack(core::ecs::vector<entity>& entities) : m_Entities(entities){};
			template <typename... Ts>
			dependency_pack(core::ecs::vector<entity>& entities, Ts&&... filters) : m_Entities(entities)
			{
				(add(filters), ...);
			};
			~dependency_pack() noexcept					  = default;
			dependency_pack(const dependency_pack& other) = default;
			dependency_pack(dependency_pack&& other)	  = default;
			dependency_pack& operator=(const dependency_pack&) = default;
			dependency_pack& operator=(dependency_pack&&) = default;

			template <typename T>
			void add(core::ecs::vector<T>& vec) noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<T>;
				m_RWBindings.emplace(int_id, std::tuple{(void**)&vec.data, (void**)&vec.tail, sizeof(T)});
				filters.emplace_back(int_id);
			}

			template <typename T>
			void add(core::ecs::vector<const T>& vec) noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<T>;
				m_RBindings.emplace(int_id, std::tuple{(void**)&vec.data, (void**)&vec.tail, sizeof(T)});
				filters.emplace_back(int_id);
			}

			template <typename T>
			void add(core::ecs::filter<T>& filter) noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<T>;
				filters.emplace_back(int_id);
			}

			template <typename... Ts>
			void add(Ts&&... args) noexcept
			{
				(add(args), ...);
			}

		  private:
			core::ecs::vector<entity>& m_Entities;
			ska::bytell_hash_map<details::component_key_t, std::tuple<void**, void**, size_t>> m_RBindings;
			ska::bytell_hash_map<details::component_key_t, std::tuple<void**, void**, size_t>> m_RWBindings;
			std::vector<details::component_key_t> filters;
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
			constexpr details::component_key_t int_id = details::component_key<component_type>;
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
						int_id,
						details::component_info{memory::raw_region{1024 * 1024 * 128}, {}, int_id, (size_t)sizeof(component_type)});
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
			constexpr details::component_key_t int_id = details::component_key<T>;

			for(auto e : entities)
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::remove_if(eMapIt->second.begin(), eMapIt->second.end(),
											  [&int_id](const std::pair<details::component_key_t, size_t>& pair) {
												  return pair.first == int_id;
											  });

				if(foundIt == std::end(eMapIt->second)) continue;
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


		void destroy(const std::vector<entity>& entities) noexcept
		{
			PROFILE_SCOPE(core::profiler)

			ska::bytell_hash_map<details::component_key_t, std::vector<entity>> erased_entities;
			ska::bytell_hash_map<details::component_key_t, std::vector<uint64_t>> erased_ids;
			core::profiler.scope_begin("erase entities");
			for(const auto& e : entities)
			{
				if(auto eMapIt = m_EntityMap.find(e); eMapIt != std::end(m_EntityMap))
				{
					for(const auto& [type, index] : eMapIt->second)
					{
						erased_entities[type].emplace_back(e);
						erased_ids[type].emplace_back(index);
					}
					m_EntityMap.erase(eMapIt);
				}
			}
			core::profiler.scope_end();

			core::profiler.scope_begin("erase IDs");
			for(auto& c : erased_ids)
			{
				if(const auto& cMapIt = m_Components.find(c.first); cMapIt != std::end(m_Components))
				{
					if(c.second.size() > 64)
					{
						std::sort(std::begin(c.second), std::end(c.second));
						auto index		 = std::begin(c.second);
						auto range_start = index;
						const auto end   = std::prev(std::end(c.second), 1);
						while(index != end)
						{
							auto next = std::next(index, 1);
							if(*index + 1 != *next)
							{
								cMapIt->second.generator.DestroyRangeID(*range_start, std::distance(range_start, next));
								range_start = next;
							}
							index = next;
						}
						cMapIt->second.generator.DestroyRangeID(*range_start, std::distance(range_start, std::end(c.second)));
					}
					else
					{
						for(auto id : c.second) cMapIt->second.generator.DestroyID(id);
					}
				}
			}
			core::profiler.scope_end();

			core::profiler.scope_begin("erase components");
			for(auto& c : erased_entities)
			{
				if(const auto& cMapIt = m_Components.find(c.first); cMapIt != std::end(m_Components))
				{
					std::sort(std::begin(c.second), std::end(c.second));
					auto ib   = std::begin(c.second);
					auto iter = std::remove_if(std::begin(cMapIt->second.entities), std::end(cMapIt->second.entities),
											   [&ib, &c](entity x) -> bool {
												   while(ib != std::end(c.second) && *ib < x) ++ib;
												   return (ib != std::end(c.second) && *ib == x);
											   });

					cMapIt->second.entities.erase(iter, cMapIt->second.entities.end());
				}
			}
			core::profiler.scope_end();
		}

		void destroy(entity e) noexcept { destroy({e}); }


		// -----------------------------------------------------------------------------
		// filter on components
		// -----------------------------------------------------------------------------
		template <typename... Ts>
		std::vector<entity> filter() const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");
			return dynamic_filter({details::component_key<Ts>...});
		}

		template <typename... Ts>
		std::vector<entity> filter(std::vector<Ts>&... out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");

			auto entities{dynamic_filter({details::component_key<Ts>...})};

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
			constexpr details::component_key_t int_id = details::component_key<T>;
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
			constexpr details::component_key_t int_id = details::component_key<T>;
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
		void tick(std::chrono::duration<float> dTime = std::chrono::duration<float>{0.0f})
		{
			PROFILE_SCOPE(core::profiler)
			for(auto& system : m_Systems)
			{
				core::profiler.scope_begin("ticking system");
				auto& sBindings				= std::get<1>(system.second);
				std::uintptr_t cache_offset = (std::uintptr_t)m_Cache.data();
				core::profiler.scope_begin("preparing data");
				for(auto& dep_pack : sBindings)
				{
					auto entities = dynamic_filter(dep_pack.filters);
					std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
					dep_pack.m_Entities.data = (entity*)cache_offset;
					dep_pack.m_Entities.tail = (entity*)(cache_offset + sizeof(entity) * entities.size());
					cache_offset += sizeof(entity) * entities.size();
					core::profiler.scope_begin("read-write data");
					for(const auto& rwBinding : dep_pack.m_RWBindings)
					{
						const auto& mem_pair = m_Components.find(rwBinding.first);
						auto size			 = std::get<2>(rwBinding.second);
						auto id				 = rwBinding.first;

						auto& data_begin = std::get<0>(rwBinding.second);
						*data_begin		 = (void*)cache_offset;
						for(const auto& e : dep_pack.m_Entities)
						{

							auto eMapIt  = m_EntityMap.find(e);
							auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
														[&id](const std::pair<details::component_key_t, size_t>& pair) {
															return pair.first == id;
														});

							auto index = foundIt->second;
							void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
							std::memcpy((void*)cache_offset, loc, std::get<2>(rwBinding.second));
							cache_offset += size;
						}
						auto& data_end = std::get<1>(rwBinding.second);
						*data_end	  = (void*)cache_offset;
					}
					core::profiler.scope_end();
					core::profiler.scope_begin("read-only data");
					for(const auto& rBinding : dep_pack.m_RBindings)
					{
						const auto& mem_pair = m_Components.find(rBinding.first);
						auto size			 = std::get<2>(rBinding.second);
						auto id				 = rBinding.first;

						auto& data_begin = std::get<0>(rBinding.second);
						*data_begin		 = (void*)cache_offset;
						for(const auto& e : dep_pack.m_Entities)
						{

							auto eMapIt  = m_EntityMap.find(e);
							auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
														[&id](const std::pair<details::component_key_t, size_t>& pair) {
															return pair.first == id;
														});

							auto index = foundIt->second;
							void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
							std::memcpy((void*)cache_offset, loc, std::get<2>(rBinding.second));
							cache_offset += size;
						}
						auto& data_end = std::get<1>(rBinding.second);
						*data_end	  = (void*)cache_offset;
					}
					core::profiler.scope_end();
				}
				core::profiler.scope_end();
				std::invoke(std::get<0>(system.second), *this, dTime);
				for(const auto& dep_pack : sBindings)
				{
					for(const auto& rwBinding : dep_pack.m_RWBindings)
					{
						set(dep_pack.m_Entities, *(void**)std::get<0>(rwBinding.second), std::get<2>(rwBinding.second),
							rwBinding.first);
					}
				}
				core::profiler.scope_end();
			}
		}

		template <typename T>
		void register_system(T& target)
		{
			PROFILE_SCOPE(core::profiler)
			m_Systems.emplace(&target,
							  std::tuple{std::bind(&T::tick, &target, std::placeholders::_1, std::placeholders::_2),
										 std::vector<dependency_pack>{}});
			target.announce(*this);
		}

		template <typename T>
		void register_dependency(T& system, dependency_pack&& pack)
		{
			PROFILE_SCOPE(core::profiler)
			auto it = m_Systems.find(&system);
			if(it == std::end(m_Systems))
			{
				m_Systems.emplace(&system,
								  std::tuple{std::bind(&T::tick, &system, std::placeholders::_1, std::placeholders::_2),
											 std::vector<dependency_pack>{}});
				it = m_Systems.find(&system);
			}
			std::get<1>(it->second).emplace_back(std::move(pack));
		}


		template <typename T>
		void set(const std::vector<entity>& entities, const std::vector<T>& data)
		{
			PROFILE_SCOPE(core::profiler)
			constexpr details::component_key_t id = details::component_key<T>;
			const auto& mem_pair				  = m_Components.find(id);
			auto size							  = sizeof(T);
			size_t i							  = 0;

			for(const auto& e : entities)
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::find_if(
					eMapIt->second.begin(), eMapIt->second.end(),
					[&id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == id; });

				auto index = foundIt->second;
				void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
				std::memcpy(loc, &data[i], size);
				++i;
			}
		}

	  private:
		void set(const core::ecs::vector<entity>& entities, void* data, size_t size, details::component_key_t id)
		{
			PROFILE_SCOPE(core::profiler)
			const auto& mem_pair = m_Components.find(id);

			size_t i				= 0;
			std::uintptr_t data_loc = (std::uintptr_t)data;
			for(const auto& e : entities)
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::find_if(
					eMapIt->second.begin(), eMapIt->second.end(),
					[&id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == id; });

				auto index = foundIt->second;
				void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
				std::memcpy(loc, (void*)data_loc, size);
				data_loc += size;
			}
		}

		std::vector<entity> dynamic_filter(const std::vector<details::component_key_t>& keys) const noexcept
		{
			PROFILE_SCOPE(core::profiler)

			for(const auto& key : keys)
			{
				if(m_Components.find(key) == std::end(m_Components)) return {};
			}

			std::vector<entity> v_intersection{m_Components.at(keys[0]).entities};

			for(size_t i = 1; i < keys.size(); ++i)
			{
				std::vector<entity> intermediate;
				intermediate.reserve(v_intersection.size());
				const auto& it = m_Components.at(keys[i]).entities;
				std::set_intersection(v_intersection.begin(), v_intersection.end(), it.begin(), it.end(),
									  std::back_inserter(intermediate));
				v_intersection = std::move(intermediate);
			}

			return v_intersection;
		}


		void fill_in(const std::vector<entity>& entities, details::component_key_t int_id, void* out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			size_t i = 0;

			const auto& mem_pair = m_Components.find(int_id);
			const auto size = mem_pair->second.size;
			const std::uintptr_t data = (std::uintptr_t)mem_pair->second.region.data();
			for(const auto& e : entities)
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
											[&int_id](const std::pair<details::component_key_t, size_t>& pair) {
												return pair.first == int_id;
											});

				auto index = foundIt->second;
				void* loc  = (void*)(data + size * index);
				std::memcpy((void*)((std::uintptr_t)out + (i * size)), loc, size);
				++i;
			}
		}

		template <typename T>
		void fill_in(const std::vector<entity>& entities, std::vector<T>& out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			out.resize(entities.size());
			return fill_in(entities, details::component_key<T>, out.data());
		}

		uint64_t mID{0u};
		/// \brief gets what components this entity uses, and which index it lives on.
		ska::bytell_hash_map<entity, std::vector<std::pair<details::component_key_t, size_t>>> m_EntityMap;

		/// \brief backing memory
		ska::bytell_hash_map<details::component_key_t, details::component_info> m_Components;
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

		std::unordered_map<void*, std::tuple<std::function<void(core::ecs::state&, std::chrono::duration<float>)>,
											 std::vector<dependency_pack>>>
			m_Systems;
		memory::raw_region m_Cache{1024 * 1024 * 32};


		// std::unordered_map<details::component_key_t,
	};
} // namespace core::ecs