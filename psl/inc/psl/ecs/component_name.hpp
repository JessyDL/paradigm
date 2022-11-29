#pragma once
#include "psl/details/fixed_astring.hpp"
#include <string_view>

namespace psl::ecs
{
	namespace details
	{
		/// \brief Verify the given name is valid for component name substitution.
		/// \details In general this means all characters are valid if they are also valid for that context for typenames.
		/// This means the name has to start with an alphabetical letter or underscore. Subsequent values can be
		/// alphanumeric.
		template <psl::details::fixed_astring Name>
		consteval auto is_valid_name() -> bool
		{
			using namespace std::literals::string_view_literals;
			return static_cast<std::string_view>(Name).find_first_not_of(
					 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:<>_"sv) ==
					 std::string_view::npos &&
				   "0123456789:<>"sv.find(Name[0]) == std::string_view::npos;
		}
	}	 // namespace details

	/// \brief Inheriting this type in your component type allows you to designate a new component name.
	/// \tparam Name Sets the new name for the given component.
	template <psl::details::fixed_astring Name>
		requires(details::is_valid_name<Name>())
	struct component_name
	{
		static constexpr auto _ECS_COMPONENT_NAME {Name};
	};

	namespace details
	{
		template <typename T>
		concept HasComponentNameOverride =
		  requires() { T::_ECS_COMPONENT_NAME; } && std::is_base_of_v<component_name<T::_ECS_COMPONENT_NAME>, T>;
	}	 // namespace details
}	 // namespace psl::ecs
