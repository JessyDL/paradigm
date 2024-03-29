#pragma once
#include "details/component_container.hpp"
#include "details/component_key.hpp"
#include "entity.hpp"
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/ecs/pack.hpp"
#include "psl/memory/sparse_array.hpp"
#include "psl/sparse_array.hpp"
#include "psl/sparse_indice_array.hpp"
#include "psl/static_array.hpp"
#include "psl/template_utils.hpp"
#include "psl/unique_ptr.hpp"

namespace psl::ecs {
class state_t;

class command_buffer_t {
	friend class state_t;

	template <IsPack PackType>
	psl::array_view<entity_t> pack_get_entities(PackType const& pack) {
		psl::array_view<entity_t> entities {};
		if constexpr(IsAccessDirect<typename PackType::access_type>) {
			entities = pack.template get<entity_t>();
		} else {
			const auto& indirect_array = pack.template get<entity_t>();
			// the internal data does not get modified, so const_cast'ing is safe here.
			entities = psl::array_view<entity_t> {const_cast<entity_t*>(indirect_array.data()), indirect_array.size()};
		}
		return entities;
	}

  public:
	command_buffer_t(const state_t& state);

	template <typename... Ts>
	void add_components(IsPack auto const& pack) {
		psl::array_view<entity_t> entities = pack_get_entities(pack);
		(add_component<Ts>(entities), ...);
	}

	template <typename... Ts>
	void add_components(psl::array_view<entity_t> entities) {
		if(entities.size() == 0)
			return;
		static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
		(add_component<Ts>(entities), ...);
	}

	template <typename... Ts>
	void add_components(IsPack auto const& pack, psl::array_view<Ts>... data) {
		psl::array_view<entity_t> entities = pack_get_entities(pack);
		add_components<Ts...>(entities, data...);
	}

	template <typename... Ts>
	void add_components(psl::array_view<entity_t> entities, psl::array_view<Ts>... data) {
		if(entities.size() == 0)
			return;
		static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
		(add_component<Ts>(entities, data), ...);
	}

	template <typename... Ts>
	void add_components(IsPack auto const& pack, Ts&&... prototype) {
		psl::array_view<entity_t> entities = pack_get_entities(pack);
		add_components<Ts...>(entities, std::forward<Ts>(prototype)...);
	}

	template <typename... Ts>
	void add_components(psl::array_view<entity_t> entities, Ts&&... prototype) {
		if(entities.size() == 0)
			return;
		static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
		(add_component(entities, std::forward<Ts>(prototype)), ...);
	}

	template <typename... Ts>
	void remove_components(IsPack auto const& pack) noexcept {
		psl::array_view<entity_t> entities = pack_get_entities(pack);
		remove_components<Ts...>(entities);
	}

	template <typename... Ts>
	void remove_components(psl::array_view<entity_t> entities) noexcept {
		if(entities.size() == 0)
			return;
		static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to remove");
		(create_storage<Ts>(), ...);
		(remove_component(details::component_key_t::generate<Ts>(), entities), ...);
	}

	template <typename... Ts>
	void add_components(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) {
		if(entities.size() == 0)
			return;
		static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
		(add_component<Ts>(entities), ...);
	}
	template <typename... Ts>
	void add_components(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
						Ts&&... prototype) {
		if(entities.size() == 0)
			return;
		static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to add");
		(add_component(entities, std::forward<Ts>(prototype)), ...);
	}

	template <typename... Ts>
	void remove_components(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) noexcept {
		if(entities.size() == 0)
			return;
		static_assert(sizeof...(Ts) > 0, "you need to supply at least one component to remove");
		(create_storage<Ts>(), ...);
		(remove_component(details::component_key_t::generate<Ts>(), entities), ...);
	}

	template <typename... Ts>
	psl::array<entity_t> create(entity_t::size_type count) {
		psl::array<entity_t> entities;
		const auto recycled = std::min<entity_t::size_type>(static_cast<entity_t::size_type>(count), m_Orphans);
		m_Orphans -= recycled;
		const auto remainder = count - recycled;

		entities.reserve(recycled + remainder);
		// we do this to protect ourselves from 1 entity loops
		if(m_Entities.size() + remainder >= m_Entities.capacity())
			m_Entities.reserve(m_Entities.size() * 2 + remainder);

		for(size_t i = 0; i < recycled; ++i) {
			const auto orphan = m_Next;
			entities.emplace_back(orphan);
			m_Next					   = static_cast<entity_t::size_type>(m_Entities[m_Next]);
			m_Entities[(size_t)orphan] = orphan;
		}

		for(size_t i = 0; i < remainder; ++i) {
			entities.emplace_back(entity_t {static_cast<entity_t::size_type>(m_Entities.size()) + m_First});
			m_Entities.emplace_back(entity_t {static_cast<entity_t::size_type>(m_Entities.size()) + m_First});
		}

		if constexpr(sizeof...(Ts) > 0) {
			(add_components<Ts>(entities), ...);
		}
		return entities;
	}

	template <typename... Ts>
	psl::array<entity_t> create(entity_t::size_type count, Ts&&... prototype) {
		psl::array<entity_t> entities;
		const auto recycled = std::min<entity_t::size_type>(count, m_Orphans);
		m_Orphans -= recycled;
		const auto remainder = count - recycled;

		entities.reserve(recycled + remainder);
		// we do this to protect ourselves from 1 entity loops
		if(m_Entities.size() + remainder >= m_Entities.capacity())
			m_Entities.reserve(m_Entities.size() * 2 + remainder);
		for(size_t i = 0; i < recycled; ++i) {
			auto orphan = m_Next;
			entities.emplace_back(orphan);
			m_Next					   = m_Entities[(size_t)m_Next];
			m_Entities[(size_t)orphan] = orphan;
		}

		for(size_t i = 0; i < remainder; ++i) {
			entities.emplace_back(entity_t {static_cast<entity_t::size_type>(m_Entities.size()) + m_First});
			m_Entities.emplace_back(entity_t {static_cast<entity_t::size_type>(m_Entities.size()) + m_First});
		}
		add_components(entities, std::forward<Ts>(prototype)...);

		return entities;
	}

	void destroy(psl::array_view<entity_t> entities) noexcept;
	void destroy(psl::ecs::details::indirect_array_t<entity_t, entity_t::size_type> entities) noexcept;
	void destroy(entity_t entity) noexcept;

  private:
	//------------------------------------------------------------
	// helpers
	//------------------------------------------------------------
	template <typename T>
	void create_storage() {
		auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [](const auto& cInfo) {
			constexpr auto key = details::component_key_t::generate<T>();
			return (key == cInfo->id());
		});

		constexpr auto key = details::component_key_t::generate<T>();

		if(it == std::end(m_Components)) {
			m_Components.emplace_back(details::instantiate_component_container<T>());
		}
	}

	details::component_container_t* get_component_container(const details::component_key_t& key) noexcept;

	//------------------------------------------------------------
	// add_component
	//------------------------------------------------------------
	template <typename T>
	void add_component(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities, T&& prototype) {
		if constexpr(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value &&
					 std::is_trivially_destructible<T>::value) {
			static_assert(!std::is_empty_v<T>,
						  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
						  "psl::ecs::empty<T>{} to avoid initialization.");
			create_storage<T>();
			add_component_impl(details::component_key_t::generate<T>(), entities, sizeof(T), &prototype);
		} else	  // todo wait till deduction guides are resolved on libc++
				  // https://bugs.llvm.org/show_bug.cgi?id=39606
		// else if constexpr(std::is_constructible<decltype(std::function(prototype)), T>::value)
		{
			using pack_type = typename psl::templates::template func_traits<T>::arguments_t;
			static_assert(psl::type_pack_size_v<pack_type> == 1,
						  "only one argument is allowed in the prototype invocable");
			using arg0_t = psl::type_at_index_t<0, pack_type>;
			static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
						  "the argument type should be of 'T&'");
			using type = typename std::remove_reference<arg0_t>::type;
			static_assert(!std::is_empty_v<type>,
						  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
						  "psl::ecs::empty<T>{} to avoid initialization.");

			create_storage<type>();
			add_component_impl(details::component_key_t::generate<type>(),
							   entities,
							   sizeof(type),
							   [prototype](std::uintptr_t location, size_t count) {
								   for(auto i = size_t {0}; i < count; ++i) {
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
	void add_component(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) {
		create_storage<T>();

		if constexpr(details::DoesComponentTypeNeedPrototypeCall<T>) {
			T v {details::prototype_for<T>()};
			add_component_impl(details::component_key_t::generate<T>(), entities, sizeof(T), &v);
		} else {
			add_component_impl(
			  details::component_key_t::generate<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
		}
	}

	template <typename T>
	void add_component(psl::array_view<entity_t> entities, T&& prototype) {
		if constexpr(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value &&
					 std::is_trivially_destructible<T>::value) {
			static_assert(!std::is_empty_v<T>,
						  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
						  "psl::ecs::empty<T>{} to avoid initialization.");


			create_storage<T>();
			add_component_impl(details::component_key_t::generate<T>(), entities, sizeof(T), &prototype);
		} else {
			using pack_type = typename psl::templates::template func_traits<T>::arguments_t;
			static_assert(psl::type_pack_size_v<pack_type> == 1,
						  "only one argument is allowed in the prototype invocable");
			using arg0_t = psl::type_at_index_t<0, pack_type>;
			static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
						  "the argument type should be of 'T&'");
			using type = typename std::remove_reference<arg0_t>::type;
			static_assert(!std::is_empty_v<type>,
						  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
						  "psl::ecs::empty<T>{} to avoid initialization.");
			create_storage<type>();
			add_component_impl(details::component_key_t::generate<type>(),
							   entities,
							   sizeof(type),
							   [prototype](std::uintptr_t location, size_t count) {
								   for(auto i = size_t {0}; i < count; ++i) {
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
	void add_component(psl::array_view<entity_t> entities) {
		create_storage<T>();

		if constexpr(details::DoesComponentTypeNeedPrototypeCall<T>) {
			T v {details::prototype_for<T>};
			add_component_impl(details::component_key_t::generate<T>(), entities, sizeof(T), &v);
		} else {
			add_component_impl(
			  details::component_key_t::generate<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
		}
	}

	template <typename T>
	void add_component(psl::array_view<entity_t> entities, psl::array_view<T> data) {
		psl_assert(entities.size() == data.size(),
				   "incorrect amount of data input compared to entities, expected {} but got {}",
				   entities.size(),
				   data.size());
		create_storage<T>();
		static_assert(!std::is_empty_v<T>,
					  "no need to pass an array of tag types through, it's a waste of computing and memory");

		add_component_impl(details::component_key_t::generate<T>(), entities, sizeof(T), data.data(), false);
	}

	void add_component_impl(const details::component_key_t& key,
							psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
							size_t size);
	void add_component_impl(const details::component_key_t& key,
							psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
							size_t size,
							std::function<void(std::uintptr_t, size_t)> invocable);
	void add_component_impl(const details::component_key_t& key,
							psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
							size_t size,
							void* prototype);


	void add_component_impl(const details::component_key_t& key, psl::array_view<entity_t> entities, size_t size);
	void add_component_impl(const details::component_key_t& key,
							psl::array_view<entity_t> entities,
							size_t size,
							std::function<void(std::uintptr_t, size_t)> invocable);
	void add_component_impl(const details::component_key_t& key,
							psl::array_view<entity_t> entities,
							size_t size,
							void* prototype,
							bool repeat = true);

	//------------------------------------------------------------
	// remove_component
	//------------------------------------------------------------
	void remove_component(const details::component_key_t& key,
						  psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) noexcept;
	void remove_component(const details::component_key_t& key, psl::array_view<entity_t> entities) noexcept;

	state_t const* m_State {nullptr};
	psl::array<std::unique_ptr<details::component_container_t>> m_Components {};
	entity_t::size_type m_First {0};
	psl::array<entity_t> m_Entities {};

	psl::array<entity_t> m_DestroyedEntities {};

	entity_t::size_type m_Next {0};
	entity_t::size_type m_Orphans {0};
};
}	 // namespace psl::ecs
