#pragma once
#include "../command_buffer.hpp"
#include "../pack.hpp"
#include "component_key.hpp"
#include "psl/array_view.hpp"
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
		   size_t frame) noexcept
		: state(state), command_buffer(state), dTime(dTime), rTime(rTime), tick(frame) {}

	const state_t& state;
	command_buffer_t command_buffer;
	std::chrono::duration<float> dTime;
	std::chrono::duration<float> rTime;
	size_t tick;
};
}	 // namespace psl::ecs

namespace psl::ecs::details {
/// \brief describes a set of dependencies for a given system
///
/// systems can have various dependencies, for example a movement system could have
/// dependencies on both a psl::ecs::components::transform component and a psl::ecs::components::renderable
/// component. This dependency will output a set of psl::ecs::entity's that have all required
/// psl::ecs::components present. Certain systems could have sets of dependencies, for example the render
/// system requires knowing about both all `psl::ecs::components::renderable` that have a
/// `psl::ecs::components::transform`, but also needs to know all `psl::ecs::components::camera's`. So that
/// system would require several dependency_pack's.
class dependency_pack {
	struct indirect_storage_t {
		psl::array<entity> indices {};
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

	auto _create_dependency_filters_impl(psl::array_view<entity>) {}
	auto _create_dependency_filters_impl(psl::array_view<entity const>) {}

	template <typename T>
	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<T, entity>) {
		constexpr auto id = details::component_key_t::generate<T>();
		m_IndirectReadWriteBindings.emplace(id, indirect_storage_t {});
	}
	template <typename T>
	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<T const, entity>) {
		constexpr auto id = details::component_key_t::generate<T>();
		m_IndirectReadBindings.emplace(id, indirect_storage_t {});
	}

	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<entity, entity>) {}
	auto _create_dependency_filters_impl(psl::ecs::details::indirect_array_t<entity const, entity>) {}

	template <typename F>
	void select_impl(std::vector<component_key_t>& target) {
		if constexpr(!std::is_same<typename std::decay<F>::type, psl::ecs::entity>::value) {
			using component_t			  = F;
			constexpr component_key_t key = details::component_key_t::generate<component_t>();
			target.emplace_back(key);
			m_Sizes[key] = sizeof(component_t);
		}
	}

	template <std::size_t... Is, typename T>
	auto select(std::index_sequence<Is...>, T, std::vector<component_key_t>& target) {
		(select_impl<typename std::tuple_element<Is, T>::type>(target), ...);
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
		if constexpr(std::is_same<T, psl::ecs::entity>::value) {
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
	auto fill_in(psl::type_pack_t<psl::ecs::details::indirect_array_t<T, psl::ecs::entity>>)
	  -> psl::ecs::details::indirect_array_t<T, psl::ecs::entity> {
		if constexpr(std::is_same<std::remove_const_t<T>, psl::ecs::entity>::value) {
			// todo: this is a temporary hack untill mixed packs can be done. Ideally entities are an array_view not an indirect_array_t
			std::vector<entity> indices {};
			indices.resize(m_Entities.size());
			std::iota(std::begin(indices), std::end(indices), entity {0});
			return psl::ecs::details::indirect_array_t<T, psl::ecs::entity>(indices, m_Entities.data());
		} else {
			constexpr component_key_t id = details::component_key_t::generate<T>();
			if constexpr(std::is_const<T>::value) {
				return psl::ecs::details::indirect_array_t<T, psl::ecs::entity>(m_IndirectReadBindings[id].indices,
																				(T*)m_IndirectReadBindings[id].data);
			} else {
				return psl::ecs::details::indirect_array_t<T, psl::ecs::entity>(
				  m_IndirectReadWriteBindings[id].indices, (T*)m_IndirectReadWriteBindings[id].data);
			}
		}
	}

	template <std::size_t... Is, typename T>
	T to_pack_impl(std::index_sequence<Is...>, psl::type_pack_t<T>) {
		using pack_type = typename T::pack_type;
		using range_t	= typename pack_type::range_t;
		if constexpr(IsAccessDirect<typename T::access_type>) {
			return T {pack_type(fill_in(psl::type_pack_t<typename std::tuple_element<Is, range_t>::type>())...)};
		} else {
			return T {pack_type(fill_in(psl::type_pack_t<typename std::tuple_element<Is, range_t>::type>())...)};
		}
	}


  public:
	template <typename T>
	dependency_pack(psl::type_pack_t<T>, bool seedWithPrevious = false)
		: m_IsPartial(IsPackPartial<typename T::policy_type>), m_IsIndirect(IsAccessIndirect<typename T::access_type>) {
		orderby =
		  [](psl::array<entity>::iterator begin, psl::array<entity>::iterator end, const psl::ecs::state_t& state) {};
		using pack_type = T;
		create_dependency_filters(
		  std::make_index_sequence<std::tuple_size_v<typename pack_type::pack_type::range_t>> {},
		  psl::type_pack_t<typename pack_type::pack_type> {});
		select(std::make_index_sequence<std::tuple_size<typename pack_type::filter_type>::value> {},
			   typename pack_type::filter_type {},
			   filters);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::add_type>::value> {},
			   typename pack_type::add_type {},
			   (seedWithPrevious) ? filters : on_add);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::remove_type>::value> {},
			   typename pack_type::remove_type {},
			   on_remove);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::break_type>::value> {},
			   typename pack_type::break_type {},
			   on_break);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::combine_type>::value> {},
			   typename pack_type::combine_type {},
			   (seedWithPrevious) ? filters : on_combine);
		select(std::make_index_sequence<std::tuple_size<typename pack_type::except_type>::value> {},
			   typename pack_type::except_type {},
			   except);
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

	bool allow_partial() const noexcept { return m_IsPartial; };
	constexpr inline bool is_partial_pack() const noexcept { return m_IsPartial; };
	constexpr inline bool is_full_pack() const noexcept { return !m_IsPartial; };
	constexpr inline bool is_direct_access() const noexcept { return !m_IsIndirect; };
	constexpr inline bool is_indirect_access() const noexcept { return m_IsIndirect; };

	size_t size_per_element() const noexcept {
		size_t res {0};
		if(!m_IsIndirect) {
			for(const auto& binding : m_RBindings) {
				res += m_Sizes.at(binding.first);
			}

			for(const auto& binding : m_RWBindings) {
				res += m_Sizes.at(binding.first);
			}
		} else {
			for(const auto& binding : m_IndirectReadBindings) {
				res += m_Sizes.at(binding.first);
			}

			for(const auto& binding : m_IndirectReadWriteBindings) {
				res += m_Sizes.at(binding.first);
			}
		}
		return res;
	}

	template <typename T>
	size_t size_of() const noexcept {
		constexpr component_key_t int_id = details::component_key_t::generate<T>();
		return m_Sizes.at(int_id);
	}

	size_t size_of(component_key_t key) const noexcept { return m_Sizes.at(key); }

	size_t entities() const noexcept { return m_Entities.size(); }
	dependency_pack slice(size_t begin, size_t end) const noexcept {
		auto cpy = make_partial_copy();

		cpy.m_Entities =
		  psl::array_view<entity>(std::next(m_Entities.begin(), begin), std::next(m_Entities.begin(), end));

		for(const auto& binding : m_RBindings) {
			auto size = cpy.m_Sizes[binding.first];

			std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
			std::uintptr_t end_mem	 = (std::uintptr_t)binding.second.data() + (end * size);
			cpy.m_RBindings[binding.first] =
			  psl::array_view<std::uintptr_t> {(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
		}
		for(const auto& binding : m_RWBindings) {
			auto size = cpy.m_Sizes[binding.first];
			// binding.second = binding.second.slice(size * begin, size * end);
			std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
			std::uintptr_t end_mem	 = (std::uintptr_t)binding.second.data() + (end * size);
			cpy.m_RWBindings[binding.first] =
			  psl::array_view<std::uintptr_t> {(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
		}

		for(const auto& binding : m_IndirectReadBindings) {
			auto size										  = cpy.m_Sizes[binding.first];
			cpy.m_IndirectReadBindings[binding.first].indices = psl::array<entity>(
			  std::next(binding.second.indices.begin(), begin), std::next(binding.second.indices.begin(), end));
			cpy.m_IndirectReadBindings[binding.first].data = binding.second.data;
		}

		for(const auto& binding : m_IndirectReadWriteBindings) {
			auto size											   = cpy.m_Sizes[binding.first];
			cpy.m_IndirectReadWriteBindings[binding.first].indices = psl::array<entity>(
			  std::next(binding.second.indices.begin(), begin), std::next(binding.second.indices.begin(), end));
			cpy.m_IndirectReadWriteBindings[binding.first].data = binding.second.data;
		}
		return cpy;
	}

  private:
	dependency_pack() = default;
	auto make_partial_copy() const noexcept -> dependency_pack {
		dependency_pack cpy {};
		cpy.m_Sizes		 = m_Sizes;
		cpy.filters		 = filters;
		cpy.on_add		 = on_add;
		cpy.on_remove	 = on_remove;
		cpy.except		 = except;
		cpy.on_combine	 = on_combine;
		cpy.on_break	 = on_break;
		cpy.on_condition = on_condition;
		cpy.orderby		 = orderby;
		cpy.m_IsPartial	 = m_IsPartial;
		cpy.m_IsIndirect = m_IsIndirect;
		return cpy;
	}

	psl::array_view<psl::ecs::entity> m_Entities {};
	std::unordered_map<component_key_t, size_t> m_Sizes {};
	std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RBindings;
	std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RWBindings;
	std::unordered_map<component_key_t, indirect_storage_t> m_IndirectReadBindings;
	std::unordered_map<component_key_t, indirect_storage_t> m_IndirectReadWriteBindings;

	std::vector<component_key_t> filters {};
	std::vector<component_key_t> on_add {};
	std::vector<component_key_t> on_remove {};
	std::vector<component_key_t> except {};
	std::vector<component_key_t> on_combine {};
	std::vector<component_key_t> on_break {};

	std::vector<std::function<psl::array<
	  entity>::iterator(psl::array<entity>::iterator, psl::array<entity>::iterator, const psl::ecs::state_t&)>>
	  on_condition {};

	std::function<void(psl::array<entity>::iterator, psl::array<entity>::iterator, const psl::ecs::state_t&)>
	  orderby {};
	bool m_IsPartial  = false;
	bool m_IsIndirect = false;
};

template <typename... Ts>
std::vector<dependency_pack> expand_to_dependency_pack(psl::type_pack_t<Ts...>, bool seedWithPrevious = false) {
	std::vector<dependency_pack> res;
	res.reserve(sizeof...(Ts));
	(std::invoke([&]() { res.emplace_back(dependency_pack(psl::type_pack_t<Ts> {}, seedWithPrevious)); }), ...);
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
	constexpr system_token(size_t id) noexcept : id(id) {};

  public:
	constexpr bool operator==(const system_token& other) const noexcept { return other.id == id; }
	constexpr bool operator!=(const system_token& other) const noexcept { return other.id != id; }

  private:
	size_t id {};
};
class system_information final {
  public:
	using pack_generator_type	= std::function<std::vector<details::dependency_pack>(bool)>;
	using system_invocable_type = std::function<void(psl::ecs::info_t&, std::vector<details::dependency_pack>)>;
	system_information()		= default;
	system_information(psl::ecs::threading threading,
					   pack_generator_type&& generator,
					   system_invocable_type&& invocable,
					   psl::array<std::shared_ptr<details::filter_group>> filters,
					   psl::array<std::shared_ptr<details::transform_group>> transforms,
					   size_t id,
					   bool seedWithExisting	  = false,
					   psl::string_view debugName = "")
		: m_Threading(threading), m_PackGenerator(std::move(generator)), m_System(std::move(invocable)),
		  m_Filters(filters), m_Transforms(transforms), m_SeedWithExisting(seedWithExisting), m_DebugName(debugName),
		  m_ID(id) {};
	~system_information()									 = default;
	system_information(const system_information&)			 = default;
	system_information(system_information&&)				 = default;
	system_information& operator=(const system_information&) = default;
	system_information& operator=(system_information&&)		 = default;

	std::vector<details::dependency_pack> create_pack() { return std::invoke(m_PackGenerator, false); }
	bool seed_with_previous() const noexcept { return m_SeedWithExisting; };
	void operator()(psl::ecs::info_t& info, std::vector<details::dependency_pack> packs) {
		m_SeedWithExisting = false;
		std::invoke(m_System, info, packs);
	}

	system_invocable_type& system() { return m_System; };

	psl::ecs::threading threading() const noexcept { return m_Threading; };

	system_token id() const noexcept { return m_ID; }

	auto filters() const noexcept { return m_Filters; }
	auto transforms() const noexcept { return m_Transforms; }

  private:
	psl::ecs::threading m_Threading = threading::sequential;
	pack_generator_type m_PackGenerator;
	system_invocable_type m_System;
	psl::array<std::shared_ptr<details::filter_group>> m_Filters {};
	psl::array<std::shared_ptr<details::transform_group>> m_Transforms {};
	bool m_SeedWithExisting {false};
	psl::string_view m_DebugName {};
	system_token m_ID {0};
};
}	 // namespace psl::ecs::details
