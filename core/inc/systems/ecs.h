#pragma once
#include "stdafx.h"


namespace core::systems::ecs
{
	/// ----------------------------------------------------------------------------------------------
	/// jmp_buffer
	/// ----------------------------------------------------------------------------------------------

	template<typename T>
	class range
	{
	public:
		class iterator
		{
			friend class range;
			iterator(T* target, std::vector<size_t>::iterator index, std::vector<size_t>::const_iterator end) : target(target), index(index), end(end)
			{

			}
		public:
			T& operator*()
			{
				return *target;
			}

			bool operator!=(iterator other)
			{
				return index != other.index;
			}

			iterator operator++() const
			{
				std::uintptr_t jmp_size = (*std::next(index) - *index) * sizeof(T);
				return iterator{ (T*)((std::uintptr_t)target + jmp_size), std::next(index), end };
			}

			iterator& operator++()
			{
				auto next = std::next(index);
				if (end == next)
				{
					target = (T*)((std::uintptr_t)target + sizeof(T));
					index = next;
					return *this;
				}
				std::uintptr_t jmp_size = (*next - *index) * sizeof(T);
				target = (T*)((std::uintptr_t)target + jmp_size);
				index = next;
				return *this;
			}
		private:
			T* target;
			std::vector<size_t>::iterator index;
			std::vector<size_t>::const_iterator end;
		};

		range(std::vector<T>& target, const std::vector<size_t>& indices)
			: m_Target(&target), m_Indices(indices)
		{
			std::sort(m_Indices.begin(), m_Indices.end());
		};

		range() : m_Target(nullptr), m_Indices({})
		{

		}


		T& operator[](size_t index)
		{
			return (*m_Target)[m_Indices[index]];
		}

		iterator begin()
		{
			size_t index = *m_Indices.begin();
			return iterator{ &(*m_Target)[index], m_Indices.begin(), m_Indices.end() };
		}

		iterator end()
		{
			size_t index = *std::prev(m_Indices.end());
			return iterator{ (T*)((std::uintptr_t)&(*m_Target)[index] + sizeof(T)), m_Indices.end() , m_Indices.end() };
		}
	private:
		std::vector<T>* m_Target{ nullptr };
		std::vector<size_t> m_Indices;
	};

	/// ----------------------------------------------------------------------------------------------
	/// Entity
	/// ----------------------------------------------------------------------------------------------
	struct entity;
	static bool valid(entity e) noexcept;

	/// \brief entity points to a collection of components
	struct entity
	{
	public:
		entity() = default;
		entity(uint64_t id) : m_ID(id) {};
		~entity() = default;
		entity(const entity&) = default;
		entity& operator=(const entity&) = default;
		entity(entity&&) = default;
		entity& operator=(entity&&) = default;

		bool valid() const noexcept
		{
			return m_ID != 0u;
		}
		friend static bool valid(entity e) noexcept;
		bool operator==(entity b) const noexcept
		{
			return m_ID == b.m_ID;
		}
		bool operator<=(entity b) const noexcept
		{
			return m_ID <= b.m_ID;
		}
		bool operator!=(entity b) const noexcept
		{
			return m_ID != b.m_ID;
		}
		bool operator>=(entity b) const noexcept
		{
			return m_ID >= b.m_ID;
		}
		bool operator<(entity b) const noexcept
		{
			return m_ID < b.m_ID;
		}
		bool operator>(entity b) const noexcept
		{
			return m_ID > b.m_ID;
		}

		uint64_t id() const noexcept { return m_ID; };
	private:
		uint64_t m_ID;
	};

	/// \brief checks if an entity is valid or not
	/// \param[in] e the entity to check
	static bool valid(entity e) noexcept
	{
		return e.m_ID != 0u;
	};
}

namespace std
{
	template <>
	struct hash<core::systems::ecs::entity>
	{
		std::size_t operator()(core::systems::ecs::entity const& e) const noexcept
		{
			return e.id();
		}
	};
} // namespace std

namespace core::systems::ecs
{


	/// ----------------------------------------------------------------------------------------------
	/// container
	/// ----------------------------------------------------------------------------------------------
	class container
	{

	};

	class state
	{
		// https://stackoverflow.com/questions/7562096/compile-time-constant-id/39640960#39640960
		template<typename>
		static void component_key() {};

		using component_key_t = void(*)();

	public:

		template<typename T, typename ... Ts>
		bool add_component(entity e, Ts&&... args)
		{
			static_assert(std::is_pod<T>::value, "the component type must be a POD (std::is_pod<T>::value == true)");
			constexpr component_key_t int_id = component_key<T>;

			auto eMapIt = m_EntityMap.find(e);
			if (eMapIt == std::end(m_EntityMap))
				return false;
			for (auto eComp : eMapIt->second)
			{
				if (eComp.first == int_id)
					return false;
			}

			auto it = m_Components.find(int_id);
			if (it == m_Components.end())
			{
				m_Components.emplace(int_id, std::pair<memory::region, std::vector<memory::segment>>{ memory::region{ 1024 * 1024 * 1024, sizeof(int), new memory::default_allocator(true) }, std::vector<memory::segment>{} });
				it = m_Components.find(int_id);
			}
			auto& pair{ it->second };

			auto res = pair.second.emplace_back(pair.first.allocate(sizeof(T)).value());
			res.set<T>(T{ std::forward<Ts>(args)... });
			m_EntityMap[e].emplace_back(int_id, pair.second.size() - 1);
			m_ComponentMap[int_id].emplace_back(e);
			return true;
		}

		template<typename T>
		bool remove_component(entity e)
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

			auto mem_pair = m_Components.find(int_id);
			// todo: remove segment;

			return true;
		}

		entity create()
		{
			return m_EntityMap.emplace(entity{ ++mID }, std::vector<std::pair<component_key_t, size_t>>{}).first->first;
		}

		bool destroy(entity e)
		{
			if (auto eMapIt = m_EntityMap.find(e); eMapIt != std::end(m_EntityMap))
			{
				for (auto& pair : eMapIt->second)
				{
					auto type = pair.first;
					auto index = pair.second;

					if (auto cMapIt = m_ComponentMap.find(pair.first); cMapIt != std::end(m_ComponentMap))
					{
						cMapIt->second.erase(std::remove(cMapIt->second.begin(), cMapIt->second.end(), e), cMapIt->second.end());
					}
					// todo: remove segment;
				}
				return true;
			}
			return false;
		}

		template<typename... Ts>
		std::vector<entity> filter()
		{
			static const std::vector<component_key_t> keys{ {component_key<Ts>...} };
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");

			std::vector<entity> v_intersection{ m_ComponentMap[keys[0]] };

			for (size_t i = 1; i < keys.size(); ++i)
			{
				std::vector<entity> intermediate;
				const auto& it = m_ComponentMap[keys[i]];
				std::set_intersection(v_intersection.begin(), v_intersection.end(),
					it.begin(), it.end(),
					std::back_inserter(intermediate));
				v_intersection = intermediate;
			}

			return v_intersection;
		}

		template<typename... Ts>
		std::vector<entity> filter(std::vector<Ts>&... out)
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
			return *(T*)mem_pair->second.second[index].range().begin;
		}

	private:
		template<typename T>
		void fill_in(const std::vector<entity>& entities, std::vector<T>& out)
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
				auto range = mem_pair->second.second[index].range();
				std::memcpy(&out[i], (void*)range.begin, sizeof(T));
				++i;
			}
		}


		uint64_t mID{ 0u };
		/// \brief gets what components this entity uses, and which index it lives on.
		std::unordered_map<entity, std::vector<std::pair<component_key_t, size_t>>> m_EntityMap;

		/// \brief translates components to what entities use it.
		std::unordered_map<component_key_t, std::vector<entity>> m_ComponentMap;

		/// \brief backing memory
		std::unordered_map<component_key_t, std::pair<memory::region, std::vector<memory::segment>>> m_Components;

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

	};
	
	class system
	{
	public:
		system()
		{
			static_assert(std::is_pod<entity>::value, "entity no longer POD");
		}
	private:
		container m_Container;
	};
}
