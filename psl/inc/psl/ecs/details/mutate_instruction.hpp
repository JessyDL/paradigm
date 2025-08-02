#pragma once
#include "psl/ecs/component_traits.hpp"
#include <strtype/strtype.hpp>
#include <tuple>
#include <type_traits>

namespace psl::ecs::details {
template <typename T, auto Member>
struct get_field_info_t {};

template <typename T, typename FieldType, FieldType T::*Member>
struct get_field_info_t<T, Member> {
	using type		  = T;
	using field_type  = FieldType;
	using member_type = FieldType T::*;
};

// todo(jdl): pending mutations should be handled before serialization
template <typename T>
struct mutate_instruction_t final : private T {
	/// \brief Queries the given field to see if it has been mutated.
	template <auto T::*Member>
	bool has_mutated() const noexcept {
		using field_type = typename get_field_info_t<T, Member>::field_type;
		auto ptr		 = reinterpret_cast<const std::uint8_t*>(this);
		auto offset		 = reinterpret_cast<std::size_t>(&(reinterpret_cast<T const volatile*>(0)->*Member));
		auto field_size	 = sizeof(field_type);
		return std::any_of(ptr + offset, ptr + offset + field_size, [](std::uint8_t byte) { return byte != 0; });
	}
};

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
using mutate_instruction_underlying_t = typename mutate_instruction_internal_type<std::remove_cvref_t<T>>::type;


template <typename T>
concept IsMutateInstruction = is_mutate_instruction_t<std::remove_cvref_t<T>>::value;

}	 // namespace psl::ecs::details

// we need to specialize to get around the issue of templated component key generation
template <typename T>
struct psl::ecs::component_trait_name_t<psl::ecs::details::mutate_instruction_t<T>> {
	static constexpr auto name {strtype::stringify_typename<T>().prefix("MUT_")};
};
