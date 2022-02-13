#pragma once
#include "../details/staged_sparse_array.hpp"
#include "../entity.hpp"
#include "component_key.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/memory/sparse_array.hpp"
#include "psl/sparse_array.hpp"
#include "psl/sparse_indice_array.hpp"
#include "psl/static_array.hpp"
#include <functional>
#include <numeric>
namespace psl
{
	template <typename... Ts>
	class pack_view;
}

namespace psl::ecs::details
{
	/// \brief implementation detail that stores the component information
	///
	/// This class serves as a base to the actual component storage.
	/// It contains some primitive functionality itself, but mostly serves
	/// as a safer way of accessing the component data from the state.
	/// \warning this should never be used by anything other than the psl::ecs::state_t
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

		void add(psl::array_view<entity> entities, void* data = nullptr, bool repeat = false)
		{
			add_impl(entities, data, repeat);
		}
		void add(entity entity, void* data = nullptr) { add_impl(entity, data); }
		void add(psl::array_view<std::pair<entity, entity>> entities, void* data = nullptr, bool repeat = false)
		{
			add_impl(entities, data, repeat);
		}
		void destroy(psl::array_view<std::pair<entity, entity>> entities) { remove_impl(entities); };
		void destroy(psl::array_view<entity> entities) noexcept { remove_impl(entities); }
		void destroy(entity entity) noexcept { remove_impl(entity); }
		virtual void* data() noexcept = 0;
		bool has_component(entity entity) const noexcept { return has_impl(entity, 0, 1); }
		bool has_added(entity entity) const noexcept { return has_impl(entity, 1, 1); }
		bool has_removed(entity entity) const noexcept { return has_impl(entity, 2, 2); }
		virtual bool has_storage_for(entity entity) const noexcept = 0;
		psl::array_view<entity> entities(bool include_removed = false) const noexcept
		{
			return entities_impl(0, (include_removed) ? 2 : 1);
		}

		void purge() noexcept { purge_impl(); }
		component_key_t id() const noexcept;

		psl::array_view<entity> added_entities() const noexcept { return entities_impl(1, 1); };
		psl::array_view<entity> removed_entities() const noexcept { return entities_impl(2, 2); };

		virtual size_t copy_to(psl::array_view<entity> entities, void* destination) const noexcept { return 0; };
		virtual size_t copy_from(psl::array_view<entity> entities, void* source, bool repeat = false) noexcept
		{
			return 0;
		};

		inline size_t component_size() const noexcept { return m_Size; };
		size_t size(bool include_removed = false) const noexcept
		{
			return entities_impl(0, (include_removed) ? 2 : 1).size();
		}

		void set(entity entity, void* data) noexcept { set_impl(entity, data); }

		virtual void remap(const psl::sparse_array<entity>& mapping, std::function<bool(entity)> pred) noexcept = 0;
		virtual bool merge(const component_info& other) noexcept												= 0;
		virtual size_t alignment() const noexcept																= 0;


	  protected:
		virtual void purge_impl() noexcept																	= 0;
		virtual void add_impl(entity entity, void* data)													= 0;
		virtual void add_impl(psl::array_view<entity> entities, void* data, bool repeat)					= 0;
		virtual void add_impl(psl::array_view<std::pair<entity, entity>> entities, void* data, bool repeat) = 0;
		virtual psl::array_view<entity> entities_impl(size_t startStage, size_t endStage) const noexcept	= 0;
		virtual void set_impl(entity entity, void* data) noexcept											= 0;
		virtual void remove_impl(entity entity)																= 0;
		virtual void remove_impl(psl::array_view<entity> entities)											= 0;
		virtual void remove_impl(psl::array_view<std::pair<entity, entity>> entities)						= 0;
		virtual bool has_impl(entity entity, size_t startStage, size_t endStage) const noexcept				= 0;

	  private:
		// size_t m_LockState{0};
		// psl::sparse_indice_array<entity> m_AssociatedEntities;
		// psl::sparse_indice_array<entity> m_Removed;
		// psl::sparse_indice_array<entity> m_Added;
		component_key_t m_ID;
		size_t m_Size;
	};

	template <typename T, bool tag_type = std::is_empty<T>::value>
	class component_info_typed final : public component_info
	{
	  public:
		component_info_typed() : component_info(details::key_for<T>(), sizeof(T)) {};
		auto& entity_data() noexcept { return m_Entities; };


		void* data() noexcept override { return m_Entities.data(); }

		bool has_storage_for(entity entity) const noexcept override { return m_Entities.has(entity, 0, 2); }

		size_t alignment() const noexcept override { return std::alignment_of_v<T>; }

		size_t copy_to(psl::array_view<entity> entities, void* destination) const noexcept override
		{
			psl_assert((std::uintptr_t)destination % alignment() == 0, "pointer has to be aligned");
			T* dest = (T*)destination;
			for(auto e : entities)
			{
				std::memcpy(dest, m_Entities.addressof(e, 0, 2), sizeof(T));
				++dest;
			}
			return entities.size() * sizeof(T);
		}
		size_t copy_from(psl::array_view<entity> entities, void* source, bool repeat) noexcept override
		{
			psl_assert((std::uintptr_t)source % alignment() == 0, "pointer has to be aligned");
			T* src = (T*)source;
			if(repeat)
			{
				for(auto e : entities)
				{
					std::memcpy((void*)&(m_Entities.at(e, 0, 2)), src, sizeof(T));
				}
			}
			else
			{
				for(auto e : entities)
				{
					// if(std::memcmp((void*)&(m_Entities.at(e, 0, 2)), src, sizeof(T)) != 0)
					std::memcpy((void*)&(m_Entities.at(e, 0, 2)), src, sizeof(T));
					++src;
				}
			}
			return sizeof(T) * entities.size();
		};

		void remap(const psl::sparse_array<entity>& mapping, std::function<bool(entity)> pred) noexcept override
		{
			m_Entities.remap(mapping, pred);
		}
		bool merge(const component_info& other) noexcept override
		{
			if(other.id() != id()) return false;

			component_info_typed<T>* other_ptr = (component_info_typed<T>*)(&other);
			m_Entities.merge(other_ptr->m_Entities);
			return true;
		}

		void set(entity e, const T& data) noexcept { m_Entities.at(e, 0, 2) = data; }

	  protected:
		void set_impl(entity entity, void* data) noexcept override { m_Entities.at(entity, 0, 2) = *(T*)data; }
		psl::array_view<entity> entities_impl(size_t startStage, size_t endStage) const noexcept override
		{
			return m_Entities.indices(startStage, endStage);
		}
		void add_impl(psl::array_view<entity> entities, void* data, bool repeat) override
		{
			m_Entities.reserve(m_Entities.size(0, 2) + entities.size());
			T* source = (T*)data;
			if(data == nullptr)
			{
				for(size_t i = 0; i < entities.size(); ++i)
				{
					m_Entities.insert(entities[i]);
				}
			}
			else if(repeat)
			{
				for(size_t i = 0; i < entities.size(); ++i)
				{
					m_Entities.insert(entities[i], *source);
				}
			}
			else
			{
				for(size_t i = 0; i < entities.size(); ++i)
				{
					m_Entities.insert(entities[i], *source);
					++source;
				}
			}
		}
		void add_impl(entity entity, void* data) override
		{
			if(data == nullptr)
				m_Entities.insert(entity);
			else
				m_Entities.insert(entity, *(T*)data);
		}
		void add_impl(psl::array_view<std::pair<entity, entity>> entities, void* data, bool repeat) override
		{
			auto count = std::accumulate(
			  std::begin(entities), std::end(entities), size_t {0}, [](size_t sum, const std::pair<entity, entity>& r) {
				  return sum + (r.second - r.first);
			  });

			m_Entities.reserve(m_Entities.size(0, 2) + count);
			T* source = (T*)data;
			if(data == nullptr)
			{
				for(auto range : entities)
				{
					for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
				}
			}
			else if(repeat)
			{
				for(auto range : entities)
				{
					for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e, *source);
				}
			}
			else
			{
				for(auto range : entities)
				{
					for(auto e = range.first; e < range.second; ++e)
					{
						m_Entities.insert(e, *source);
						++source;
					}
				}
			}
		}
		void purge_impl() noexcept override { m_Entities.promote(); }

		void remove_impl(entity entity) override { m_Entities.erase(entity); }
		void remove_impl(psl::array_view<entity> entities) override
		{
			for(size_t i = 0; i < entities.size(); ++i) m_Entities.erase(entities[i]);
		}
		void remove_impl(psl::array_view<std::pair<entity, entity>> entities) override
		{
			for(auto range : entities)
			{
				for(auto i = range.first; i < range.second; ++i) m_Entities.erase(i);
			}
		}
		bool has_impl(entity entity, size_t startStage, size_t endStage) const noexcept override
		{
			return m_Entities.has(entity, startStage, endStage);
		}

	  private:
		details::staged_sparse_array<T, entity> m_Entities;
	};

	template <typename T>
	class component_info_typed<T, true> final : public component_info
	{
	  public:
		component_info_typed() : component_info(details::key_for<T>(), 0) {};


		void* data() noexcept override { return nullptr; }

		bool has_storage_for(entity entity) const noexcept override { return m_Entities.has(entity, 0, 2); }

		size_t alignment() const noexcept override { return 1; }

		void remap(const psl::sparse_array<entity>& mapping, std::function<bool(entity)> pred) noexcept override
		{
			m_Entities.remap(mapping, pred);
		}

		bool merge(const component_info& other) noexcept override
		{
			if(other.id() != id()) return false;

			component_info_typed<T>* other_ptr = (component_info_typed<T>*)(&other);
			m_Entities.merge(other_ptr->m_Entities);
			return true;
		}

	  protected:
		void set_impl(entity entity, void* data) noexcept {};
		psl::array_view<entity> entities_impl(size_t startStage, size_t endStage) const noexcept override
		{
			return m_Entities.indices(startStage, endStage);
		}
		void add_impl(psl::array_view<entity> entities, void* data, bool repeat) override
		{
			m_Entities.reserve(m_Entities.size(0, 2) + entities.size());
			for(auto e : entities) m_Entities.insert(e);
		}
		void add_impl(entity entity, void* data) override { m_Entities.insert(entity); }
		void add_impl(psl::array_view<std::pair<entity, entity>> entities, void* data, bool repeat) override
		{
			auto count = std::accumulate(
			  std::begin(entities), std::end(entities), size_t {0}, [](size_t sum, const std::pair<entity, entity>& r) {
				  return sum + (r.second - r.first);
			  });

			m_Entities.reserve(m_Entities.size(0, 2) + count);
			for(auto range : entities)
			{
				for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
			}
		}
		void purge_impl() noexcept override { m_Entities.promote(); }

		void remove_impl(entity entity) override { m_Entities.erase(entity); }
		void remove_impl(psl::array_view<entity> entities) override
		{
			for(size_t i = 0; i < entities.size(); ++i) m_Entities.erase(entities[i]);
		}
		void remove_impl(psl::array_view<std::pair<entity, entity>> entities) override
		{
			for(auto range : entities)
			{
				for(auto i = range.first; i < range.second; ++i) m_Entities.erase(i);
			}
		}
		bool has_impl(entity entity, size_t startStage, size_t endStage) const noexcept override
		{
			return m_Entities.has(entity, startStage, endStage);
		}

	  private:
		details::staged_sparse_array<void, entity> m_Entities;
	};
}	 // namespace psl::ecs::details

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
}	 // namespace std