#pragma once
#include "component_key.h"
#include "array_view.h"
#include "../entity.h"
#include "sparse_array.h"
#include "memory/sparse_array.h"


namespace psl
{
	template <typename... Ts>
	class pack_view;
}

namespace psl::ecs::details
{
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

		virtual void add(psl::array_view<entity> entities)								   = 0;
		virtual void add(psl::array_view<std::pair<entity, entity>> entities)			   = 0;
		virtual void destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept = 0;
		virtual void destroy(psl::array_view<entity> entities) noexcept					   = 0;
		virtual void destroy(entity entity) noexcept									   = 0;
		virtual void* data() noexcept													   = 0;
		virtual psl::array_view<entity> entities() const noexcept						   = 0;
		virtual bool has_component(entity entity) const noexcept						   = 0;

		component_key_t id() const noexcept;


	  private:
		component_key_t m_ID;
		size_t m_Size;
	};

	template <typename T, bool tag_type = std::is_empty<T>::value>
	class component_info_typed final : public component_info
	{
	  public:
		component_info_typed() : component_info(details::key_for<T>(), sizeof(T)){};
		psl::memory::sparse_array<T, entity>& entity_data() { return m_Entities; };

		void add(psl::array_view<entity> entities) override
		{
			m_Entities.reserve(m_Entities.size() + entities.size());
			for(auto e : entities) m_Entities.insert(e);
		}
		void add(psl::array_view<std::pair<entity, entity>> entities) override
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
		void destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept override
		{
			for(auto range : entities)
			{
				m_Entities.erase(range.first, range.second);
			}
		}
		void destroy(psl::array_view<entity> entities) noexcept override
		{
			for(auto e : entities) m_Entities.erase(e);
		}
		void destroy(entity entity) noexcept override { m_Entities.erase(entity); }
		void* data() noexcept override { return m_Entities.data(); }
		psl::array_view<entity> entities() const noexcept override { return m_Entities.indices(); }

		bool has_component(entity entity) const noexcept override { return m_Entities.has(entity); }

	  private:
		psl::memory::sparse_array<T, entity> m_Entities{};
	};

	template <typename T>
	class component_info_typed<T, true> final : public component_info
	{
	  public:
		component_info_typed() : component_info(details::key_for<T>(), 0){};
		psl::sparse_array<T, entity>& entity_data() { throw std::runtime_error("there is no data to a tag type"); };

		void add(psl::array_view<entity> entities) override
		{
			for(auto e : entities) m_Entities.insert(e);
		}
		void add(psl::array_view<std::pair<entity, entity>> entities) override
		{
			for(auto range : entities)
			{
				for(auto e = range.first; e < range.second; ++e) m_Entities.insert(e);
			}
		}
		void destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept override
		{
			for(auto range : entities)
			{
				m_Entities.erase(range.first, range.second);
			}
		}
		void destroy(psl::array_view<entity> entities) noexcept override
		{
			for(auto e : entities) m_Entities.erase(e);
		}
		void destroy(entity entity) noexcept override { m_Entities.erase(entity); }
		void* data() noexcept override { return m_Entities.data(); }
		psl::array_view<entity> entities() const noexcept override { return m_Entities.indices(); }

		bool has_component(entity entity) const noexcept override { return m_Entities.has(entity); }

	  private:
		psl::sparse_array<int, entity> m_Entities{};
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