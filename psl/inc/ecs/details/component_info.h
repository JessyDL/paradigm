#pragma once
#include "component_key.h"
#include "array_view.h"
#include "static_array.h"
#include "../entity.h"
#include "sparse_array.h"
#include "memory/sparse_array.h"
#include "sparse_indice_array.h"
#include <numeric>
#include "assertions.h"

namespace psl
{
	template <typename... Ts>
	class pack_view;
}

static psl::ecs::details::component_key_t attractor_key;

namespace psl::ecs::details
{
	/// \brief implementation detail that stores the component information
	///
	/// This class serves as a base to the actual component storage.
	/// It contains some primitive functionality itself, but mostly serves
	/// as a safer way of accessing the component data from the state.
	/// \warn this should never be used by anything other than the psl::ecs::state
	/// The 'public' API is not safe to use.
	class component_info
	{
	  public:
		component_info() = default;
		component_info(component_key_t id, size_t size);

		component_info(const component_info& other) = delete;
		component_info(component_info&& other);
		virtual ~component_info() = default;
		component_info& operator=(const component_info& other) = delete;
		component_info& operator							   =(component_info&& other);

		bool is_tag() const noexcept;

		void add(psl::array_view<entity> entities)
		{
			add_impl(entities);
			auto& added = m_Added[m_LockState];
			for(auto e : entities)
			{
				assert_debug_break(!has_component(e) && !has_removed(e));
				added.insert(e);
				m_AssociatedEntities.insert(e);
			}
		}
		void add(entity entity)
		{
			add_impl(entity);
			assert_debug_break(!has_component(entity) && !has_removed(entity));
			m_Added[m_LockState].insert(entity);
			m_AssociatedEntities.insert(entity);
		}
		void add(psl::array_view<std::pair<entity, entity>> entities)
		{
			add_impl(entities);
			auto& added = m_Added[m_LockState];
			for(auto range : entities)
			{
				for(auto e = range.first; e < range.second; ++e)
				{
					assert_debug_break(!has_component(e) && !has_removed(e));
					added.insert(e);
					m_AssociatedEntities.insert(e);
				}
			}
		}
		void destroy(psl::array_view<std::pair<entity, entity>> entities)
		{
			auto& removed = m_Removed[m_LockState];
			for(auto range : entities)
			{

				for(auto e = range.first; e < range.second; ++e)
				{
					removed.insert(e);
					m_AssociatedEntities.erase(e);
				}
			}
		};
		void destroy(psl::array_view<entity> entities) noexcept
		{
			auto& removed = m_Removed[m_LockState];
			for(auto e : entities)
			{
				removed.insert(e);
				m_AssociatedEntities.erase(e);
			}
		}
		void destroy(entity entity) noexcept
		{
			if(!m_AssociatedEntities.has(entity)) return;
			m_AssociatedEntities.erase(entity);
			m_Removed[m_LockState].insert(entity);
		}
		virtual void* data() noexcept = 0;
		bool has_component(entity entity) const noexcept { return m_AssociatedEntities.has(entity); }
		bool has_added(entity entity) const noexcept { return m_Added[0].has(entity); }
		bool has_removed(entity entity) const noexcept { return m_Removed[0].has(entity); }
		virtual bool has_storage_for(entity entity) const noexcept = 0;
		psl::array_view<entity> entities(bool include_removed = false) const noexcept
		{
			return (include_removed) ? entities_impl() : m_AssociatedEntities.indices();
		}

		void purge() noexcept
		{
			purge_impl();
			m_Removed[m_LockState].clear();
		}
		component_key_t id() const noexcept;

		psl::array_view<entity> added_entities() const noexcept { return m_Added[0].indices(); };
		psl::array_view<entity> removed_entities() const noexcept { return m_Removed[0].indices(); };

		void lock() noexcept { m_LockState = 1; }
		void unlock() noexcept { m_LockState = 0; }

		void unlock_and_purge()
		{
			m_LockState = 0;
			purge();

			std::swap(m_Added[0], m_Added[1]);
			std::swap(m_Removed[0], m_Removed[1]);
			m_Added[1].clear();
			m_Removed[1].clear();
		}

		virtual size_t copy_to(psl::array_view<entity> entities, void* destination) const noexcept { return 0; };
		virtual size_t copy_from(psl::array_view<entity> entities, void* source) noexcept { return 0; };

		inline size_t component_size() const noexcept { return m_Size; };
		size_t size(bool include_removed = false) const noexcept
		{
			return (include_removed) ? entities_impl().size() : m_AssociatedEntities.indices().size();
		}
	  protected:
		virtual void purge_impl() noexcept										   = 0;
		virtual void add_impl(entity entity)									   = 0;
		virtual void add_impl(psl::array_view<entity> entities)					   = 0;
		virtual void add_impl(psl::array_view<std::pair<entity, entity>> entities) = 0;
		virtual psl::array_view<entity> entities_impl() const noexcept			   = 0;

	  private:
		size_t m_LockState{0};
		psl::sparse_indice_array<entity> m_AssociatedEntities;
		psl::static_array<psl::sparse_indice_array<entity>, 2> m_Removed;
		psl::static_array<psl::sparse_indice_array<entity>, 2> m_Added;
		component_key_t m_ID;
		size_t m_Size;
	};

	template <typename T, bool tag_type = std::is_empty<T>::value>
	class component_info_typed final : public component_info
	{
	  public:
		component_info_typed() : component_info(details::key_for<T>(), sizeof(T)){};
		memory::sparse_array<T, entity>& entity_data() { return m_Entities; };


		void* data() noexcept override { return m_Entities.data(); }

		bool has_storage_for(entity entity) const noexcept override { return m_Entities.has(entity); }

		size_t copy_to(psl::array_view<entity> entities, void* destination) const noexcept override
		{
			for(auto e : entities)
			{
				std::memcpy(destination, (void*)&(m_Entities.at(e)), sizeof(T));

				destination = (void*)((T*)destination + 1);
			}
			return entities.size() * sizeof(T);
		}
		size_t copy_from(psl::array_view<entity> entities, void* source) noexcept override
		{
			for(auto e : entities)
			{
				if(std::memcmp((void*)&m_Entities[e], source, sizeof(T)) != 0)
					std::memcpy((void*)&m_Entities[e], source, sizeof(T));
				source = (void*)((T*)source + 1);
			}
			return sizeof(T) * entities.size();
		};

	  protected:
		psl::array_view<entity> entities_impl() const noexcept override { return m_Entities.indices(); }
		void add_impl(psl::array_view<entity> entities) override
		{
			m_Entities.reserve(m_Entities.size() + entities.size());
			std::for_each(std::begin(entities), std::end(entities), [this](auto e) { m_Entities.insert(e); });
		}
		void add_impl(entity entity) override { m_Entities.insert(entity); }
		void add_impl(psl::array_view<std::pair<entity, entity>> entities) override
		{
			auto count = std::accumulate(
				std::begin(entities), std::end(entities), size_t{0},
				[](size_t sum, const std::pair<entity, entity>& r) { return sum + (r.second - r.first); });

			m_Entities.reserve(m_Entities.size() + count);
			for(auto range : entities)
			{
				for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
			}
		}
		void purge_impl() noexcept override
		{
			auto entities{removed_entities()};
			std::for_each(std::begin(entities), std::end(entities), [this](auto e) { m_Entities.erase(e); });
		}

	  private:
		memory::sparse_array<T, entity> m_Entities{};
	};

	template <typename T>
	class component_info_typed<T, true> final : public component_info
	{
	  public:
		component_info_typed() : component_info(details::key_for<T>(), 0){};


		void* data() noexcept override { return m_Entities.data(); }

		bool has_storage_for(entity entity) const noexcept override { return m_Entities.has(entity); }

	  protected:
		psl::array_view<entity> entities_impl() const noexcept override { return m_Entities.indices(); }
		void add_impl(psl::array_view<entity> entities) override
		{
			for(auto e : entities) m_Entities.insert(e);
		}
		void add_impl(entity entity) override { m_Entities.insert(entity); }
		void add_impl(psl::array_view<std::pair<entity, entity>> entities) override
		{
			for(auto range : entities)
			{
				for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
			}
		}
		void purge_impl() noexcept override
		{
			auto entities{removed_entities()};
			for(auto e : entities) m_Entities.erase(e);
		}

	  private:
		  memory::sparse_array<int, entity> m_Entities{};
	};
} // namespace psl::ecs::details

namespace std
{
	template <>
	struct hash<psl::ecs::details::component_info>
	{
		std::size_t operator()(psl::ecs::details::component_info const& ci) const noexcept
		{
			static_assert(sizeof(size_t) == sizeof(psl::ecs::details::component_key_t), "should be castable");
			return (size_t)ci.id();
		}
	};
} // namespace std