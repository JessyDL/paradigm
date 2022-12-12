#pragma once
#include <cstdint>

namespace psl::ecs
{
	enum class component_type : std::uint8_t
	{
		COMPLEX = 0,
		TRIVIAL = 1,
		FLAG	= 2,
	};

	template <typename T>
	concept IsComponentFlagType = std::is_empty_v<T>;

	template <typename T>
	concept IsComponentTrivialType = std::is_trivial_v<T> && !
	IsComponentFlagType<T>;

	template <typename T>
	concept IsComponentComplexType = !
	IsComponentTrivialType<T> && !IsComponentFlagType<T>;

	template <typename T>
	concept IsComponentSerializable = IsComponentTrivialType<T> || IsComponentFlagType<T>;

	/// \brief Specialize this type to override default behaviours, or to enable serialization
	/// \note `serializable` controls the default behaviour. Ineligible types (complex component types) will throw compile errors if you try to force serialization.
	template <typename T>
	struct component_traits
	{
		static constexpr bool serializable {false};
	};

	/// \warning Do not specialize this.
	template <typename T>
	static constexpr auto component_type_v = IsComponentFlagType<T>		 ? component_type::FLAG
											 : IsComponentTrivialType<T> ? component_type::TRIVIAL
																		 : component_type::COMPLEX;
}	 // namespace psl::ecs
