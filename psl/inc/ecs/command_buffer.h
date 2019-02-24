#pragma once
#include "entity.h"
#include "details/component_key.h"
#include "details/component_info.h"
#include "array_view.h"
#include "array.h"
#include "vector.h"
#include "unique_ptr.h"
#include "sparse_indice_array.h"
#include "memory/sparse_array.h"
#include "sparse_array.h"
namespace psl::ecs
{
	class state;

	namespace details
	{
		class command_buffer_component_info
		{
		  public:
			command_buffer_component_info() = default;
			command_buffer_component_info(entity first, component_key_t id, size_t size)
				: m_First(first), m_ID(id), m_Size(size){};

			command_buffer_component_info(const command_buffer_component_info& other) = delete;
			command_buffer_component_info(command_buffer_component_info&& other)	  = default;
			virtual ~command_buffer_component_info()								  = default;
			command_buffer_component_info& operator=(const command_buffer_component_info& other) = delete;
			command_buffer_component_info& operator=(command_buffer_component_info&& other) = default;

			bool is_tag() const noexcept { return m_Size == 0; };

			void add(psl::array_view<entity> entities)
			{
				m_Changes = entities.size() > 0;
				add_impl(entities);
				auto& added = m_Added;
				for(auto e : entities)
				{
					if(e < m_First) continue;
					added.insert(e);
					++m_Count;
				}
			}
			void add(psl::array_view<std::pair<entity, entity>> entities)
			{
				m_Changes = true;
				add_impl(entities);
				auto& added = m_Added;
				for(auto range : entities)
				{
					for(auto e = range.first; e < range.second; ++e)
					{
						if(e < m_First) continue;
						added.insert(e);
						++m_Count;
					}
				}
			}
			void destroy(psl::array_view<std::pair<entity, entity>> entities)
			{
				m_Changes	 = true;
				auto& removed = m_Removed;
				for(auto range : entities)
				{

					for(auto e = range.first; e < range.second; ++e)
					{
						if(e < m_First)
						{
							m_Removed.insert(e);
							continue;
						}
						--m_Count;
					}
				}
			};
			void destroy(psl::array_view<entity> entities) noexcept
			{
				m_Changes	 = entities.size() > 0;
				auto& removed = m_Removed;
				for(auto e : entities)
				{
					if(e < m_First)
					{
						m_Removed.insert(e);
						continue;
					}
					--m_Count;
				}
			}
			void destroy(entity entity) noexcept
			{
				m_Changes = true;
				if(entity < m_First)
				{
					m_Removed.insert(entity);
					return;
				}
				--m_Count;
			}
			virtual void* data() noexcept = 0;


			size_t count() const noexcept { return m_Count; };
			bool changes() const noexcept { return m_Changes; };
			component_key_t id() const noexcept { return m_ID; };

			psl::array_view<entity> added_entities() const noexcept { return m_Added.indices(); };
			psl::array_view<entity> removed_entities() const noexcept { return m_Removed.indices(); };


			inline size_t component_size() const noexcept { return m_Size; };

			virtual details::component_info* create_storage(psl::sparse_array<entity>& remapped_entities) const = 0;
			virtual void merge_into(details::component_info* dst,
									psl::sparse_array<entity>& remapped_entities) const							= 0;

		  protected:
			virtual void add_impl(psl::array_view<entity> entities)					   = 0;
			virtual void add_impl(psl::array_view<std::pair<entity, entity>> entities) = 0;

			// state const * m_State;
			entity m_First;

		  private:
			size_t m_Count{0};
			psl::sparse_indice_array<entity> m_Removed;
			psl::sparse_indice_array<entity> m_Added;
			bool m_Changes = false;
			component_key_t m_ID;
			size_t m_Size;
		};

		template <typename T, bool tag_type = std::is_empty<T>::value>
		class command_buffer_component_info_typed final : public command_buffer_component_info
		{
		  public:
			command_buffer_component_info_typed(entity first)
				: command_buffer_component_info(first, details::key_for<T>(), sizeof(T)){};
			memory::sparse_array<T, entity>& entity_data() { return m_Entities; };


			void* data() noexcept override { return m_Entities.data(); }

			details::component_info* create_storage(psl::sparse_array<entity>& remapped_entities) const override
			{
				// assert(m_ModifiedEntities.size() == 0);
				auto target = new details::component_info_typed<T>();
				target->lock();

				for(auto e : m_Entities.indices())
				{
					target->add(remapped_entities[e]);
				}
				std::memcpy(target->data(), m_Entities.data(), sizeof(T) * m_Entities.size());
				return target;
			}

			void merge_into(details::component_info* dst, psl::sparse_array<entity>& remapped_entities) const override
			{
				if(m_Entities.size() > 0)
				{
					auto offset = dst->entities(true).size();
					for(auto e : m_Entities.indices())
					{
						if(!remapped_entities.has(e))
							continue;
						dst->add(remapped_entities[e]);
						std::memcpy((void*)((T*)dst->data() + offset), (void*)&m_Entities.at(e), sizeof(T));
						offset += 1;
					}

				}
				auto dst_typed = (details::component_info_typed<T>*)dst;

				auto& data{dst_typed->entity_data()};

				for(auto e : m_ModifiedEntities.indices())
				{
					if(!data.has(e)) data.insert(e);

					data[e] = m_ModifiedEntities.at(e);
				}

				auto removed{removed_entities()};
				for(auto e : removed)
				{
					assert_debug_break(dst->has_component(e));
					if(e < m_First) dst->destroy(e);
				}
			}

		  protected:
			void add_impl(psl::array_view<entity> entities) override
			{
				m_Entities.reserve(m_Entities.size() + entities.size());
				std::for_each(std::begin(entities), std::end(entities),
							  [this](auto e) { (e < m_First) ? m_ModifiedEntities.insert(e) : m_Entities.insert(e); });
			}
			void add_impl(psl::array_view<std::pair<entity, entity>> entities) override
			{
				auto count = std::accumulate(
					std::begin(entities), std::end(entities), size_t{0},
					[](size_t sum, const std::pair<entity, entity>& r) { return sum + (r.second - r.first); });

				m_Entities.reserve(m_Entities.size() + count);
				for(auto range : entities)
				{
					for(auto e = range.first; e < range.second; ++e)
						(e < m_First) ? m_ModifiedEntities.insert(e) : m_Entities.insert(e);
				}
			}

		  private:
			memory::sparse_array<T, entity> m_ModifiedEntities{};
			memory::sparse_array<T, entity> m_Entities{};
		};

		template <typename T>
		class command_buffer_component_info_typed<T, true> final : public command_buffer_component_info
		{
		  public:
			command_buffer_component_info_typed(entity first)
				: command_buffer_component_info(first, details::key_for<T>(), 0){};

			void* data() noexcept override { return nullptr; }

			details::component_info* create_storage(psl::sparse_array<entity>& remapped_entities) const override
			{
				// assert(m_ModifiedEntities.size() == 0);
				auto target = new details::component_info_typed<T>();
				target->lock();
				for(auto e : m_Entities.indices())
				{
					target->add(remapped_entities[e]);
				}

				for(auto e : m_ModifiedEntities.indices()) target->add(e);
				return target;
			}

			void merge_into(details::component_info* dst, psl::sparse_array<entity>& remapped_entities) const override
			{
				auto offset = dst->entities(true).size();
				for(auto e : m_Entities.indices()) dst->add(remapped_entities[e]);

				for(auto e : m_ModifiedEntities.indices())
				{
					assert_debug_break(!dst->has_component(e));
					dst->add(e);
				}
				auto removed{removed_entities()};
				for(auto e : removed)
				{
					if(e < m_First) dst->destroy(e);
				}
			}

		  protected:
			void add_impl(psl::array_view<entity> entities) override
			{
				for(auto e : entities) (e < m_First) ? m_ModifiedEntities.insert(e) : m_Entities.insert(e);
			}
			void add_impl(psl::array_view<std::pair<entity, entity>> entities) override
			{
				for(auto range : entities)
				{
					for(auto e = range.first; e < range.second; ++e)
						(e < m_First) ? m_ModifiedEntities.insert(e) : m_Entities.insert(e);
				}
			}

		  private:
			psl::sparse_indice_array<entity> m_ModifiedEntities{};
			psl::sparse_indice_array<entity> m_Entities{};
		};
	} // namespace details

	class command_buffer
	{
		friend class state;

	  public:
		command_buffer(const state& state);

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities)
		{
			if(entities.size() == 0) return;
			static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
			(add_component<Ts>(entities), ...);
		}

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities, Ts&&... prototype)
		{
			if(entities.size() == 0) return;
			static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
			(add_component(entities, std::forward<Ts>(prototype)), ...);
		}

		template <typename... Ts>
		void remove_components(psl::array_view<entity> entities) noexcept
		{
			if(entities.size() == 0) return;
			static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to remove");
			(create_storage<Ts>(), ...);
			(remove_component(details::key_for<Ts>(), entities), ...);
		}

		template <typename... Ts>
		void add_components(psl::array_view<std::pair<entity, entity>> entities)
		{
			if(entities.size() == 0) return;
			static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
			(add_component<Ts>(entities), ...);
		}
		template <typename... Ts>
		void add_components(psl::array_view<std::pair<entity, entity>> entities, Ts&&... prototype)
		{
			if(entities.size() == 0) return;
			static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
			(add_component(entities, std::forward<Ts>(prototype)), ...);
		}

		template <typename... Ts>
		void remove_components(psl::array_view<std::pair<entity, entity>> entities) noexcept
		{
			if(entities.size() == 0) return;
			static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to remove");
			(create_storage<Ts>(), ...);
			(remove_component(details::key_for<Ts>(), entities), ...);
		}

		template <typename... Ts>
		psl::array<entity> create(entity count)
		{
			psl::array<entity> entities;
			const auto recycled = std::min<entity>(count, (entity)m_Orphans);
			m_Orphans -= recycled;
			const auto remainder = count - recycled;

			entities.reserve(recycled + remainder);
			// we do this to protect ourselves from 1 entity loops
			if(m_Entities.size() + remainder >= m_Entities.capacity())
				m_Entities.reserve(m_Entities.size() * 2 + remainder);

			for(size_t i = 0; i < recycled; ++i)
			{
				const auto orphan = m_Next;
				entities.emplace_back(orphan);
				m_Next					   = m_Entities[(size_t)m_Next];
				m_Entities[(size_t)orphan] = orphan;
			}

			for(size_t i = 0; i < remainder; ++i)
			{
				entities.emplace_back((entity)m_Entities.size() + m_First);
				m_Entities.emplace_back((entity)m_Entities.size() + m_First);
			}

			if constexpr(sizeof...(Ts) > 0)
			{
				(add_components<Ts>(entities), ...);
			}
			return entities;
		}

		template <typename... Ts>
		psl::array<entity> create(entity count, Ts&&... prototype)
		{
			psl::array<entity> entities;
			const auto recycled = std::min<entity>(count, (entity)m_Orphans);
			m_Orphans -= recycled;
			const auto remainder = count - recycled;

			entities.reserve(recycled + remainder);
			// we do this to protect ourselves from 1 entity loops
			if(m_Entities.size() + remainder >= m_Entities.capacity())
				m_Entities.reserve(m_Entities.size() * 2 + remainder);
			for(size_t i = 0; i < recycled; ++i)
			{
				auto orphan = m_Next;
				entities.emplace_back(orphan);
				m_Next					   = m_Entities[(size_t)m_Next];
				m_Entities[(size_t)orphan] = orphan;
			}

			for(size_t i = 0; i < remainder; ++i)
			{
				entities.emplace_back((entity)m_Entities.size() + m_First);
				m_Entities.emplace_back((entity)m_Entities.size() + m_First);
			}
			add_components(entities, std::forward<Ts>(prototype)...);

			return entities;
		}

		void destroy(psl::array_view<entity> entities) noexcept;
		void destroy(entity entity) noexcept;

	  private:
		//------------------------------------------------------------
		// helpers
		//------------------------------------------------------------
		template <typename T>
		void create_storage()
		{
			auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [](const auto& cInfo) {
				constexpr auto key = details::key_for<T>();
				return (key == cInfo->id());
			});

			constexpr auto key = details::key_for<T>();
			if(it == std::end(m_Components))
				m_Components.emplace_back(new details::command_buffer_component_info_typed<T>(m_First));
		}

		details::command_buffer_component_info*
		get_command_buffer_component_info(details::component_key_t key) noexcept;

		//------------------------------------------------------------
		// add_component
		//------------------------------------------------------------
		template <typename T>
		void add_component(psl::array_view<std::pair<entity, entity>> entities, T&& prototype)
		{
			if constexpr(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value &&
						 std::is_trivially_destructible<T>::value)
			{
				static_assert(!std::is_empty_v<T>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");
				create_storage<T>();
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &prototype);
			}
			else // todo wait till deduction guides are resolved on libc++
				 // https://bugs.llvm.org/show_bug.cgi?id=39606
			// else if constexpr(std::is_constructible<decltype(std::function(prototype)), T>::value)
			{
				using tuple_type = typename psl::templates::template func_traits<T>::arguments_t;
				static_assert(std::tuple_size<tuple_type>::value == 1,
							  "only one argument is allowed in the prototype invocable");
				using arg0_t = std::tuple_element_t<0, tuple_type>;
				static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
							  "the argument type should be of 'T&'");
				using type = typename std::remove_reference<arg0_t>::type;
				static_assert(!std::is_empty_v<type>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");

				create_storage<type>();
				add_component_impl(details::key_for<type>(), entities, sizeof(type),
								   [prototype](std::uintptr_t location, size_t count) {
									   for(auto i = size_t{0}; i < count; ++i)
									   {
										   std::invoke(prototype, *((type*)(location) + i));
									   }
								   });
			}
			/*else
			{
				static_assert(psl::templates::always_false<T>::value,
							  "could not figure out if the template type was an invocable or a component prototype");
			}*/
		}

		template <typename T>
		void add_component(psl::array_view<std::pair<entity, entity>> entities, psl::ecs::empty<T>&& prototype)
		{
			create_storage<T>();
			if constexpr(std::is_trivially_constructible_v<T>)
			{
				add_component_impl(details::key_for<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
			}
			else
			{
				T v{};
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &v);
			}
		}

		template <typename T>
		void add_component(psl::array_view<std::pair<entity, entity>> entities)
		{
			create_storage<T>();
			if constexpr(std::is_trivially_constructible_v<T>)
			{
				add_component_impl(details::key_for<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
			}
			else
			{
				T v{};
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &v);
			}
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities, T&& prototype)
		{

			if constexpr(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value &&
						 std::is_trivially_destructible<T>::value)
			{
				static_assert(!std::is_empty_v<T>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");


				create_storage<T>();
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &prototype);
			}
			else
			{
				using tuple_type = typename psl::templates::template func_traits<T>::arguments_t;
				static_assert(std::tuple_size<tuple_type>::value == 1,
							  "only one argument is allowed in the prototype invocable");
				using arg0_t = std::tuple_element_t<0, tuple_type>;
				static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
							  "the argument type should be of 'T&'");
				using type = typename std::remove_reference<arg0_t>::type;
				static_assert(!std::is_empty_v<type>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");
				create_storage<type>();
				add_component_impl(details::key_for<type>(), entities, sizeof(type),
								   [prototype](std::uintptr_t location, size_t count) {
									   for(auto i = size_t{0}; i < count; ++i)
									   {
										   std::invoke(prototype, *((type*)(location) + i));
									   }
								   });
			}
			/*else
			{
				static_assert(psl::templates::always_false<T>::value,
							  "could not figure out if the template type was an invocable or a component prototype");
			}*/
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities, psl::ecs::empty<T>&& prototype)
		{
			create_storage<T>();

			if constexpr(std::is_trivially_constructible_v<T>)
			{
				add_component_impl(details::key_for<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
			}
			else
			{
				T v{};
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &v);
			}
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities)
		{
			create_storage<T>();
			if constexpr(std::is_trivially_constructible_v<T>)
			{
				add_component_impl(details::key_for<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
			}
			else
			{
				T v{};
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &v);
			}
		}

		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
								size_t size);
		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
								size_t size, std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
								size_t size, void* prototype);


		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size);
		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
								std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
								void* prototype);

		//------------------------------------------------------------
		// remove_component
		//------------------------------------------------------------
		void remove_component(details::component_key_t key,
							  psl::array_view<std::pair<entity, entity>> entities) noexcept;
		void remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept;

		state const* m_State;
		psl::array<psl::unique_ptr<details::command_buffer_component_info>> m_Components;
		entity m_First;
		psl::array<entity> m_Entities;

		psl::array<entity> m_DestroyedEntities;

		entity m_Next;
		size_t m_Orphans{0};
	};
} // namespace psl::ecs