#pragma once
#include "details/component_container.hpp"
#include "details/component_key.hpp"
#include "details/mutate_instruction.hpp"
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

namespace details {
	template <typename T>
	concept IsRangeType = requires(T t) {
		{ t.begin() } -> std::same_as<typename T::iterator>;
		{ t.end() } -> std::same_as<typename T::iterator>;
		// todo std::convertible_to is not available in android ndk
		{ t.size() } -> std::convertible_to<size_t>;
	};

	enum class add_component_behaviour_mode_t {
		empty_container,
		callable_1,
		callable_2,
		range,
		standard_layout,
	};

	template <typename T, typename Prototype>
	struct decode_add_component_behaviour_t {
		static_assert(psl::templates::always_false<T>::value,
					  "could not figure out if the template type was an invocable or a component prototype");
	};

	template <typename T, typename Prototype>
		requires(is_empty_container<Prototype>::value)
	struct decode_add_component_behaviour_t<T, Prototype> {
		using underlying_t		   = typename empty_container<Prototype>::type;
		using type				   = std::conditional_t<IsMutateInstruction<T>, T, underlying_t>;
		static constexpr auto mode = add_component_behaviour_mode_t::empty_container;
	};


	template <typename T, typename Prototype>
		requires(psl::templates::is_callable_n<Prototype, 1>::value)
	struct decode_add_component_behaviour_t<T, Prototype> {
		using pack_type = typename psl::templates::func_traits<Prototype>::arguments_t;
		static_assert(psl::type_pack_size_v<pack_type> == 1, "only one argument is allowed in the prototype invocable");

		using arg0_t = psl::type_at_index_t<0, pack_type>;
		static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>, "the argument type should be of 'T&'");
		using underlying_t = typename std::remove_reference<arg0_t>::type;
		using type		   = std::conditional_t<IsMutateInstruction<T>, T, underlying_t>;
		static_assert(!std::is_empty_v<underlying_t>,
					  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
					  "psl::ecs::empty<T>{} to avoid initialization.");
		static constexpr auto mode = add_component_behaviour_mode_t::callable_1;
	};

	template <typename T, typename Prototype>
		requires(psl::templates::is_callable_n<Prototype, 2>::value)
	struct decode_add_component_behaviour_t<T, Prototype> {
		using pack_type = typename psl::templates::func_traits<Prototype>::arguments_t;
		static_assert(psl::type_pack_size_v<pack_type> == 2, "two arguments required in the prototype invocable");
		using arg0_t = psl::type_at_index_t<0, pack_type>;
		static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
					  "the argument type for arg 0 should be of 'T&'");
		using underlying_t = typename std::remove_reference<arg0_t>::type;
		using type		   = std::conditional_t<IsMutateInstruction<T>, T, underlying_t>;
		static_assert(std::is_invocable_v<Prototype, underlying_t&, psl::ecs::entity_t>,
					  "Must be invocable by your component type & entity_t as the second parameter");
		static_assert(!std::is_empty_v<underlying_t>,
					  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
					  "psl::ecs::empty<T>{} to avoid initialization.");
		static constexpr auto mode = add_component_behaviour_mode_t::callable_2;
	};

	template <typename T, typename Prototype>
		requires(IsRangeType<Prototype>)
	struct decode_add_component_behaviour_t<T, Prototype> {
		using underlying_t = std::remove_cvref_t<decltype(*std::declval<Prototype>().data())>;
		using type		   = std::conditional_t<IsMutateInstruction<T>, T, underlying_t>;
		static_assert(!std::is_empty_v<underlying_t>,
					  "no need to pass an array of tag types through, it's a waste of computing and memory");
		static constexpr auto mode = add_component_behaviour_mode_t::range;
	};

	template <typename T, typename Prototype>
		requires(std::is_standard_layout<T>::value && !is_empty_container<Prototype>::value &&
				 !IsRangeType<Prototype> && !psl::templates::is_callable_n<Prototype, 1>::value &&
				 !psl::templates::is_callable_n<Prototype, 2>::value)
	struct decode_add_component_behaviour_t<T, Prototype> {
		using type		   = T;
		using underlying_t = T;
		static_assert(!std::is_empty_v<T>,
					  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
					  "psl::ecs::empty<T>{} to avoid initialization.");
		static constexpr auto mode = add_component_behaviour_mode_t::standard_layout;
	};
}	 // namespace details

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

	template <typename T>
		requires((!IsFilteringOp<T> && IsRestrictedMutable<T>))
	void mutate_components(psl::array_view<entity_t> entities, T&& prototype) {
		add_component<details::mutate_instruction_t<T>>(entities, std::forward<T>(prototype));
	}

	template <typename T>
		requires((!IsFilteringOp<T> && IsRestrictedMutable<T>))
	void mutate_components(IsPack auto const& pack, auto&& prototype) {
		psl::array_view<entity_t> entities = pack_get_entities(pack);
		mutate_components<T>(entities, std::forward<decltype(prototype)>(prototype));
	}

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
		(add_component<Ts>(entities, std::forward<Ts>(prototype)), ...);
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
	void add_component(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
					   auto&& prototype) {
		psl::array<entity_t> entity_array;
		entity_array.reserve(
		  std::accumulate(entities.begin(), entities.end(), size_t {0}, [](size_t acc, const auto& pair) {
			  return acc + (pair.second - pair.first);
		  }));

		for(const auto& pair : entities) {
			for(auto i = pair.first; i < pair.second; ++i) {
				entity_array.emplace_back(entity_t {i});
			}
		}
		add_component<T>(entity_array, std::forward<decltype(prototype)>(prototype));
	}

	template <typename T>
	void add_component(psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) {
		add_component<T>(entities, empty<T> {});
	}

	template <typename T>
	void add_component(psl::array_view<entity_t> entities, auto&& prototype) {
		using behavior_t = details::decode_add_component_behaviour_t<T, std::remove_cvref_t<decltype(prototype)>>;
		static constexpr auto mode = behavior_t::mode;
		using type				   = typename behavior_t::type;
		using underlying_t		   = typename behavior_t::underlying_t;

		if constexpr(mode == details::add_component_behaviour_mode_t::empty_container) {
			create_storage<type>();
			if constexpr(details::DoesComponentTypeNeedPrototypeCall<underlying_t>) {
				underlying_t v {details::prototype_for<underlying_t>()};
				add_component_impl(details::component_key_t::generate<type>(), entities, sizeof(type), &v);
			} else {
				add_component_impl(details::component_key_t::generate<type>(), entities, sizeof(type));
			}
		} else if constexpr(mode == details::add_component_behaviour_mode_t::callable_1) {
			create_storage<type>();
			add_component_impl(details::component_key_t::generate<type>(),
							   entities,
							   sizeof(type),
							   [prototype](std::uintptr_t location, size_t count) {
								   for(auto i = size_t {0}; i < count; ++i) {
									   std::invoke(prototype, *((underlying_t*)(location) + i));
								   }
							   });
		} else if constexpr(mode == details::add_component_behaviour_mode_t::callable_2) {
			create_storage<type>();
			add_component_impl(details::component_key_t::generate<type>(),
							   entities,
							   sizeof(type),
							   [prototype, &entities](std::uintptr_t location, size_t count) {
								   for(auto i = size_t {0}; i < count; ++i) {
									   std::invoke(prototype, *((underlying_t*)(location) + i), entities[i]);
								   }
							   });
		} else if constexpr(mode == details::add_component_behaviour_mode_t::range) {
			psl_assert(entities.size() == prototype.size(),
					   "incorrect amount of data input compared to entities, expected {} but got {}",
					   entities.size(),
					   prototype.size());
			create_storage<type>();
			add_component_impl(
			  details::component_key_t::generate<type>(), entities, sizeof(type), prototype.data(), false);
		} else if constexpr(mode == details::add_component_behaviour_mode_t::standard_layout) {
			create_storage<type>();
			add_component_impl(details::component_key_t::generate<type>(), entities, sizeof(type), &prototype);
		}
	}

	template <typename T>
	void add_component(psl::array_view<entity_t> entities) {
		add_component<T>(entities, empty<T> {});
	}

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
