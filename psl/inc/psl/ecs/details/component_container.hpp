#pragma once
#include "../details/staged_sparse_array.hpp"
#include "../details/staged_sparse_memory_region.hpp"
#include "../entity.hpp"
#include "component_key.hpp"
#include "psl/array_view.hpp"
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
	/// \brief implementation detail that stores the component data
	///
	/// This class serves as a base to the actual component storage.
	/// It contains some primitive functionality itself, but mostly serves
	/// as a safer way of accessing the component data from the state.
	/// \warning this should never be used by anything other than the psl::ecs::state_t
	/// The 'public' API is not safe to use.
	class component_container_t
	{
	  public:
		component_container_t(component_key_t id, size_t size);

		component_container_t(const component_container_t& other) = delete;
		component_container_t(component_container_t&& other);
		virtual ~component_container_t()							   = default;
		component_container_t& operator=(const component_container_t& other) = delete;
		component_container_t& operator=(component_container_t&& other);

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
		bool has_component(entity entity) const noexcept { return has_impl(entity, stage_range_t::ALIVE); }
		bool has_added(entity entity) const noexcept { return has_impl(entity, stage_range_t::ADDED); }
		bool has_removed(entity entity) const noexcept { return has_impl(entity, stage_range_t::REMOVED); }
		virtual bool has_storage_for(entity entity) const noexcept = 0;
		psl::array_view<entity> entities(bool include_removed = false) const noexcept
		{
			return entities_impl((include_removed) ? stage_range_t::ALL : stage_range_t::ALIVE);
		}

		void purge() noexcept { purge_impl(); }
		component_key_t id() const noexcept;

		psl::array_view<entity> added_entities() const noexcept { return entities_impl(stage_range_t::ADDED); };
		psl::array_view<entity> removed_entities() const noexcept { return entities_impl(stage_range_t::REMOVED); };

		virtual size_t copy_to(psl::array_view<entity> entities, void* destination) const noexcept { return 0; };
		virtual size_t copy_from(psl::array_view<entity> entities, void* source, bool repeat = false) noexcept
		{
			return 0;
		};

		inline size_t component_size() const noexcept { return m_Size; };
		size_t size(bool include_removed = false) const noexcept
		{
			return entities_impl((include_removed) ? stage_range_t::ALL : stage_range_t::ALIVE).size();
		}

		void set(entity entity, void* data) noexcept { set_impl(entity, data); }

		virtual void remap(const psl::sparse_array<entity>& mapping, std::function<bool(entity)> pred) noexcept = 0;
		virtual bool merge(const component_container_t& other) noexcept												= 0;
		virtual size_t alignment() const noexcept																= 0;

	  protected:
		virtual void purge_impl() noexcept																	= 0;
		virtual void add_impl(entity entity, void* data)													= 0;
		virtual void add_impl(psl::array_view<entity> entities, void* data, bool repeat)					= 0;
		virtual void add_impl(psl::array_view<std::pair<entity, entity>> entities, void* data, bool repeat) = 0;
		virtual psl::array_view<entity> entities_impl(stage_range_t stage) const noexcept	= 0;
		virtual void set_impl(entity entity, void* data) noexcept											= 0;
		virtual void remove_impl(entity entity)																= 0;
		virtual void remove_impl(psl::array_view<entity> entities)											= 0;
		virtual void remove_impl(psl::array_view<std::pair<entity, entity>> entities)						= 0;
		virtual bool has_impl(entity entity, stage_range_t stage) const noexcept							= 0;

	  protected:
		component_key_t m_ID;
		size_t m_Size;
	};

	template <typename T, bool tag_type = std::is_empty<T>::value>
	class component_container_typed_t final : public component_container_t
	{
	  public:
		component_container_typed_t() : component_container_t(details::component_key_t::generate<T>(), sizeof(T)) {};
		auto& entity_data() noexcept { return m_Entities; };


		void* data() noexcept override { return m_Entities.data(); }

		bool has_storage_for(entity entity) const noexcept override
		{
			return m_Entities.has(entity, stage_range_t::ALL);
		}

		size_t alignment() const noexcept override { return std::alignment_of_v<T>; }

		size_t copy_to(psl::array_view<entity> entities, void* destination) const noexcept override
		{
			psl_assert((std::uintptr_t)destination % alignment() == 0, "pointer has to be aligned");
			T* dest = (T*)destination;
			for(auto e : entities)
			{
				std::memcpy(dest, m_Entities.addressof(e, stage_range_t::ALL), sizeof(T));
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
					std::memcpy((void*)&(m_Entities.at(e, stage_range_t::ALL)), src, sizeof(T));
				}
			}
			else
			{
				for(auto e : entities)
				{
					std::memcpy((void*)&(m_Entities.at(e, stage_range_t::ALL)), src, sizeof(T));
					++src;
				}
			}
			return sizeof(T) * entities.size();
		};

		void remap(const psl::sparse_array<entity>& mapping, std::function<bool(entity)> pred) noexcept override
		{
			m_Entities.remap(mapping, pred);
		}
		bool merge(const component_container_t& other) noexcept override
		{
			if(other.id() != id()) return false;

			component_container_typed_t<T>* other_ptr = (component_container_typed_t<T>*)(&other);
			m_Entities.merge(other_ptr->m_Entities);
			return true;
		}

		void set(entity e, const T& data) noexcept { m_Entities.at(e, stage_range_t::ALL) = data; }

	  protected:
		void set_impl(entity entity, void* data) noexcept override
		{
			m_Entities.at(entity, stage_range_t::ALL) = *(T*)data;
		}
		psl::array_view<entity> entities_impl(stage_range_t stage) const noexcept override
		{
			return m_Entities.indices(stage);
		}
		void add_impl(psl::array_view<entity> entities, void* data, bool repeat) override
		{
			m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + entities.size());
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

			m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + count);
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
		bool has_impl(entity entity, stage_range_t stage) const noexcept override
		{
			return m_Entities.has(entity, stage);
		}

	  private:
		details::staged_sparse_array<T, entity> m_Entities;
	};

	class component_container_flag_t : public component_container_t
	{
	  public:
		component_container_flag_t(psl::ecs::details::component_key_t key) : component_container_t(key, 0) {};

		void* data() noexcept override { return nullptr; }

		bool has_storage_for(entity entity) const noexcept override { return m_Entities.has(entity, stage_range_t::ALL); }

		size_t alignment() const noexcept override { return 1; }

		void remap(const psl::sparse_array<entity>& mapping, std::function<bool(entity)> pred) noexcept override
		{
			m_Entities.remap(mapping, pred);
		}

		bool merge(const component_container_t& other) noexcept override
		{
			if(other.id() != id()) return false;

			component_container_flag_t* other_ptr = (component_container_flag_t*)(&other);
			m_Entities.merge(other_ptr->m_Entities);
			return true;
		}

	  protected:
		void set_impl(entity entity, void* data) noexcept {};
		psl::array_view<entity> entities_impl(stage_range_t stage) const noexcept override
		{
			return m_Entities.indices(stage);
		}
		void add_impl(psl::array_view<entity> entities, void* data, bool repeat) override
		{
			m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + entities.size());
			for(auto e : entities) m_Entities.insert(e);
		}
		void add_impl(entity entity, void* data) override { m_Entities.insert(entity); }
		void add_impl(psl::array_view<std::pair<entity, entity>> entities, void* data, bool repeat) override
		{
			auto count = std::accumulate(
			  std::begin(entities), std::end(entities), size_t {0}, [](size_t sum, const std::pair<entity, entity>& r) {
				  return sum + (r.second - r.first);
			  });

			m_Entities.reserve(m_Entities.size(stage_range_t::ALL) + count);
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
		bool has_impl(entity entity, stage_range_t stage) const noexcept override
		{
			return m_Entities.has(entity, stage);
		}

	  private:
		details::staged_sparse_array<void, entity> m_Entities;
	};

	class component_container_untyped_t : public component_container_t
	{
		using stage_range_t = details::stage_range_t;

	  public:
		component_container_untyped_t(psl::ecs::details::component_key_t key, size_t size, size_t alignment) :
			component_container_t(key, size), m_Entities(size), m_Alignment(alignment) {};

		auto& entity_data() noexcept { return m_Entities; };


		void* data() noexcept override { return m_Entities.data(); }

		bool has_storage_for(entity entity) const noexcept override
		{
			return m_Entities.has(entity, stage_range_t::ALL);
		}

		size_t alignment() const noexcept override { return m_Alignment; }

		size_t copy_to(psl::array_view<entity> entities, void* destination) const noexcept override
		{
			psl_assert((std::uintptr_t)destination % alignment() == 0, "pointer has to be aligned");
			std::byte* dest = (std::byte*)destination;
			for(auto e : entities)
			{
				std::memcpy(dest, m_Entities.addressof(e, stage_range_t::ALL), m_Size);
				dest += m_Size;
			}
			return entities.size() * m_Size;
		}
		size_t copy_from(psl::array_view<entity> entities, void* source, bool repeat) noexcept override
		{
			psl_assert((std::uintptr_t)source % alignment() == 0, "pointer has to be aligned");
			std::byte* src = (std::byte*)source;
			if(repeat)
			{
				for(auto e : entities)
				{
					std::memcpy(m_Entities.addressof(e, stage_range_t::ALL), src, m_Size);
				}
			}
			else
			{
				for(auto e : entities)
				{
					std::memcpy(m_Entities.addressof(e, stage_range_t::ALL), src, m_Size);
					src += m_Size;
				}
			}
			return m_Size * entities.size();
		};

		void remap(const psl::sparse_array<entity>& mapping, std::function<bool(entity)> pred) noexcept override
		{
			m_Entities.remap(mapping, pred);
		}

		bool merge(const component_container_t& other) noexcept override
		{
			if(other.id() != id()) return false;

			component_container_untyped_t* other_ptr = (component_container_untyped_t*)(&other);
			return m_Entities.merge(other_ptr->m_Entities).success;
		}

		template <typename T>
		void set(entity e, const T& data) noexcept
		{
			m_Entities.template at<T>(e, stage_range_t::ALL) = data;
		}

	  protected:
		void set_impl(entity entity, void* data) noexcept override
		{
			auto* ptr = m_Entities.addressof(entity, stage_range_t::ALL);
			std::memcpy(ptr, data, m_Size);
		}
		psl::array_view<entity> entities_impl(stage_range_t stage) const noexcept override
		{
			return m_Entities.indices(stage);
		}

		void add_impl(psl::array_view<entity> entities, void* data, bool repeat) override
		{
			std::byte* source = (std::byte*)data;
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
					auto ptr = m_Entities.get_or_insert(entities[i]);
					memcpy(ptr, source, m_Size);
				}
			}
			else
			{
				for(size_t i = 0; i < entities.size(); ++i)
				{
					auto ptr = m_Entities.get_or_insert(entities[i]);
					memcpy(ptr, source, m_Size);
					source += m_Size;
				}
			}
		}
		void add_impl(entity entity, void* data) override
		{
			auto ptr = m_Entities.get_or_insert(entity);

			if(data != nullptr) memcpy(ptr, data, m_Size);
		}

		void add_impl(psl::array_view<std::pair<entity, entity>> entities, void* data, bool repeat) override
		{
			auto count = std::accumulate(
			  std::begin(entities), std::end(entities), size_t {0}, [](size_t sum, const std::pair<entity, entity>& r) {
				  return sum + (r.second - r.first);
			  });

			std::byte* source = (std::byte*)data;
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
					for(auto e = range.first; e < range.second; ++e)
					{
						auto ptr = m_Entities.get_or_insert(e);
						memcpy(ptr, source, m_Size);
					}
				}
			}
			else
			{
				for(auto range : entities)
				{
					for(auto e = range.first; e < range.second; ++e)
					{
						auto ptr = m_Entities.get_or_insert(e);
						memcpy(ptr, source, m_Size);
						source += m_Size;
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
		bool has_impl(entity entity, stage_range_t stage) const noexcept override
		{
			return m_Entities.has(entity, stage);
		}

	  private:
		details::staged_sparse_memory_region_t m_Entities;
		size_t m_Alignment {};
	};

	template <details::IsValidForStagedSparseMemoryRange T>
	class component_container_typed_t<T, false> final : public component_container_untyped_t
	{
	  public:
		component_container_typed_t() :
			component_container_untyped_t(details::component_key_t::generate<T>(), sizeof(T), std::alignment_of_v<T>)
		{}
	};

	template <typename T>
	class component_container_typed_t<T, true> final : public component_container_flag_t
	{
	  public:
		component_container_typed_t() : component_container_flag_t(details::component_key_t::generate<T>()) {}
	};
}	 // namespace psl::ecs::details

namespace std
{
	template <>
	struct hash<psl::ecs::details::component_container_t>
	{
		std::size_t operator()(psl::ecs::details::component_container_t const& ci) const noexcept
		{
			return std::hash<psl::ecs::details::component_key_t> {}(ci.id());
		}
	};
}	 // namespace std
