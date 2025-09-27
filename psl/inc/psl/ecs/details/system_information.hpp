#pragma once
#include "../command_buffer.hpp"
#include "../pack.hpp"
#include "component_key.hpp"
#include "psl/array_view.hpp"
#include "psl/ecs/details/mutate_instruction.hpp"
#include "psl/ecs/filtering.hpp"
#include "psl/template_utils.hpp"
#include <chrono>
#include <functional>

namespace psl::ecs {
enum class threading { seq = 0, sequential = seq, par = 1, parallel = par, main = 2 };

class state_t;

struct info_t {
	info_t(const state_t& state,
		   std::chrono::duration<float> dTime,
		   std::chrono::duration<float> rTime,
		   size_t tick,
		   size_t system_tick) noexcept
		: state(state), command_buffer(state), dTime(dTime), rTime(rTime), tick(tick), system_tick(system_tick) {}

	const state_t& state;
	command_buffer_t command_buffer;
	std::chrono::duration<float> dTime;
	std::chrono::duration<float> rTime;
	size_t tick;
	size_t system_tick;
};
}	 // namespace psl::ecs

namespace psl::ecs::details {
class system_invocable_task_t;

constexpr auto align(std::uintptr_t& ptr, size_t alignment) noexcept {
#pragma warning(push)
#pragma warning(disable : 4146)
	const auto orig	   = ptr;
	const auto aligned = (ptr - 1u + alignment) & -alignment;
	ptr				   = aligned;
	return aligned - orig;
#pragma warning(pop)
}

/// \brief describes a set of dependencies for a given system
///
/// systems can have various dependencies, for example a movement system could have
/// dependencies on both a psl::ecs::components::transform component and a psl::ecs::components::renderable
/// component. This dependency will output a set of psl::ecs::entity_t's that have all required
/// psl::ecs::components present. Certain systems could have sets of dependencies, for example the render
/// system requires knowing about both all `psl::ecs::components::renderable` that have a
/// `psl::ecs::components::transform`, but also needs to know all `psl::ecs::components::camera's`. So that
/// system would require several dependency_pack's.
class dependency_pack {
	friend class system_invocable_task_t;
	struct type_info_t {
		size_t size {0};
		size_t alignment {0};
	};

	struct indirect_storage_t {
		psl::array_view<entity_t::size_type> indices {};
		void* data {nullptr};
	};
	friend class psl::ecs::state_t;
	template <std::size_t... Is, typename T>
	auto create_dependency_filters(std::index_sequence<Is...>, psl::type_pack_t<T>) {
		(_create_dependency_filters_impl(
		   typename std::remove_reference<decltype(std::declval<T>().template get<Is>())>::type {}),
		 ...);
	}

	template <typename T>
	auto _create_dependency_filters_impl(psl::array_view<T>) {
		constexpr auto id = details::component_key_t::generate<T>();
		m_RWBindings.emplace(id, psl::array_view<std::uintptr_t> {});
	}

	template <typename T>
	auto _create_dependency_filters_impl(psl::array_view<T const>) {
		constexpr auto id = details::component_key_t::generate<T>();
		m_RBindings.emplace(id, psl::array_view<std::uintptr_t> {});
	}

	auto _create_dependency_filters_impl(psl::array_view<entity_t>) {}
	auto _create_dependency_filters_impl(psl::array_view<entity_t const>) {}

	template <typename T>
	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<T, entity_t::size_type>) {
		constexpr auto id = details::component_key_t::generate<T>();
		m_IndirectReadWriteBindings.emplace(id, indirect_storage_t {});
	}
	template <typename T>
	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<T const, entity_t::size_type>) {
		constexpr auto id = details::component_key_t::generate<T>();
		m_IndirectReadBindings.emplace(id, indirect_storage_t {});
	}

	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<entity_t, entity_t::size_type>) {}
	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<entity_t const, entity_t::size_type>) {}

	template <typename F, typename Fn>
	void select_impl(std::vector<cached_container_entry_t>& target, Fn& query) {
		if constexpr(!std::is_same<typename std::decay<F>::type, psl::ecs::entity_t>::value) {
			using component_t			  = F;
			constexpr component_key_t key = details::component_key_t::generate<component_t>();

			if constexpr(details::IsMutateInstruction<F>) {
				using underlying_t						 = details::mutate_instruction_underlying_t<component_t>;
				constexpr component_key_t underlying_key = details::component_key_t::generate<underlying_t>();
				target.emplace_back(underlying_key, query.template operator()<component_t>());
			} else {
				target.emplace_back(key, query.template operator()<component_t>());
			}
			m_Info[key] = type_info_t {sizeof(component_t), alignof(component_t)};
		}
	}

	template <std::size_t... Is, typename T, typename Fn>
	auto select(std::index_sequence<Is...>, T, std::vector<cached_container_entry_t>& target, Fn& query) {
		(select_impl<typename std::tuple_element<Is, T>::type>(target, query), ...);
	}

	template <typename Pred, typename... Ts>
	void select_ordering_impl(std::pair<Pred, std::tuple<Ts...>>);


	template <typename Pred, typename... Ts>
	void select_condition_impl(std::pair<Pred, std::tuple<Ts...>>);


	template <typename... Ts>
	void select_ordering(std::tuple<Ts...>) {
		(select_ordering_impl(Ts {}), ...);
	}

	template <typename... Ts>
	void select_condition(std::tuple<Ts...>) {
		(select_condition_impl(Ts {}), ...);
	}

	template <typename T>
	auto fill_in(psl::type_pack_t<psl::array_view<T>>) -> psl::array_view<T> {
		if constexpr(std::is_same<T, psl::ecs::entity_t>::value) {
			return m_Entities;
		} else {
			constexpr component_key_t id = details::component_key_t::generate<T>();
			if constexpr(std::is_const<T>::value) {
				return *(psl::array_view<T>*)&m_RBindings[id];
			} else {
				return *(psl::array_view<T>*)&m_RWBindings[id];
			}
		}
	}

	template <typename T>
	auto fill_in(psl::type_pack_t<psl::ecs::details::indirect_array_t<T, psl::ecs::entity_t::size_type>>) {
		if constexpr(std::is_same<T, psl::ecs::entity_t>::value) {
			// todo: this is a temporary hack until mixed packs can be done. Ideally entities are an array_view not an
			// indirect_array_t
			m_EntityIndices.resize(m_Entities.size());
			std::iota(std::begin(m_EntityIndices), std::end(m_EntityIndices), entity_t::size_type {0});
			return psl::ecs::details::indirect_array_t<T, psl::ecs::entity_t::size_type>(m_EntityIndices,
																						 (T*)m_Entities.data());
		} else {
			constexpr component_key_t id = details::component_key_t::generate<T>();
			if constexpr(std::is_const<T>::value) {
				auto it = m_IndirectReadBindings.find(id);
				psl_assert(it != m_IndirectReadBindings.end(), "type wasn't present in `m_IndirectReadBindings`");
				return psl::ecs::details::indirect_array_t<T, psl::ecs::entity_t::size_type>(it->second.indices,
																							 (T*)it->second.data);
			} else {
				auto it = m_IndirectReadWriteBindings.find(id);
				psl_assert(it != m_IndirectReadWriteBindings.end(),
						   "type wasn't present in `m_IndirectReadWriteBindings`");
				return psl::ecs::details::indirect_array_t<T, psl::ecs::entity_t::size_type>(it->second.indices,
																							 (T*)it->second.data);
			}
		}
	}

	template <std::size_t... Is, typename T>
	auto to_pack_impl(std::index_sequence<Is...>, psl::type_pack_t<T>) -> T {
		using pack_type = typename T::pack_type;
		using range_t	= typename pack_type::range_t;
		return T(pack_type(fill_in(psl::type_pack_t<typename std::tuple_element<Is, range_t>::type>())...));
	}


  public:
	template <typename T, typename Fn>
	dependency_pack(psl::type_pack_t<T>, Fn&& query)
		: m_IsPartial(IsPackPartial<typename T::policy_type>), m_IsIndirect(IsAccessIndirect<typename T::access_type>) {
		orderby			= [](psl::array<entity_t>::iterator begin,
					 psl::array<entity_t>::iterator end,
					 const psl::ecs::state_t& state) {};
		using pack_type = T;
		create_dependency_filters(
		  std::make_index_sequence<std::tuple_size_v<typename pack_type::pack_type::range_t>> {},
		  psl::type_pack_t<typename pack_type::pack_type> {});
		select(std::make_index_sequence<std::tuple_size<typename pack_type::filter_type>::value> {},
			   typename pack_type::filter_type {},
			   filters,
			   query);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::add_type>::value> {},
			   typename pack_type::add_type {},
			   on_add,
			   query);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::remove_type>::value> {},
			   typename pack_type::remove_type {},
			   on_remove,
			   query);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::break_type>::value> {},
			   typename pack_type::break_type {},
			   on_break,
			   query);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::on_mutate_type>::value> {},
			   typename pack_type::on_mutate_type {},
			   on_mutate,
			   query);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::combine_type>::value> {},
			   typename pack_type::combine_type {},
			   on_combine,
			   query);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::except_type>::value> {},
			   typename pack_type::except_type {},
			   except,
			   query);
		select_ordering(typename pack_type::order_by_type {});
		select_condition(typename pack_type::conditional_type {});

		static_assert(std::tuple_size<typename pack_type::order_by_type>::value < 2);

		std::sort(std::begin(filters), std::end(filters));
		filters.erase(std::unique(std::begin(filters), std::end(filters)), std::end(filters));
		std::sort(std::begin(except), std::end(except));
		auto cpy = filters;
		filters.clear();
		std::set_difference(
		  std::begin(cpy), std::end(cpy), std::begin(except), std::end(except), std::back_inserter(filters));
	};


	~dependency_pack() noexcept							   = default;
	dependency_pack(const dependency_pack& other)		   = default;
	dependency_pack(dependency_pack&& other)			   = default;
	dependency_pack& operator=(const dependency_pack&)	   = default;
	dependency_pack& operator=(dependency_pack&&) noexcept = default;


	template <typename... Ts>
	psl::ecs::pack_t<Ts...> to_pack(psl::type_pack_t<Ts...>) {
		// note: pack is constructed here, we need to figure out how to make it a view optionally
		using pack_t  = psl::ecs::pack_t<Ts...>;
		using range_t = typename pack_t::pack_type::range_t;

		return to_pack_impl(std::make_index_sequence<std::tuple_size<range_t>::value> {}, psl::type_pack_t<pack_t> {});
	}

	constexpr inline bool is_partial_pack() const noexcept {
		return m_IsPartial;
	};
	constexpr inline bool is_full_pack() const noexcept {
		return !m_IsPartial;
	};
	constexpr inline bool is_direct_access() const noexcept {
		return !m_IsIndirect;
	};
	constexpr inline bool is_indirect_access() const noexcept {
		return m_IsIndirect;
	};

	inline size_t size_per_element() const noexcept {
		size_t res {0};
		if(!m_IsIndirect) {
			for(const auto& binding : m_RBindings) {
				res += m_Info.at(binding.first).size;
			}

			for(const auto& binding : m_RWBindings) {
				res += m_Info.at(binding.first).size;
			}
		} else {
			for(const auto& binding : m_IndirectReadBindings) {
				res += m_Info.at(binding.first).size;
			}

			for(const auto& binding : m_IndirectReadWriteBindings) {
				res += m_Info.at(binding.first).size;
			}
		}
		return res;
	}

	inline size_t bindings_total_size() const noexcept {
		auto res = size_t {0};
		if(m_IsIndirect) {
			for(const auto& binding : m_IndirectReadBindings) {
				align(res, alignof(entity_t));
				res += sizeof(entity_t) * m_Entities.size();
			}
			for(const auto& binding : m_IndirectReadWriteBindings) {
				align(res, alignof(entity_t));
				res += sizeof(entity_t) * m_Entities.size();
			}
		} else {
			for(const auto& binding : m_RBindings) {
				auto const& info = m_Info.at(binding.first);
				align(res, info.alignment);
				res += info.size * m_Entities.size();
			}
			for(const auto& binding : m_RWBindings) {
				auto const& info = m_Info.at(binding.first);
				align(res, info.alignment);
				res += info.size * m_Entities.size();
			}
		}
		return res;
	}

	inline size_t align_of(component_key_t key) const noexcept {
		return m_Info.at(key).alignment;
	}

	/// \brief used in conjunction with size_per_element to determine the overall alignment of a packed array
	/// so we can safely allocate memory for it.
	inline size_t align_of_first_binding() const noexcept {
		if(m_IsIndirect) {
			if(!m_IndirectReadBindings.empty() || !m_IndirectReadWriteBindings.empty()) {
				return alignof(entity_t::size_type);
			}
		} else {
			for(const auto& binding : m_RBindings) {
				return m_Info.at(binding.first).alignment;
			}
			for(const auto& binding : m_RWBindings) {
				return m_Info.at(binding.first).alignment;
			}
		}
		return 1;
	}

	template <typename T>
	inline constexpr size_t size_of() const noexcept {
		constexpr component_key_t int_id = details::component_key_t::generate<T>();
		return m_Info.at(int_id).size;
	}

	inline size_t size_of(component_key_t key) const noexcept {
		return m_Info.at(key).size;
	}

	inline size_t entities() const noexcept {
		return m_Entities.size();
	}
	dependency_pack slice(size_t begin, size_t end) const noexcept {
		auto cpy = make_partial_copy();

		cpy.m_Entities =
		  psl::array_view<entity_t>(std::next(m_Entities.begin(), begin), std::next(m_Entities.begin(), end));

		for(const auto& binding : m_RBindings) {
			auto size = cpy.m_Info[binding.first].size;

			std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
			std::uintptr_t end_mem	 = (std::uintptr_t)binding.second.data() + (end * size);
			cpy.m_RBindings[binding.first] =
			  psl::array_view<std::uintptr_t> {(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
		}
		for(const auto& binding : m_RWBindings) {
			auto size = cpy.m_Info[binding.first].size;
			// binding.second = binding.second.slice(size * begin, size * end);
			std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
			std::uintptr_t end_mem	 = (std::uintptr_t)binding.second.data() + (end * size);
			cpy.m_RWBindings[binding.first] =
			  psl::array_view<std::uintptr_t> {(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
		}

		for(const auto& binding : m_IndirectReadBindings) {
			cpy.m_IndirectReadBindings[binding.first].indices = psl::array_view<entity_t::size_type>(
			  std::next(binding.second.indices.begin(), begin), std::next(binding.second.indices.begin(), end));
			cpy.m_IndirectReadBindings[binding.first].data = binding.second.data;
		}

		for(const auto& binding : m_IndirectReadWriteBindings) {
			cpy.m_IndirectReadWriteBindings[binding.first].indices = psl::array_view<entity_t::size_type>(
			  std::next(binding.second.indices.begin(), begin), std::next(binding.second.indices.begin(), end));
			cpy.m_IndirectReadWriteBindings[binding.first].data = binding.second.data;
		}
		return cpy;
	}

	dependency_pack from_entities(psl::array_view<psl::ecs::entity_t> entities) const noexcept {
		auto cpy	   = make_partial_copy();
		cpy.m_Entities = entities;

		for(const auto& binding : m_RBindings) {
			cpy.m_RBindings[binding.first] = psl::array_view<std::uintptr_t> {};
		}
		for(const auto& binding : m_RWBindings) {
			cpy.m_RWBindings[binding.first] = psl::array_view<std::uintptr_t> {};
		}
		for(const auto& binding : m_IndirectReadBindings) {
			cpy.m_IndirectReadBindings[binding.first].indices = psl::array_view<entity_t::size_type> {};
			cpy.m_IndirectReadBindings[binding.first].data	  = nullptr;
		}
		for(const auto& binding : m_IndirectReadWriteBindings) {
			cpy.m_IndirectReadWriteBindings[binding.first].indices = psl::array_view<entity_t::size_type> {};
			cpy.m_IndirectReadWriteBindings[binding.first].data	   = nullptr;
		}
		return cpy;
	}

	struct binding_info_t {
		component_key_t id;
		bool is_read_only;
		bool is_indirect;
	};

	std::vector<binding_info_t> get_bindings() const noexcept {
		std::vector<binding_info_t> res;
		res.reserve(m_IsIndirect ? m_IndirectReadBindings.size() + m_IndirectReadWriteBindings.size()
								 : m_RBindings.size() + m_RWBindings.size());
		if(m_IsIndirect) {
			for(const auto& binding : m_IndirectReadBindings) {
				res.push_back({binding.first, true, true});
			}
			for(const auto& binding : m_IndirectReadWriteBindings) {
				res.push_back({binding.first, false, true});
			}
		} else {
			for(const auto& binding : m_RBindings) {
				res.push_back({binding.first, true, false});
			}
			for(const auto& binding : m_RWBindings) {
				res.push_back({binding.first, false, false});
			}
		}
		return res;
	}


  private:
	dependency_pack() = default;
	auto make_partial_copy() const noexcept -> dependency_pack {
		dependency_pack cpy {};
		cpy.m_Info		 = m_Info;
		cpy.filters		 = filters;
		cpy.on_add		 = on_add;
		cpy.on_remove	 = on_remove;
		cpy.except		 = except;
		cpy.on_combine	 = on_combine;
		cpy.on_break	 = on_break;
		cpy.on_mutate	 = on_mutate;
		cpy.on_condition = on_condition;
		cpy.orderby		 = orderby;
		cpy.m_IsPartial	 = m_IsPartial;
		cpy.m_IsIndirect = m_IsIndirect;
		return cpy;
	}

	// todo this variable is a hack/workaround, and should be resolved.
	// this issue is that indirect packs can't be mixed with "owning" memory as of now, and so
	// to access entities within an indirect pack we must create a fake indices array.
	psl::array<psl::ecs::entity_t::size_type> m_EntityIndices {};
	psl::array_view<psl::ecs::entity_t> m_Entities {};
	std::unordered_map<component_key_t, type_info_t> m_Info {};
	mutable std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RBindings;
	mutable std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RWBindings;
	mutable std::unordered_map<component_key_t, indirect_storage_t> m_IndirectReadBindings;
	mutable std::unordered_map<component_key_t, indirect_storage_t> m_IndirectReadWriteBindings;

	std::vector<cached_container_entry_t> filters {};
	std::vector<cached_container_entry_t> on_add {};
	std::vector<cached_container_entry_t> on_remove {};
	std::vector<cached_container_entry_t> except {};
	std::vector<cached_container_entry_t> on_combine {};
	std::vector<cached_container_entry_t> on_break {};
	std::vector<cached_container_entry_t> on_mutate {};

	std::vector<std::function<psl::array<
	  entity_t>::iterator(psl::array<entity_t>::iterator, psl::array<entity_t>::iterator, const psl::ecs::state_t&)>>
	  on_condition {};

	std::function<void(psl::array<entity_t>::iterator, psl::array<entity_t>::iterator, const psl::ecs::state_t&)>
	  orderby {};
	bool m_IsPartial  = false;
	bool m_IsIndirect = false;
};

template <typename... Ts>
std::vector<dependency_pack> expand_to_dependency_pack(psl::type_pack_t<Ts...>) {
	std::vector<dependency_pack> res;
	res.reserve(sizeof...(Ts));
	(std::invoke([&]() { res.emplace_back(dependency_pack(psl::type_pack_t<Ts> {})); }), ...);
	return res;
}

template <typename... Ts, typename Fn>
std::vector<dependency_pack> expand_to_dependency_pack(psl::type_pack_t<Ts...>, Fn&& query) {
	std::vector<dependency_pack> res;
	res.reserve(sizeof...(Ts));
	(std::invoke([&]() { res.emplace_back(dependency_pack(psl::type_pack_t<Ts> {}, query)); }), ...);
	return res;
}
namespace {
	template <size_t... Is, typename... Ts>
	auto compress_from_dependency_pack_impl(std::index_sequence<Is...>,
											psl::type_pack_t<Ts...>,
											std::vector<dependency_pack>& pack) {
		return std::make_tuple(pack[Is].to_pack(decode_pack_types_t<Ts> {})...);
	}
}	 // namespace

template <typename... Ts>
auto compress_from_dependency_pack(psl::type_pack_t<Ts...>, std::vector<dependency_pack>& pack) {
	return compress_from_dependency_pack_impl(std::index_sequence_for<Ts...> {}, psl::type_pack_t<Ts...> {}, pack);
}

class system_information;
class system_token {
	friend class system_information;
	friend std::hash<system_token>;
	friend class psl::ecs::state_t;

	constexpr system_token(psl::ecs::state_t* owner, size_t id) noexcept : m_Owner(owner), m_Id(id) {};

  public:
	constexpr system_token() noexcept								= default;
	constexpr system_token(system_token const&) noexcept			= default;
	constexpr system_token(system_token&&) noexcept					= default;
	constexpr system_token& operator=(system_token const&) noexcept = default;
	constexpr system_token& operator=(system_token&&) noexcept		= default;
	constexpr bool operator==(const system_token& other) const noexcept {
		return m_Owner == other.m_Owner && other.m_Id == m_Id;
	}
	constexpr bool operator!=(const system_token& other) const noexcept {
		return m_Owner == other.m_Owner && other.m_Id != m_Id;
	}

	constexpr auto value() const noexcept {
		return m_Id;
	}

	constexpr auto is_valid() const noexcept {
		return m_Id != std::numeric_limits<size_t>::max();
	}

	void add_dependency(system_token& token);
	void remove_dependency(system_token& token);
	auto dependencies() const noexcept -> std::unordered_set<system_token> const&;

  private:
	psl::ecs::state_t* m_Owner {nullptr};
	size_t m_Id {std::numeric_limits<size_t>::max()};
};
}	 // namespace psl::ecs::details

template <>
struct std::hash<psl::ecs::details::system_token> {
	size_t operator()(const psl::ecs::details::system_token& token) const noexcept {
		return token.m_Id;
	}
};
namespace psl::ecs::details {
class system_information final {
	friend class psl::ecs::state_t;
	friend std::hash<system_information>;

  public:
	using pack_generator_type	= std::function<std::vector<details::dependency_pack>()>;
	using system_invocable_type = std::function<void(psl::ecs::info_t&, std::vector<details::dependency_pack>)>;
	system_information()		= default;
	system_information(psl::ecs::threading threading,
					   pack_generator_type&& generator,
					   system_invocable_type&& invocable,
					   psl::array<std::shared_ptr<details::filter_group>> filters,
					   psl::array<std::shared_ptr<details::transform_group>> transforms,
					   system_token id,
					   psl::string_view debugName,
					   bool is_const)
		: m_Threading(threading), m_PackGenerator(std::move(generator)), m_System(std::move(invocable)),
		  m_Filters(filters), m_Transforms(transforms), m_DebugName(debugName), m_ID(id), m_IsConst(is_const) {};
	~system_information()									 = default;
	system_information(const system_information&)			 = default;
	system_information(system_information&&)				 = default;
	system_information& operator=(const system_information&) = default;
	system_information& operator=(system_information&&)		 = default;

	bool operator==(const system_information& other) const noexcept {
		return m_ID == other.m_ID;
	}
	bool operator!=(const system_information& other) const noexcept {
		return m_ID != other.m_ID;
	}

	std::vector<details::dependency_pack> create_pack() {
		return std::invoke(m_PackGenerator);
	}
	void operator()(psl::ecs::info_t& info, std::vector<details::dependency_pack> packs) {
		std::invoke(m_System, info, packs);
	}

	system_invocable_type& system() {
		return m_System;
	};

	psl::ecs::threading threading() const noexcept {
		return m_Threading;
	};

	constexpr system_token id() const noexcept {
		return m_ID;
	}

	auto filters() const noexcept {
		return m_Filters;
	}
	auto transforms() const noexcept {
		return m_Transforms;
	}

	constexpr auto debug_name() const noexcept {
		return m_DebugName;
	}

	auto tick() noexcept {
		return m_Tick++;
	}

	auto is_const() const noexcept {
		return m_IsConst;
	}

	void add_dependency(system_token& token) {
		psl_assert(token.is_valid() && token != m_ID, "cannot add self as dependency");
		psl_assert(m_ID.m_Owner == token.m_Owner, "cross state dependencies are not supported");
		m_Dependencies.insert(token);
	}

	void remove_dependency(system_token& token) {
		m_Dependencies.erase(token);
	}

	auto dependencies() const noexcept -> std::unordered_set<system_token> const& {
		return m_Dependencies;
	}

  private:
	psl::ecs::threading m_Threading = threading::sequential;
	pack_generator_type m_PackGenerator;
	system_invocable_type m_System;
	psl::array<std::shared_ptr<details::filter_group>> m_Filters {};
	psl::array<std::shared_ptr<details::transform_group>> m_Transforms {};
	std::unordered_set<system_token> m_Dependencies {};
	psl::string m_DebugName {};
	system_token m_ID {};
	size_t m_Tick {0};
	bool m_IsConst {false};
};
}	 // namespace psl::ecs::details

namespace psl::ecs {
using system_token = details::system_token;
}


template <>
struct std::hash<psl::ecs::details::system_information> {
	size_t operator()(const psl::ecs::details::system_information& token) const noexcept {
		return std::hash<psl::ecs::details::system_token> {}(token.m_ID);
	}
};
