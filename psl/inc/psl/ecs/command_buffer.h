#pragma once
#include "details/component_info.h"
#include "details/component_key.h"
#include "entity.h"
#include "psl/array.h"
#include "psl/array_view.h"
#include "psl/memory/sparse_array.h"
#include "psl/sparse_array.h"
#include "psl/sparse_indice_array.h"
#include "psl/static_array.h"
#include "psl/template_utils.h"
#include "psl/unique_ptr.h"

namespace psl::ecs
{
	class state;

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
		void add_components(psl::array_view<entity> entities, psl::array_view<Ts>... data)
		{
			if(entities.size() == 0) return;
			static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
			(add_component<Ts>(entities, data), ...);
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
			if(it == std::end(m_Components)) m_Components.emplace_back(new details::component_info_typed<T>());
		}

		details::component_info* get_component_info(details::component_key_t key) noexcept;

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
			else	// todo wait till deduction guides are resolved on libc++
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
				add_component_impl(
				  details::key_for<type>(), entities, sizeof(type), [prototype](std::uintptr_t location, size_t count) {
					  for(auto i = size_t {0}; i < count; ++i)
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
				T v {};
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
				T v {};
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
				add_component_impl(
				  details::key_for<type>(), entities, sizeof(type), [prototype](std::uintptr_t location, size_t count) {
					  for(auto i = size_t {0}; i < count; ++i)
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
				T v {};
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
				T v {};
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &v);
			}
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities, psl::array_view<T> data)
		{
			assert(entities.size() == data.size());
			create_storage<T>();
			static_assert(!std::is_empty_v<T>,
						  "no need to pass an array of tag types through, it's a waste of computing and memory");

			add_component_impl(details::key_for<T>(), entities, sizeof(T), data.data(), false);
		}

		void add_component_impl(details::component_key_t key,
								psl::array_view<std::pair<entity, entity>> entities,
								size_t size);
		void add_component_impl(details::component_key_t key,
								psl::array_view<std::pair<entity, entity>> entities,
								size_t size,
								std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key,
								psl::array_view<std::pair<entity, entity>> entities,
								size_t size,
								void* prototype);


		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size);
		void add_component_impl(details::component_key_t key,
								psl::array_view<entity> entities,
								size_t size,
								std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key,
								psl::array_view<entity> entities,
								size_t size,
								void* prototype,
								bool repeat = true);

		//------------------------------------------------------------
		// remove_component
		//------------------------------------------------------------
		void remove_component(details::component_key_t key,
							  psl::array_view<std::pair<entity, entity>> entities) noexcept;
		void remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept;

		state const* m_State;
		psl::array<psl::unique_ptr<details::component_info>> m_Components;
		entity m_First;
		psl::array<entity> m_Entities;

		psl::array<entity> m_DestroyedEntities;

		entity m_Next;
		size_t m_Orphans {0};
	};
}	 // namespace psl::ecs