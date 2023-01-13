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
	friend class psl::ecs::state_t;
	template <std::size_t... Is, typename T>
	auto create_dependency_filters(std::index_sequence<Is...>, psl::type_pack_t<T>) {
		(add(psl::type_pack_t<typename std::remove_reference<decltype(std::declval<T>().template get<Is>())>::type> {}),
		 ...);
	}

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
	psl::array_view<T> fill_in(psl::type_pack_t<psl::array_view<T>>) {
		if constexpr(std::is_same<T, psl::ecs::entity>::value) {
			return m_Entities;
		} else if constexpr(std::is_const<T>::value) {
			constexpr component_key_t int_id = details::component_key_t::generate<T>();
			return *(psl::array_view<T>*)&m_RBindings[int_id];
		} else {
			constexpr component_key_t int_id = details::component_key_t::generate<T>();
			return *(psl::array_view<T>*)&m_RWBindings[int_id];
		}
	}

	template <std::size_t... Is, typename T>
	T to_pack_impl(std::index_sequence<Is...>, psl::type_pack_t<T>) {
		using pack_t	  = T;
		using pack_view_t = typename pack_t::pack_type;
		using range_t	  = typename pack_t::pack_type::range_t;

		return T {pack_view_t(fill_in(psl::type_pack_t<typename std::tuple_element<Is, range_t>::type>())...)};
	}


  public:
	template <typename T>
	dependency_pack(psl::type_pack_t<T>, bool seedWithPrevious = false) {
		orderby =
		  [](psl::array<entity>::iterator begin, psl::array<entity>::iterator end, const psl::ecs::state_t& state) {};
		using pack_t = T;
		create_dependency_filters(std::make_index_sequence<std::tuple_size_v<typename pack_t::pack_type::range_t>> {},
								  psl::type_pack_t<T> {});
		select(std::make_index_sequence<std::tuple_size<typename pack_t::filter_type>::value> {},
			   typename pack_t::filter_type {},
			   filters);
		select(std::make_index_sequence<std::tuple_size<typename pack_t::add_type>::value> {},
			   typename pack_t::add_type {},
			   (seedWithPrevious) ? filters : on_add);
		select(std::make_index_sequence<std::tuple_size<typename pack_t::remove_type>::value> {},
			   typename pack_t::remove_type {},
			   on_remove);
		select(std::make_index_sequence<std::tuple_size<typename pack_t::break_type>::value> {},
			   typename pack_t::break_type {},
			   on_break);
		select(std::make_index_sequence<std::tuple_size<typename pack_t::combine_type>::value> {},
			   typename pack_t::combine_type {},
			   (seedWithPrevious) ? filters : on_combine);
		select(std::make_index_sequence<std::tuple_size<typename pack_t::except_type>::value> {},
			   typename pack_t::except_type {},
			   except);
		select_ordering(typename pack_t::order_by_type {});
		select_condition(typename pack_t::conditional_type {});

		static_assert(std::tuple_size<typename pack_t::order_by_type>::value < 2);

		std::sort(std::begin(filters), std::end(filters));
		filters.erase(std::unique(std::begin(filters), std::end(filters)), std::end(filters));
		std::sort(std::begin(except), std::end(except));
		auto cpy = filters;
		filters.clear();
		std::set_difference(
		  std::begin(cpy), std::end(cpy), std::begin(except), std::end(except), std::back_inserter(filters));
		if constexpr(std::is_same<psl::ecs::partial, typename pack_t::policy_type>::value) {
			m_IsPartial = true;
		}
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

	size_t size_per_element() const noexcept {
		size_t res {0};
		for(const auto& binding : m_RBindings) {
			res += m_Sizes.at(binding.first);
		}

		for(const auto& binding : m_RWBindings) {
			res += m_Sizes.at(binding.first);
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
		dependency_pack cpy {*this};

		cpy.m_Entities = cpy.m_Entities.slice(begin, end);

		for(auto& binding : cpy.m_RBindings) {
			auto size = cpy.m_Sizes[binding.first];

			std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
			std::uintptr_t end_mem	 = (std::uintptr_t)binding.second.data() + (end * size);
			binding.second = psl::array_view<std::uintptr_t> {(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
		}
		for(auto& binding : cpy.m_RWBindings) {
			auto size = cpy.m_Sizes[binding.first];
			// binding.second = binding.second.slice(size * begin, size * end);
			std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
			std::uintptr_t end_mem	 = (std::uintptr_t)binding.second.data() + (end * size);
			binding.second = psl::array_view<std::uintptr_t> {(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
		}
		return cpy;
	}

  private:
	template <typename T>
	void add(psl::type_pack_t<psl::array_view<T>>) noexcept {
		constexpr component_key_t int_id = details::component_key_t::generate<T>();
		m_RWBindings.emplace(int_id, psl::array_view<std::uintptr_t> {});
	}

	template <typename T>
	void add(psl::type_pack_t<psl::array_view<const T>>) noexcept {
		constexpr component_key_t int_id = details::component_key_t::generate<T>();
		m_RBindings.emplace(int_id, psl::array_view<std::uintptr_t> {});
	}


	void add(psl::type_pack_t<psl::array_view<psl::ecs::entity>>) noexcept {}
	void add(psl::type_pack_t<psl::array_view<const psl::ecs::entity>>) noexcept {}

  private:
	psl::array_view<psl::ecs::entity> m_Entities {};
	std::unordered_map<component_key_t, size_t> m_Sizes;
	std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RBindings;
	std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RWBindings;

	std::vector<component_key_t> filters;
	std::vector<component_key_t> on_add;
	std::vector<component_key_t> on_remove;
	std::vector<component_key_t> except;
	std::vector<component_key_t> on_combine;
	std::vector<component_key_t> on_break;

	std::vector<std::function<psl::array<
	  entity>::iterator(psl::array<entity>::iterator, psl::array<entity>::iterator, const psl::ecs::state_t&)>>
	  on_condition;

	std::function<void(psl::array<entity>::iterator, psl::array<entity>::iterator, const psl::ecs::state_t&)> orderby;
	bool m_IsPartial = false;
};

template <typename... Ts>
std::vector<dependency_pack> expand_to_dependency_pack(psl::type_pack_t<Ts...>, bool seedWithPrevious = false) {
	std::vector<dependency_pack> res;
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
