#pragma once
#include "stdafx.h"
#include "entity.h"
#include "range.h"
#include "memory/raw_region.h"
#include "IDGenerator.h"

namespace core::ecs
{
	template<typename T>
	class filter
	{
	};

	class state
	{
		// https://stackoverflow.com/questions/7562096/compile-time-constant-id/39640960#39640960
		template<typename>
		static void component_key() {};

		using component_key_t = void(*)();

		struct system_binding
		{
			std::unordered_map<component_key_t, void*> m_RBindings;
			std::unordered_map<component_key_t, void*> m_RWBindings;
			std::vector<component_key_t> filters;

			template<typename T>
			void register_rw(void* binding_point)
			{
				constexpr component_key_t int_id = component_key<T>;
				m_RWBindings[int_id] = binding_point;
				filters.emplace_back(int_id);
			}
			template<typename T>
			void register_r(void* binding_point)
			{
				constexpr component_key_t int_id = component_key<T>;
				m_RBindings[int_id] = binding_point;
				filters.emplace_back(int_id);
			}
			template<typename T>
			void register_filter()
			{
				constexpr component_key_t int_id = component_key<T>;
				filters.emplace_back(int_id);
			}

			void prepare(state& s)
			{
				auto& entities = s.dynamic_filter(filters);

				//std::vector<component_key_t, void*> targetbuffers;
				//s.dynamic_filter(entities, filters, targetbuffers);
			}
		};

	public:

		template<typename T>
		bool add_component(const std::vector<entity>& entities, std::optional<T> _template = std::nullopt) noexcept
		{
			static_assert(std::is_pod<T>::value, "the component type must be a POD (std::is_pod<T>::value == true)");
			constexpr component_key_t int_id = component_key<T>;

			auto ent_cpy = entities;
			auto end = std::remove_if(std::begin(ent_cpy), std::end(ent_cpy), [this, int_id](const entity& e)
			{
				auto eMapIt = m_EntityMap.find(e);
				if (eMapIt == std::end(m_EntityMap))
					return true;

				for (auto eComp : eMapIt->second)
				{
					if (eComp.first == int_id)
						return true;
				}
				return false;
			});

			auto it = m_Components.find(int_id);
			if (it == m_Components.end())
			{
				m_Components.emplace(int_id, std::tuple<memory::raw_region, size_t, psl::IDGenerator<uint64_t>>{ memory::raw_region{1024 * 1024 * 1024}, (size_t)sizeof(T), std::numeric_limits<uint64_t>::max() });
				it = m_Components.find(int_id);
			}
			auto& pair{ it->second };

			for(auto it = std::begin(ent_cpy); it != end; ++it)
			{
				const entity& e{ *it };
				auto index = std::get<2>(pair).CreateID().second;

				if (_template)
					std::memcpy(std::get<0>(pair).data(), &_template.value(), sizeof(T));

				m_EntityMap[e].emplace_back(int_id, index);
				m_ComponentMap[int_id].emplace_back(e);
			}
			return true;
		}

		template<typename T>
		bool add_component(entity e, std::optional<T> _template = std::nullopt) noexcept
		{
			return add_component<T>(std::vector<entity>{ e }, _template);
		}

		template<typename T>
		bool remove_component(entity e) noexcept
		{
			constexpr component_key_t int_id = component_key<T>;
			auto eMapIt = m_EntityMap.find(e);
			auto foundIt = std::remove_if(eMapIt->second.begin(), eMapIt->second.end(), [&int_id](const std::pair<component_key_t, size_t>& pair)
			{
				return pair.first == int_id;
			});

			if (foundIt == std::end(eMapIt->second))
				return false;
			auto index = foundIt->second;

			eMapIt->second.erase(foundIt, eMapIt->second.end());
			auto eCompIt = m_ComponentMap.find(int_id);
			eCompIt->second.erase(std::remove(eCompIt->second.begin(), eCompIt->second.end(), e), eCompIt->second.end());

			auto& mem_pair = m_Components.find(int_id);
			void* loc = (void*)((std::uintptr_t)std::get<0>(mem_pair->second).data() + sizeof(T) * index);
			std::memset(loc, 0, std::get<1>(mem_pair->second));

			return true;
		}

		entity create() noexcept
		{
			return m_EntityMap.emplace(entity{ ++mID }, std::vector<std::pair<component_key_t, size_t>>{}).first->first;
		}

		std::vector<entity> create(size_t count) noexcept
		{
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota (std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
				m_EntityMap.emplace(entity{ ++mID }, std::vector<std::pair<component_key_t, size_t>>{});
			return result;
		}

		bool destroy(entity e) noexcept
		{
			if (auto eMapIt = m_EntityMap.find(e); eMapIt != std::end(m_EntityMap))
			{
				for (const auto& [type, index] : eMapIt->second)
				{
					if (auto cMapIt = m_ComponentMap.find(type); cMapIt != std::end(m_ComponentMap))
					{
						cMapIt->second.erase(std::remove(cMapIt->second.begin(), cMapIt->second.end(), e), cMapIt->second.end());
					}

					auto& mem_pair = m_Components.find(type);
					void* loc = (void*)((std::uintptr_t)std::get<0>(mem_pair->second).data() + std::get<1>(mem_pair->second) * index);
					std::memset(loc, 0, std::get<1>(mem_pair->second));
				}
				return true;
			}
			return false;
		}

		template<typename... Ts>
		std::vector<entity> filter() const noexcept
		{
			static const std::vector<component_key_t> keys{ {component_key<Ts>...} };
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");

			std::vector<entity> v_intersection{ m_ComponentMap.at(keys[0]) };

			for (size_t i = 1; i < keys.size(); ++i)
			{
				std::vector<entity> intermediate;
				const auto& it = m_ComponentMap.at(keys[i]);
				std::set_intersection(v_intersection.begin(), v_intersection.end(),
					it.begin(), it.end(),
					std::back_inserter(intermediate));
				v_intersection = intermediate;
			}

			return v_intersection;
		}

		template<typename... Ts>
		std::vector<entity> filter(std::vector<Ts>&... out)  const noexcept
		{
			static const std::vector<component_key_t> keys{ {component_key<Ts>...} };
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");

			auto entities{ filter<Ts...>() };			

			(fill_in(entities, out), ...);


			return entities;
		}

		template<typename T>
		T& get_component(entity e)
		{
			constexpr component_key_t int_id = component_key<T>;
			auto eMapIt = m_EntityMap.find(e);
			auto foundIt = std::remove_if(eMapIt->second.begin(), eMapIt->second.end(), [&int_id](const std::pair<component_key_t, size_t>& pair)
			{
				return pair.first == int_id;
			});

			if (foundIt == std::end(eMapIt->second))
			{
				throw std::runtime_error("missing component");
			}
			auto index = foundIt->second;
			auto mem_pair = m_Components.find(int_id);
			return *(T*)((std::uintptr_t)std::get<0>(mem_pair->second).data() + std::get<1>(mem_pair->second) * index);
		}

		void tick()
		{

			for (const auto& system : m_Systems)
			{

				std::invoke(system, std::chrono::duration<float>{0});
			}
		}

		template<typename T>
		void register_system(T& target)
		{
			target.announce(*this);
			m_Systems.emplace_back(std::bind(&T::tick, &target, std::placeholders::_1));
		}

		template<typename T>
		void register_rw_range(range<T>& range)
		{
			constexpr component_key_t int_id = component_key<T>;
			currBindings.m_RWBindings[int_id] = &range;
		}
	private:
		std::vector<entity> dynamic_filter(const std::vector<component_key_t>& keys) const noexcept
		{
			std::vector<entity> v_intersection{ m_ComponentMap.at(keys[0]) };

			for (size_t i = 1; i < keys.size(); ++i)
			{
				std::vector<entity> intermediate;
				const auto& it = m_ComponentMap.at(keys[i]);
				std::set_intersection(v_intersection.begin(), v_intersection.end(),
					it.begin(), it.end(),
					std::back_inserter(intermediate));
				v_intersection = intermediate;
			}

			return v_intersection;
		}

		std::vector<entity> dynamic_filter(const std::vector<component_key_t>& keys, const std::vector<std::pair<component_key_t, void*>>& targetBuffers)  const noexcept
		{

			auto entities{ dynamic_filter(keys) };			

			//(fill_in(entities, out), ...);


			return entities;
		}

		template<typename T>
		void fill_in(const std::vector<entity>& entities, std::vector<T>& out) const noexcept
		{
			constexpr component_key_t int_id = component_key<T>;
			out.resize(entities.size());
			size_t i = 0;

			for (const auto& e : entities)
			{
				auto eMapIt = m_EntityMap.find(e);
				auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(), [&int_id](const std::pair<component_key_t, size_t>& pair)
				{
					return pair.first == int_id;
				});	

				auto index = foundIt->second;

				auto mem_pair = m_Components.find(int_id);
				void* loc = (void*)((std::uintptr_t)std::get<0>(mem_pair->second).data() + sizeof(T) * index);
				std::memcpy(&out[i], loc, sizeof(T));
				++i;
			}
		}

		void fill_in(const std::vector<entity>& entities, component_key_t int_id, void* out) const noexcept
		{
			/*out.resize(entities.size());
			size_t i = 0;

			for (const auto& e : entities)
			{
				auto eMapIt = m_EntityMap.find(e);
				auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(), [&int_id](const std::pair<component_key_t, size_t>& pair)
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


		uint64_t mID{ 0u };
		/// \brief gets what components this entity uses, and which index it lives on.
		std::unordered_map<entity, std::vector<std::pair<component_key_t, size_t>>> m_EntityMap;

		/// \brief translates components to what entities use it.
		std::unordered_map<component_key_t, std::vector<entity>> m_ComponentMap;

		/// \brief backing memory
		std::unordered_map<component_key_t, std::tuple<memory::raw_region, size_t, psl::IDGenerator<uint64_t>>> m_Components;

		/// overhead is 
		/// sizeof(component) * Nc + sizeof(entity) * Ne +																// store ever component as well as entity
		/// (sizeof(entity) + ((sizeof(size_t) /* component ID */ + sizeof(size_t) /* component index*/) * Nce)) * Ne +	// for every entity store the component ID, and index that it uses
		/// (sizeof(size_t) /* component ID */ + sizeof(entity) * Nec) * nct											// for every component, store the entity that uses them.
		///
		/// where Nc is the count of unique components that exists (not types)
		/// where Ne is the count of the unique entities that exists
		/// where Nce is the count of components this entity has
		/// where Nec is the count of entities that uses this component type.
		/// where Nct is the count of unique component types.

		std::vector<std::function<void(std::chrono::duration<float>)>> m_Systems;

		system_binding currBindings;
	};

	class system
	{
	public:
		system()
		{
			static_assert(std::is_pod<entity>::value, "entity no longer POD");
		}
	private:
	};
}