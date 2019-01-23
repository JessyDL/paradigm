#pragma once
#include <cstdint>
#include <vector>
#include "array_view.h"
#include "entity.h"
#include "component_key.h"
#include "state_operations.h"
#include <numeric>

namespace core::ecs
{
	class state;
	/// \brief contains command_buffer for the core::ecs::state to process at a later time
	///
	/// this class will be passed to all ecs systems that wish to get access to editing
	/// the ECS state while the system is being executed.
	/// There is no promise when these command_buffer get executed, except that they will be synchronized
	/// by the next tick event.
	/// \warn there is an order to the command_buffer, the command with least precedence is the add_components
	/// command_buffer, followed by remove_components command_buffer. After which the create entity command has precedence
	/// and finally the destroy entity. This means adding an entity, adding components, and then destroying it
	/// turns into a no-op.
	/// \warn the precedence of command_buffer persists between several command blocks. This means that even if command
	/// block 'A' adds components, if command block 'B' removes them, it will be as if they never existed.
	/// \warn entities and components that are created and destroyed immediately are not visible to the systems.
	/// you will not receive on_add and other filter instructions from these objects.
	class command_buffer final
	{
		template <typename KeyT, typename ValueT>
		using key_value_container_t = psl::bytell_map<KeyT, ValueT>;
		// using key_value_container_t = std::unordered_map<KeyT, ValueT>;

		friend class ecs::state;
		// only our good friend ecs::state should be able to create us. This is to prevent misuse.
		command_buffer(state& state, uint64_t id_offset);

	  private:
		/// \brief verify the entities exist locally
		///
		/// Entities might exist, but not be present in the command queue's local data containers,
		/// this command makes sure that entities that exist in the ecs::state are then replicated
		/// locally so that components can be added.
		/// \param[in] entities the entity list to verify
		void verify_entities(psl::array_view<entity> entities);

	  public:
		// -----------------------------------------------------------------------------
		// add component
		// -----------------------------------------------------------------------------

		template <typename T>
		void add_component(psl::array_view<entity> entities, T&& _template) noexcept
		{
			if(entities.size() == 0) return;
			verify_entities(entities);
			details::add_component(m_EntityMap, m_Components, entities, std::forward<T>(_template));
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities) noexcept
		{
			return add_component(entities, core::ecs::empty<T>{});
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
			if(entities.size() == 0) return;
			constexpr component_key_t key = details::component_key<details::remove_all<T>>;
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
			if(count == 0) return {};
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
			{
				m_NewEntities.emplace_back(mID);
				m_EntityMap.emplace(++mID, std::vector<std::pair<component_key_t, size_t>>{});
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
			if(count == 0) return {};
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
			{
				m_NewEntities.emplace_back(mID);
				m_EntityMap.emplace(++mID, std::vector<std::pair<component_key_t, size_t>>{});
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
			if(entities.size() == 0) return;
			m_MarkedForDestruction.insert(std::end(m_MarkedForDestruction), std::begin(entities), std::end(entities));
		}

	  private:
		/// \brief applies the changeset to the local data for processing
		///
		/// conflicting command_buffer, such as adding and removing components get resolved locally first
		/// before we process the final command_buffer.
		void apply(size_t id_difference_n);
		std::vector<entity> m_MarkedForDestruction;
		std::vector<entity> m_NewEntities;

		key_value_container_t<component_key_t, std::vector<entity>> m_ErasedComponents;

		// these are reserved for added components only, not to be confused with dynamic editing of components
		key_value_container_t<entity, std::vector<std::pair<component_key_t, size_t>>> m_EntityMap;
		key_value_container_t<component_key_t, details::component_info> m_Components;

		state& m_State;
		uint64_t mID{0u};
		uint64_t m_StartID{0u};
	};
} // namespace core::ecs