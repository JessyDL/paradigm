#pragma once
#include "psl/array.hpp"
#include "psl/collections/indirect_array.hpp"
#include "psl/details/fixed_astring.hpp"
#include "psl/ecs/command_buffer.hpp"
#include "psl/ecs/component_traits.hpp"
#include "psl/ecs/details/component_container.hpp"
#include "psl/ecs/details/component_key.hpp"
#include "psl/ecs/details/components_cache.hpp"
#include "psl/ecs/details/entity_container.hpp"
#include "psl/ecs/details/filter_handler.hpp"
#include "psl/ecs/details/mutate_instruction.hpp"
#include "psl/ecs/details/stage_range.hpp"
#include "psl/ecs/details/system_information.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/filtering.hpp"
#include "psl/pack_view.hpp"
#include "psl/string_utils.hpp"
#include "psl/unique_ptr.hpp"
#include "selectors.hpp"
#include <chrono>

#include "psl/serialization/serializer.hpp"

#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	#include "psl/ecs/details/entity_relationship_handler.hpp"
	#include "psl/ecs/entity_relationship_data.hpp"
#else
	#include "psl/ecs/details/entity_relationship_null_handler.hpp"
#endif

/// \brief Private implementation details for the ECS.
/// \warning Users should not rely on these implementations.
namespace psl::ecs::details {
class system_handler_t;
template <typename... Ts>
struct get_packs {
	using type = psl::type_pack_t<Ts...>;
};

template <typename... Ts>
struct get_packs<psl::type_pack_t<Ts...>> : public get_packs<Ts...> {};

template <typename T, typename... Ts>
	requires(std::is_same_v<std::remove_cvref_t<T>, psl::ecs::info_t>)
struct get_packs<T, Ts...> : public get_packs<Ts...> {};


template <typename... Ts>
struct signature_info {
	using packs					   = psl::type_pack_t<Ts...>;
	static constexpr bool has_info = false;
	static constexpr bool is_const = true;
};

template <typename... Ts>
struct signature_info<psl::ecs::info_t&, Ts...> {
	using packs					   = typename get_packs<Ts...>::type;
	static constexpr bool has_info = true;
	static constexpr bool is_const = false;
};
template <typename... Ts>
struct signature_info<psl::ecs::info_t const&, Ts...> {
	using packs					   = typename get_packs<Ts...>::type;
	static constexpr bool has_info = true;
	static constexpr bool is_const = true;
};

template <typename... Ts>
struct signature_info<psl::type_pack_t<Ts...>> : public signature_info<Ts...> {};
}	 // namespace psl::ecs::details

namespace psl::utility {
template <>
struct converter<psl::ecs::entity_t> {
	using value_t	 = psl::ecs::entity_t;
	using view_t	 = psl::string8::view;
	using encoding_t = psl::string8_t;

	static encoding_t to_string(const value_t& x) {
		return converter<psl::ecs::entity_t::size_type> {}.to_string(static_cast<psl::ecs::entity_t::size_type>(x));
	}

	static value_t from_string(view_t str) {
		return psl::ecs::details::make_entity(converter<psl::ecs::entity_t::size_type> {}.from_string(str));
	}

	static void from_string(value_t& out, view_t str) {
		out = from_string(str);
	}

	static bool is_valid(view_t str) {
		return true;
	}
};
}	 // namespace psl::utility

/// \brief Entity Component System
///
namespace psl::ecs {
class state_t;

struct transient_system_tag_t {};
constexpr transient_system_tag_t transient_system_tag {};


class system_group_t final {
	friend class state_t;
	system_group_t(size_t id, psl::string_view debugName = "") : m_Id(id), m_DebugName(debugName) {}

  public:
	system_group_t() = default;

  private:
	size_t m_Id {std::numeric_limits<size_t>::max()};
	psl::string_view m_DebugName;
};

class state_t final : public details::entity_relationship_handler_t,
					  public details::entity_container_t,
					  public details::components_cache_t,
					  private details::filter_handler_t {
	friend class psl::serialization::accessor;
	static constexpr auto serialization_name {"ECS"};

	template <typename S>
	void serialize(S& serializer) {
		if constexpr(psl::serialization::details::IsDecoder<S>) {
			if(m_Tick != 0) {
				throw std::runtime_error("unsupported deserializing into non-empty state");
			}
		}

		entity_container_t::serialize(serializer);
		entity_relationship_handler_t::serialize(serializer);
		components_cache_t::serialize(serializer);
	}

  public:
	state_t(size_t workers								= 0,
			size_t cache_size							= 1024 * 1024 * 128,
			entity_t::size_type min_entities_per_worker = 1024);
	~state_t();
	state_t(const state_t&)			   = delete;
	state_t(state_t&&)				   = delete;
	state_t& operator=(const state_t&) = delete;
	state_t& operator=(state_t&&)	   = delete;

	template <typename T>
		requires((!IsFilteringOp<T> && IsRestrictedMutable<T>))
	void mutate_components(psl::array_view<entity_t> entities, auto&& prototype) {
		components_cache_t::add_component<details::mutate_instruction_t<T>>(
		  entities, std::forward<decltype(prototype)>(prototype));
		modify_entities(entities);
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void add_components(psl::array_view<entity_t> entities) {
		(components_cache_t::add_component<Ts>(entities, empty<Ts> {}), ...);
		modify_entities(entities);
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void add_components(psl::array_view<entity_t> entities, psl::array_view<Ts>... data) {
		(components_cache_t::add_component<Ts>(entities, data), ...);
		modify_entities(entities);
	}
	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void add_components(psl::array_view<entity_t> entities, Ts&&... prototype) {
		(components_cache_t::add_component<Ts>(entities, std::forward<Ts>(prototype)), ...);
		modify_entities(entities);
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void remove_components(psl::array_view<entity_t> entities) noexcept {
		components_cache_t::remove_components<Ts...>(entities);
		modify_entities(entities);
	}

	void clear(bool release_memory = true) noexcept;

	template <typename... Ts>
	[[maybe_unused]] psl::array<entity_t> create(auto count) {
		auto entities = std::move(entity_container_t::create(count));
		if constexpr(sizeof...(Ts) > 0) {
			add_components<Ts...>(entities);
		}
		return entities;
	}

	template <typename... Ts>
	[[maybe_unused]] psl::array<entity_t> create(auto count, Ts&&... prototype) {
		auto entities = std::move(entity_container_t::create(count));
		if constexpr(sizeof...(Ts) > 0) {
			add_components(entities, std::forward<Ts>(prototype)...);
		}
		return entities;
	}

	void destroy(psl::array_view<entity_t> entities) noexcept;
	void destroy(entity_t entity) noexcept;

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	psl::array<entity_t> filter() const noexcept {
		auto filter_group = make_filter_group<psl::ecs::pack_direct_full_t<Ts...>>();
		psl_assert(filter_group.size() == 1, "expected only one filter group");

		auto it =
		  std::find_if(std::begin(m_Filters), std::end(m_Filters), [&filter_group](const details::filter_result& data) {
			  return *data.group == filter_group[0];
		  });

		details::filter_result data {0};

		if(it == std::end(m_Filters)) {
			data.group = std::make_shared<details::filter_group>(filter_group[0]);
			initialize(data);
			if(data.group->is_singular_filter()) {
				return data.entities;
			}
		} else {
			data.entities = it->entities;
			data.group	  = it->group;
		}

		psl::array<entity_t> modified {};
		if(data.group->hierarchy_change == hierarchy_change_event::none) {
			modified = modified_entities();
		} else {
			modified = psl::array<entity_t>(modified_hierarchy_entities());
		}
		// if(!modified.empty() || (data.group->relationship & entity_relationship::self) != data.group->relationship) {
		std::sort(std::begin(modified), std::end(modified));
		filter(data, modified);
		//}
		return data.entities;
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	psl::array<entity_t> filter(psl::array_view<entity_t> entities) const noexcept {
		auto filter_group = make_filter_group<psl::ecs::pack_direct_full_t<Ts...>>();
		psl_assert(filter_group.size() == 1, "expected only one filter group");

		details::filter_result data {0, {}, std::make_shared<details::filter_group>(filter_group[0])};
		filter(data, entities);
		return data.entities;
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void set_components(psl::array_view<entity_t> entities, psl::array_view<Ts>... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
		modify_entities(entities);
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void set_components(psl::array_view<entity_t> entities, Ts&&... data) noexcept {
		(set_component(entities, std::forward<Ts>(data)), ...);
		modify_entities(entities);
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void assign_components(psl::array_view<entity_t> entities, psl::array_view<Ts>... data) noexcept {
		(components_cache_t::set_component(entities, std::forward<Ts>(data)), ...);
	}

	template <typename... Ts>
		requires(sizeof...(Ts) > 0)
	void assign_components(psl::array_view<entity_t> entities, Ts&&... data) noexcept {
		(components_cache_t::set_component(entities, std::forward<Ts>(data)), ...);
	}

	void tick(std::chrono::duration<float> dTime);
	void tick(std::chrono::duration<float> dTime, system_group_t group);
	void tick(std::chrono::duration<float> dTime, psl::array_view<system_group_t> groups);

	void reset(psl::array_view<entity_t> entities) noexcept;

	/// \brief returns the amount of active systems
	size_t systems() const noexcept {
		return m_SystemInformations.size() - m_ToRevoke.size();
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn>
		requires(!std::is_same_v<Fn, transient_system_tag_t>)
	auto declare(Fn&& fn, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(
		  threading::sequential, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, std::nullopt);
	}


	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto
	declare(threading threading, Fn&& fn, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(threading, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, std::nullopt);
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
		requires(!std::is_same_v<Fn, transient_system_tag_t>)
	auto declare(Fn&& fn, T* ptr, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(threading::sequential, std::forward<Fn>(fn), ptr, DebugName, systemGroup, std::nullopt);
	}
	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(threading threading, Fn&& fn, T* ptr, std::optional<system_group_t> systemGroup = std::nullopt)
	  -> system_token {
		return declare_impl(threading, std::forward<Fn>(fn), ptr, DebugName, systemGroup, std::nullopt);
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto
	declare(transient_system_tag_t, Fn&& fn, std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(
		  threading::sequential, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, transient_system_tag);
	}


	template <psl::details::fixed_astring DebugName = "", typename Fn>
	auto declare(transient_system_tag_t,
				 threading threading,
				 Fn&& fn,
				 std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(
		  threading, std::forward<Fn>(fn), (void*)nullptr, DebugName, systemGroup, transient_system_tag);
	}

	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(transient_system_tag_t, Fn&& fn, T* ptr, std::optional<system_group_t> systemGroup = std::nullopt)
	  -> system_token {
		return declare_impl(
		  threading::sequential, std::forward<Fn>(fn), ptr, DebugName, systemGroup, transient_system_tag);
	}
	template <psl::details::fixed_astring DebugName = "", typename Fn, typename T>
	auto declare(transient_system_tag_t,
				 threading threading,
				 Fn&& fn,
				 T* ptr,
				 std::optional<system_group_t> systemGroup = std::nullopt) -> system_token {
		return declare_impl(threading, std::forward<Fn>(fn), ptr, DebugName, systemGroup, transient_system_tag);
	}

	template <psl::details::fixed_astring DebugName = "">
	[[nodiscard]] auto create_system_group() noexcept {
		m_SystemGroups.emplace(m_SystemGroupCounter, psl::array<details::system_token> {});
		auto group = system_group_t {m_SystemGroupCounter, DebugName};
		++m_SystemGroupCounter;
		return group;
	}

	bool revoke(details::system_token id) noexcept {
		if(m_LockState) {
			if(auto it = m_SystemInformations.find(id);
			   it != std::end(m_SystemInformations) &&
			   // and we make sure we don't "double delete"
			   std::find(std::begin(m_ToRevoke), std::end(m_ToRevoke), id) == std::end(m_ToRevoke)) {
				m_ToRevoke.emplace_back(id);
				return true;
			}
			return false;
		} else {
			if(auto it = m_SystemInformations.find(id); it != std::end(m_SystemInformations)) {
				m_SystemInformations.erase(it);

				for(auto& group : m_SystemGroups) {
					group.second.erase(std::remove_if(std::begin(group.second),
													  std::end(group.second),
													  [&id](const auto& token) { return token == id; }),
									   std::end(group.second));
				}
				return true;
			}
			return false;
		}
	}

	template <typename Ts = void>
	size_t size() {
		if constexpr(std::is_same_v<Ts, void>) {
			return entity_container_t::size();
		} else {
			return components_cache_t::size<Ts>();
		}
	}

	void system_connect(details::system_token child, details::system_token parent) {
		psl_assert(!m_LockState, "cannot modify system dependencies while the state is locked");
		m_SystemInformations[child].add_dependency(parent);
	}

	void system_disconnect(details::system_token child, details::system_token parent) {
		psl_assert(!m_LockState, "cannot modify system dependencies while the state is locked");
		m_SystemInformations[child].remove_dependency(parent);
	}

	auto system_dependencies(details::system_token system) const noexcept
	  -> std::unordered_set<details::system_token> const& {
		if(auto it = m_SystemInformations.find(system); it != std::end(m_SystemInformations)) {
			return it->second.dependencies();
		}
		static std::unordered_set<system_token> empty {};
		return empty;
	}

  private:
	//------------------------------------------------------------
	// helpers
	//------------------------------------------------------------
	template <IsPack... Ts>
	auto make_filter_group() const noexcept -> psl::array<details::filter_group> {
		psl::array<details::filter_group> result {};
		result.reserve(sizeof...(Ts));
		(
		  [&result, this]<typename... Ys>(psl::utility::templates::type_pack_t<Ys...>) mutable {
			  result.emplace_back(make_filter_group<Ys...>());
		  }(decode_pack_types_t<Ts> {}),
		  ...);
		return result;
	}

	template <IsPack... Ts>
	auto make_filter_group(psl::type_pack_t<Ts...>) const noexcept -> psl::array<details::filter_group> {
		psl::array<details::filter_group> result {};
		result.reserve(sizeof...(Ts));
		(
		  [&result, this]<typename... Ys>(psl::utility::templates::type_pack_t<Ys...>) mutable {
			  result.emplace_back(make_filter_group<Ys...>());
		  }(decode_pack_types_t<Ts> {}),
		  ...);
		return result;
	}

	template <typename... Ts>
	details::filter_group make_filter_group() const noexcept {
		return details::filter_group {
		  psl::type_pack_t<Ts...> {},
		  [this]<typename T>() -> details::component_container_t* { return get_component_untyped_info<T>(); }};
	}

	void execute_command_buffer(command_buffer_t& command_buffer);

	//------------------------------------------------------------
	// filter
	//------------------------------------------------------------

	void filter(details::filter_result& data, psl::array_view<entity_t> source) const noexcept override;
	psl::array<entity_t>::iterator filter(details::filter_group const& group,
										  psl::array<entity_t>::iterator begin,
										  psl::array<entity_t>::iterator end) const noexcept override;

	void initialize(details::filter_result& data) const noexcept override;
	psl::array<details::filter_result>& get_filter_results() noexcept override {
		return m_Filters;
	}

	//------------------------------------------------------------
	// transformations
	//------------------------------------------------------------

	void transform(details::transform_result& data, psl::array_view<entity_t> source) const noexcept;
	void transform(details::transform_result& data) const noexcept;


	template <typename... Ts>
	psl::array<details::component_key_t> to_keys() const noexcept {
		return psl::array<details::component_key_t> {details::component_key_t::generate<Ts>()...};
	}

	//------------------------------------------------------------
	// system declare
	//------------------------------------------------------------

	std::pair<std::shared_ptr<details::filter_group>, std::shared_ptr<details::transform_group>>
	add_filter_group(details::filter_group& filter_group,
					 details::transform_group& transform_group,
					 psl::string_view debugName) {
		std::shared_ptr<details::transform_group> shared_transform_group {};
		auto filter_it =
		  std::find_if(std::begin(m_Filters), std::end(m_Filters), [&filter_group](const details::filter_result& data) {
			  return *data.group == filter_group;
		  });
		if(filter_it == std::end(m_Filters)) {
			m_Filters.emplace_back(
			  details::filter_result {m_FilterCounter++, {}, std::make_shared<details::filter_group>(filter_group)});
			filter_it = std::prev(std::end(m_Filters));
			initialize(*filter_it);
		}

		filter_it->group->add_debug_system_name(debugName);

		if(transform_group) {
			auto it = std::find_if(
			  std::begin(filter_it->transformations),
			  std::end(filter_it->transformations),
			  [&transform_group](const details::transform_result& data) { return *data.group == transform_group; });
			if(it == std::end(filter_it->transformations)) {
				filter_it->transformations.emplace_back(
				  details::transform_result {{}, {}, std::make_shared<details::transform_group>(transform_group)});

				it = std::prev(std::end(filter_it->transformations));
			}

			it->group->add_debug_system_name(debugName);
			return {filter_it->group, it->group};
		}

		return {filter_it->group, {}};
	}

	template <typename Fn, typename T, typename pack_type>
	auto create_system_tick_functional(Fn& fn, T* ptr) const noexcept {
		using function_args	 = typename psl::templates::func_traits<typename std::decay<Fn>::type>::arguments_t;
		using signature_info = details::signature_info<function_args>;

		if constexpr(std::is_member_function_pointer<Fn>::value) {
			return [fn, ptr](psl::ecs::info_t& info, psl::array<details::dependency_pack> packs) -> void {
				if constexpr(signature_info::has_info) {
					auto tuple_argument_list =
					  std::tuple_cat(std::tuple<T*, psl::ecs::info_t&>(ptr, info),
									 details::compress_from_dependency_pack(pack_type {}, packs));

					std::apply(fn, std::move(tuple_argument_list));
				} else {
					auto tuple_argument_list =
					  std::tuple_cat(std::tuple<T*>(ptr), details::compress_from_dependency_pack(pack_type {}, packs));
					std::apply(fn, std::move(tuple_argument_list));
				}
			};
		} else {
			return [fn](psl::ecs::info_t& info, psl::array<details::dependency_pack> packs) -> void {
				if constexpr(signature_info::has_info) {
					auto tuple_argument_list = std::tuple_cat(
					  std::tuple<psl::ecs::info_t&>(info), details::compress_from_dependency_pack(pack_type {}, packs));

					std::apply(fn, std::move(tuple_argument_list));
				} else {
					auto tuple_argument_list = details::compress_from_dependency_pack(pack_type {}, packs);
					std::apply(fn, std::move(tuple_argument_list));
				}
			};
		}
	}

	template <typename Fn, typename T = void>
	auto declare_impl(threading threading,
					  Fn&& fn,
					  T* ptr,
					  psl::string_view debugName					  = "",
					  std::optional<system_group_t> systemGroup		  = std::nullopt,
					  std::optional<transient_system_tag_t> transient = std::nullopt) {
		using function_args	  = typename psl::templates::func_traits<typename std::decay<Fn>::type>::arguments_t;
		using signature_info  = details::signature_info<function_args>;
		using pack_type		  = typename signature_info::packs;
		auto filter_groups	  = make_filter_group(pack_type {});
		auto transform_groups = []<typename... Ts>(psl::type_pack_t<Ts...>) -> psl::array<details::transform_group> {
			return psl::array<details::transform_group> {details::transform_group(decode_pack_types_t<Ts> {})...};
		}(pack_type {});
		auto pack_generator = [this]() {
			return details::expand_to_dependency_pack(
			  pack_type {},
			  [this]<typename Z>() -> details::component_container_t* { return get_component_untyped_info<Z>(); });
		};
		// make sure systems don't have any non-basic filter operations if they are part of a system group
		// the reason for this is that the tracking for component lifetime events is not done on a per system level
		// but handled by the storage of the components themselves. This means that if a group is ticked on a different
		// cadence than every tick, the system would miss out on component lifetime events.
		if(systemGroup != std::nullopt) {
			for(auto const& filter_group : filter_groups) {
				if(!filter_group.is_basic_filter()) {
					throw std::runtime_error(
					  "system groups can only contain basic filter operations, no on_add, on_break, on_combine, or "
					  "on_remove");
				}
			}
		}

		auto system_tick = create_system_tick_functional<Fn, T, pack_type>(fn, ptr);
		auto& sys_info	 = (m_LockState) ? m_NewSystemInformations : m_SystemInformations;

		psl::array<std::shared_ptr<details::filter_group>> shared_filter_groups;
		psl::array<std::shared_ptr<details::transform_group>> shared_transform_groups;

		for(size_t i = 0; i < filter_groups.size(); ++i) {
			auto [shared_filter, transform_filter] = add_filter_group(filter_groups[i], transform_groups[i], debugName);
			shared_filter_groups.emplace_back(shared_filter);
			shared_transform_groups.emplace_back(transform_filter);
		}

		auto system_id = details::system_token {this, ++m_SystemCounter};

		sys_info.emplace(system_id,
						 details::system_information {threading,
													  std::move(pack_generator),
													  std::move(system_tick),
													  std::move(shared_filter_groups),
													  std::move(shared_transform_groups),
													  system_id,
													  debugName,
													  signature_info::is_const});


		if(systemGroup != std::nullopt) {
			auto it = m_SystemGroups.find(systemGroup->m_Id);
			psl_assert(it != std::end(m_SystemGroups),
					   "system group not found, are you sure you registered it with this container? Do note that "
					   "groups do not persist a reset.");

			m_SystemGroupIndices.emplace(system_id);
			it->second.push_back(system_id);
		}

		m_SystemGroups[0].push_back(system_id);	   // always add to the default group
		if(transient.has_value()) {
			m_ToRevoke.push_back(system_id);
		}
		return system_id;
	}

	psl::array<std::unique_ptr<info_t>> m_InfoBuffer {};
	mutable psl::array<details::filter_result> m_Filters {};
	std::unordered_map<system_token, details::system_information> m_SystemInformations {};

	psl::array<details::system_token> m_ToRevoke {};
	std::unordered_map<system_token, details::system_information> m_NewSystemInformations {};
	std::unordered_map<size_t, psl::array<details::system_token>> m_SystemGroups {};
	std::unordered_set<details::system_token> m_SystemGroupIndices {};
	size_t m_SystemGroupCounter {1};

	std::unique_ptr<psl::ecs::details::system_handler_t> m_SystemHandler {nullptr};

	size_t m_LockState {0};
	size_t m_Tick {0};
	size_t m_SystemCounter {0};
	size_t m_FilterCounter {0};
	entity_t::size_type m_MinEntitiesPerWorker {1024};
};
}	 // namespace psl::ecs
