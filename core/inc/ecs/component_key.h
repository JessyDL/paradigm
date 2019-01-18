#pragma once
#include <cstdint>

namespace core::ecs
{
	namespace details
	{
		// added to trick the compiler to not throw away the results at compile time
		template <typename T>
		constexpr const std::uintptr_t component_key_var{0u};

		template <typename T>
		constexpr const std::uintptr_t* component_key() noexcept
		{
			return &component_key_var<T>;
		}

		template <typename T>
		using remove_all =
			typename std::remove_pointer<typename std::remove_reference<typename std::remove_cv<T>::type>::type>::type;
	}
	using component_key_t = const std::uintptr_t* (*)();
}