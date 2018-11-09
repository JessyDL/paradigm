#pragma once
#include "stdafx.h"
#include "entity.h"
#include "range.h"
#include "memory/raw_region.h"
#include "IDGenerator.h"
#include <type_traits>
#include "event.h"

namespace core::ecs
{
	namespace details
	{
		// added to trick the compiler to not throw away the results at compile time
		template<typename T>
		constexpr const std::uintptr_t component_key_var{ 0u };

		template<typename T>
		constexpr const std::uintptr_t* component_key() noexcept
		{ 
			return &component_key_var<T>;
		}

		using component_key_t = const std::uintptr_t*(*)();


		struct component_info
		{
			component_info() = default;
			component_info(memory::raw_region&& region, std::vector<entity>&& entities, component_key_t id, size_t size)
				: region(std::move(region)), entities(std::move(entities)), id(id), size(size),
				  generator(region.size() / size){};
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
	template<typename T>
	struct filter{};

	class state
	{
		struct system_binding
		{
			system_binding() = default;

			template<typename... Ts>
			system_binding(const Ts&... args)
			{
				reg(args ...);
			}
			std::unordered_map<details::component_key_t, std::tuple<void**, void**, size_t>>
				m_RBindings;
			std::unordered_map<details::component_key_t, std::tuple<void**, void**, size_t>>
				m_RWBindings;
			std::vector<details::component_key_t> filters;

			template <typename T>
			void reg(core::ecs::vector<T, access::READ_WRITE>& vec) noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<T>;
				m_RWBindings.emplace(int_id,
									 std::tuple{(void**)&vec.data, (void**)&vec.tail, sizeof(T)});
				filters.emplace_back(int_id);
			}

			template <typename T>
			void reg(core::ecs::vector<T, access::READ_ONLY>& vec) noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<T>;
				m_RBindings.emplace(int_id,
									std::tuple{(void**)&vec.data, (void**)&vec.tail, sizeof(T)});
				filters.emplace_back(int_id);
			}

			template <typename T>
			void reg() noexcept
			{
				constexpr details::component_key_t int_id = details::component_key<T>;
				filters.emplace_back(int_id);
			}
		};


	  public:
		template <typename T>
		void add_component(const std::vector<entity>& entities, std::optional<T> _template = std::nullopt) noexcept
		{
			static_assert(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value, 
				"the component type must be trivially copyable and standard layout (std::is_trivially_copyable<T>::value == true && std::is_standard_layout<T>::value == true)");
			constexpr details::component_key_t int_id = details::component_key<T>;

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

			auto it = m_Components.find(int_id);
			if(it == m_Components.end())
			{
				m_Components.emplace(
					int_id,
					details::component_info{memory::raw_region{1024 * 1024 * 128}, {}, int_id, (size_t)sizeof(T)});
				it = m_Components.find(int_id);
			}
			auto& pair{it->second};

			if (!_template)
				_template = T();

			for(auto it = std::begin(ent_cpy); it != end; ++it)
			{
				const entity& e{*it};
				auto index = pair.generator.CreateID().second;

				std::memcpy((void*)((std::uintptr_t)pair.region.data() + index * sizeof(T)), &_template.value(),
								sizeof(T));				

				m_EntityMap[e].emplace_back(int_id, index);

				pair.entities.emplace(std::upper_bound(std::begin(pair.entities), std::end(pair.entities), e), e);
			}
		}

		template <typename... Ts>
		void add_components(const std::vector<entity>& entities, std::optional<Ts>... _template) noexcept
		{
			( add_component<Ts>(entities, _template), ...);
		}


		template <typename T>
		void add_component(entity e, std::optional<T> _template = std::nullopt) noexcept
		{
			add_component<T>(std::vector<entity>{e}, _template);
		}

		template <typename... Ts>
		void add_components(entity e, std::optional<Ts>... _template) noexcept
		{
			( add_component<Ts>(std::vector<entity>{e}, _template), ...);
		}

		template <typename... Ts>
		void add_components(entity e) noexcept
		{
			( add_component<Ts>(std::vector<entity>{e}), ...);
		}

		template <typename T>
		void remove_component(const std::vector<entity>& entities) noexcept
		{
			constexpr details::component_key_t int_id = details::component_key<T>;

			for (auto e : entities)
			{
				auto eMapIt = m_EntityMap.find(e);
				auto foundIt = std::remove_if(
					eMapIt->second.begin(), eMapIt->second.end(),
					[&int_id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == int_id; });

				if (foundIt == std::end(eMapIt->second)) continue;
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

		template <typename... Ts>
		void remove_components(const std::vector<entity>& entities) noexcept
		{
			( remove_component<Ts>(entities), ...);
		}


		template <typename T>
		void remove_component(entity e) noexcept
		{
			remove_component<T>(std::vector<entity>{e});
		}

		template <typename... Ts>
		void remove_components(entity e) noexcept
		{
			( remove_component<Ts>(std::vector<entity>{e}), ...);
		}


		template<typename ... Ts>
		entity create() noexcept
		{
			auto e = m_EntityMap.emplace(entity{++mID}, std::vector<std::pair<details::component_key_t, size_t>>{})
				.first->first;
			if constexpr (sizeof...(Ts) > 0)
			{
				add_components<Ts...>(e);
			}
			return e;
		}

		template<typename ... Ts>
		entity create(std::optional<Ts>... _template) noexcept
		{
			auto e = m_EntityMap.emplace(entity{++mID}, std::vector<std::pair<details::component_key_t, size_t>>{})
				.first->first;
			if constexpr (sizeof...(Ts) > 0)
			{
				add_components<Ts...>(e, _template...);
			}
			return e;
		}

		template<typename ... Ts>
		std::vector<entity> create(size_t count) noexcept
		{
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
				m_EntityMap.emplace(++mID, std::vector<std::pair<details::component_key_t, size_t>>{});
			if constexpr (sizeof...(Ts) > 0)
			{
				add_components<Ts...>(result);
			}
			return result;
		}

		template<typename ... Ts>
		std::vector<entity> create(size_t count, std::optional<Ts>... _template) noexcept
		{
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
				m_EntityMap.emplace(++mID, std::vector<std::pair<details::component_key_t, size_t>>{});
			if constexpr (sizeof...(Ts) > 0)
			{
				add_components<Ts...>(result, _template...);
			}
			return result;
		}

		bool destroy(entity e) noexcept
		{
			if(auto eMapIt = m_EntityMap.find(e); eMapIt != std::end(m_EntityMap))
			{
				for(const auto& [type, index] : eMapIt->second)
				{
					if(const auto& cMapIt = m_Components.find(type); cMapIt != std::end(m_Components))
					{
						cMapIt->second.entities.erase(
							std::remove(cMapIt->second.entities.begin(), cMapIt->second.entities.end(), e),
							cMapIt->second.entities.end());

						void* loc = (void*)((std::uintptr_t)cMapIt->second.region.data() + cMapIt->second.size * index);
						std::memset(loc, 0, cMapIt->second.size);
					}
				}
				return true;
			}
			return false;
		}

		template <typename... Ts>
		std::vector<entity> filter() const noexcept
		{
			static const std::vector<details::component_key_t> keys{{details::component_key<Ts> ...}};
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");

			for (const auto& key : keys)
			{
				if (m_Components.find(key) == std::end(m_Components))
					return {};
			}

			std::vector<entity> v_intersection{m_Components.at(keys[0]).entities};

			for(size_t i = 1; i < keys.size(); ++i)
			{
				std::vector<entity> intermediate;
				const auto& it = m_Components.at(keys[i]).entities;
				std::set_intersection(v_intersection.begin(), v_intersection.end(), it.begin(), it.end(),
									  std::back_inserter(intermediate));
				v_intersection = intermediate;
			}

			return v_intersection;
		}

		template <typename... Ts>
		std::vector<entity> filter(std::vector<Ts>&... out) const noexcept
		{
			static const std::vector<details::component_key_t> keys{{details::component_key<Ts> ...}};
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");

			auto entities{filter<Ts...>()};

			(fill_in(entities, out), ...);


			return entities;
		}

		template <typename T>
		T& get_component(entity e)
		{
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
		const T& read_component(entity e) const
		{
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

		void tick(std::chrono::duration<float> dTime = std::chrono::duration<float>{ 0.0f })
		{
			for(const auto& system : m_Systems)
			{
				auto& sBindings				= std::get<1>(system.second);
				auto entities				= dynamic_filter(sBindings.filters);
				std::uintptr_t cache_offset = (std::uintptr_t)m_Cache.data();

				for(const auto& rwBinding : sBindings.m_RWBindings)
				{
					const auto& mem_pair = m_Components.find(rwBinding.first);
					auto size			 = std::get<2>(rwBinding.second);
					auto id				 = rwBinding.first;

					auto& data_begin = std::get<0>(rwBinding.second);
					*data_begin = (void*)cache_offset;
					for(const auto& e : entities)
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
					*data_end = (void*)cache_offset;
				}
				for(const auto& rBinding : sBindings.m_RBindings)
				{
					const auto& mem_pair = m_Components.find(rBinding.first);
					auto size			 = std::get<2>(rBinding.second);
					auto id				 = rBinding.first;

					auto& data_begin = std::get<0>(rBinding.second);
					*data_begin = (void*)cache_offset;
					for(const auto& e : entities)
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
					*data_end = (void*)cache_offset;
				}

				std::invoke(std::get<0>(system.second), *this, entities, dTime);
				for(const auto& rwBinding : sBindings.m_RWBindings)
				{
					set(entities, *(void**)std::get<0>(rwBinding.second), std::get<2>(rwBinding.second), rwBinding.first);
				}
			}
		}

		template <typename T>
		void register_system(T& target)
		{
			m_Systems.emplace(&target, std::tuple{std::bind(&T::tick, &target, std::placeholders::_1,
															std::placeholders::_2, std::placeholders::_3),
												  system_binding{}});
			currBinding = &std::get<1>(m_Systems[(void*)&target]);
			target.announce(*this);
			currBinding = nullptr;
		}

		template <typename T, core::ecs::access access_level >
		void register_dependency(core::ecs::vector<T, access_level>& range) noexcept
		{
			currBinding->reg<T>(range);
		}

		template <typename T>
		void register_dependency()
		{
			currBinding->reg<T>();
		}

		template <typename T>
		void set(const std::vector<entity>& entities, const std::vector<T>& data)
		{
			constexpr details::component_key_t id = details::component_key<T>;
			const auto& mem_pair				  = m_Components.find(id);
			auto size							  = sizeof(T);
			size_t i = 0;

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
		void set(const std::vector<entity>& entities, void* data, size_t size, details::component_key_t id)
		{
			const auto& mem_pair = m_Components.find(id);

			size_t i = 0;
			std::uintptr_t data_loc = (std::uintptr_t)data;
			for(const auto& e : entities)
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::find_if(
					eMapIt->second.begin(), eMapIt->second.end(),
					[&id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == id; });

				auto index = foundIt->second;
				void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
				std::memcpy(loc,(void*)data_loc, size);
				data_loc += size;
			}
		}

		std::vector<entity> dynamic_filter(const std::vector<details::component_key_t>& keys) const noexcept
		{
			for (const auto& key : keys)
			{
				if (m_Components.find(key) == std::end(m_Components))
					return {};
			}
			std::vector<entity> v_intersection{m_Components.at(keys[0]).entities};

			for(size_t i = 1; i < keys.size(); ++i)
			{
				std::vector<entity> intermediate;
				const auto& it = m_Components.at(keys[i]).entities;
				std::set_intersection(v_intersection.begin(), v_intersection.end(), it.begin(), it.end(),
									  std::back_inserter(intermediate));
				v_intersection = intermediate;
			}

			return v_intersection;
		}

		std::vector<entity>
		dynamic_filter(const std::vector<details::component_key_t>& keys,
					   const std::vector<std::pair<details::component_key_t, void*>>& targetBuffers) const noexcept
		{

			auto entities{dynamic_filter(keys)};

			//(fill_in(entities, out), ...);


			return entities;
		}

		template <typename T>
		void fill_in(const std::vector<entity>& entities, std::vector<T>& out) const noexcept
		{
			constexpr details::component_key_t int_id = details::component_key<T>;
			out.resize(entities.size());
			size_t i = 0;

			const auto& mem_pair = m_Components.find(int_id);

			for(const auto& e : entities)
			{
				auto eMapIt  = m_EntityMap.find(e);
				auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
											[&int_id](const std::pair<details::component_key_t, size_t>& pair) {
												return pair.first == int_id;
											});

				auto index = foundIt->second;
				void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + sizeof(T) * index);
				std::memcpy(&out[i], loc, sizeof(T));
				++i;
			}
		}

		void fill_in(const std::vector<entity>& entities, details::component_key_t int_id, void* out) const noexcept
		{
			/*out.resize(entities.size());
			size_t i = 0;

			for (const auto& e : entities)
			{
				auto eMapIt = m_EntityMap.find(e);
				auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(), [&int_id](const
			std::pair<details::component_key_t, size_t>& pair)
				{
					return pair.first == int_id;
				});

				auto index = foundIt->second;

				auto mem_pair = m_Components.find(int_id);
				auto range = mem_pair->second.second[index].range();
				std::memcpy(&out[i], (void*)range.begin, sizeof(T));
				++i;
			}*/
		}


		uint64_t mID{0u};
		/// \brief gets what components this entity uses, and which index it lives on.
		std::unordered_map<entity, std::vector<std::pair<details::component_key_t, size_t>>> m_EntityMap;

		/// \brief backing memory
		std::unordered_map<details::component_key_t, details::component_info> m_Components;
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

		// std::vector<std::function<void(std::chrono::duration<float>)>> m_Systems;
		system_binding* currBinding{nullptr};
		std::unordered_map<
			void*,
			std::tuple<std::function<void(core::ecs::state&, const std::vector<entity>&, std::chrono::duration<float>)>,
					   system_binding>>
			m_Systems;
		memory::raw_region m_Cache{1024 * 1024 * 32};


		//std::unordered_map<details::component_key_t, 
	};
} // namespace core::ecs