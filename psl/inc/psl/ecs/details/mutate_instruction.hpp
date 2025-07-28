#pragma once
#include <tuple>
#include <type_traits>

namespace psl::ecs::details {
template <typename T, auto... Fields>
struct mutate_instruction_fields_t {
	using target					   = T;
	static constexpr std::tuple fields = {Fields...};
	using types						   = std::tuple<std::remove_reference_t<decltype(std::declval<T>().*Fields)>...>;

	void apply(T& target) {
		// apply all the values to the target object
		((target.*Fields = std::get<std::remove_reference_t<decltype(std::declval<T>().*Fields)>>(values)), ...);
	}

	template <typename... Args>
	mutate_instruction_fields_t(Args&&... args) : values {std::forward<Args>(args)...} {}

	types values;
};

template <typename T, typename Mutator>
struct mutate_instruction_t {
	using target = T;

	void apply(T& target) {
		mutator(target);
	}

	Mutator mutator;
};

template <typename T, auto... Fields>
struct mutate_instruction_t<T, mutate_instruction_fields_t<T, Fields...>> {
	using target  = T;
	using Mutator = mutate_instruction_fields_t<T, Fields...>;

	void apply(T& target) {
		mutator.apply(target);
	}

	mutate_instruction_t(auto&&... args) : mutator {std::forward<decltype(args)>(args)...} {}

	Mutator mutator;
};

template <typename T, typename Mutator>
constexpr auto make_mutate_instruction(Mutator mutator) {
	return mutate_instruction_t<T, Mutator> {.mutator = std::move(mutator)};
}

template <typename T, auto... Fields>
constexpr auto make_mutate_instruction(auto&&... args) {
	return mutate_instruction_t<T, mutate_instruction_fields_t<T, Fields...>> {args...};
}

template <typename T>
struct is_mutate_instruction_t : std::false_type {};

template <typename T, auto... Fields>
struct is_mutate_instruction_t<mutate_instruction_t<T, mutate_instruction_fields_t<T, Fields...>>> : std::true_type {};

template <typename T, typename Mutator>
struct is_mutate_instruction_t<mutate_instruction_t<T, Mutator>> : std::true_type {};

template <typename T>
concept IsMutateInstruction = is_mutate_instruction_t<std::remove_cvref_t<T>>::value;

}	 // namespace psl::ecs::details
