#pragma once
#include <cstdint>

namespace psl::ecs::details
{
	// added to trick the compiler to not throw away the results at compile time
	template <typename T>
	constexpr const std::uintptr_t component_key_var{0u};

	template <typename T>
	constexpr const std::uintptr_t* component_key() noexcept
	{
		static_assert(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value && std::is_trivially_destructible<T>::value);
		return &component_key_var<T>;
	}
	

	using component_key_t = const std::uintptr_t* (*)();

	template<typename T>
	component_key_t key_for()
	{
		return component_key<typename std::remove_pointer<typename std::remove_reference<typename std::remove_cv<T>::type>::type>::type>;
	};
}