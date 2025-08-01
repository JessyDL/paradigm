#pragma once
#include "psl/ecs/component_traits.hpp"
#include <strtype/strtype.hpp>
#include <tuple>
#include <type_traits>

namespace psl::ecs::details {
// todo(jdl): pending mutations should be handled before serialization
template <typename T>
struct mutate_instruction_t : public T {};

template <typename T>
struct is_mutate_instruction_t : public std::false_type {};

template <typename T>
struct is_mutate_instruction_t<mutate_instruction_t<T>> : public std::true_type {};

template <typename T>
struct mutate_instruction_target {};

template <typename T>
struct mutate_instruction_target<mutate_instruction_t<T>> {
	using type = T;
};

template <typename T>
struct mutate_instruction_internal_type {
	using type = T;
};

template <typename T>
struct mutate_instruction_internal_type<mutate_instruction_t<T>> {
	using type = T;
};

template <typename T>
using mutate_instruction_underlying_t = typename mutate_instruction_internal_type<T>::type;


template <typename T>
concept IsMutateInstruction = is_mutate_instruction_t<std::remove_cvref_t<T>>::value;

}	 // namespace psl::ecs::details

// we need to specialize to get around the issue of templated component key generation
template <typename T>
struct psl::ecs::component_trait_name_t<psl::ecs::details::mutate_instruction_t<T>> {
	static constexpr auto name {strtype::stringify_typename<T>().prefix("MUT_")};
};
