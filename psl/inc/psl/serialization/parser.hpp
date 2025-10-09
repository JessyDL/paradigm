#pragma once
#include <meta>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "psl/reflection.hpp"

// paradigm archive
namespace psl::par {
namespace impl {
	using type_id_t	  = uint64_t;
	using object_id_t = uint64_t;

	enum class value_type_t { value, object, array, enumeration };

	struct type_field_info_t {
		std::string name;
		type_id_t type;
		bool is_optional = false;
		std::optional<std::string_view> initial_value;
		std::vector<std::string> annotations;
	};

	struct type_info_t {
		std::string name;
		value_type_t value_type;
		std::vector<type_field_info_t> fields;
	};

	struct enum_type_info_t {
		type_id_t type;
		type_id_t underlying_type;
		std::vector<std::pair<std::string, std::string>> values;
	};

	struct field_info_t {
		std::string name;
		type_id_t type;
		bool is_optional = false;
		std::optional<std::string_view> initial_value;
		std::vector<std::string> annotations;
	};

	struct object_info_t {
		std::string name;
		std::vector<field_info_t> fields;
	};

	template <typename T>
	concept IsStringLike = requires(T t) {
		{ t.c_str() } -> std::convertible_to<const typename T::value_type*>;
	} || requires(T t) {
		{ t.data() } -> std::convertible_to<const typename T::value_type*>;
		typename T::traits_type;	// strings have traits_type
	};

	template <typename T>
	concept IsArrayLike = requires {
		typename T::value_type;
		requires std::same_as<typename T::value_type, typename T::value_type>;
		requires requires(T a) {
			{ a.begin() } -> std::same_as<typename T::iterator>;
			{ a.end() } -> std::same_as<typename T::iterator>;
		};
	} && (!IsStringLike<T>);


	template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
		requires std::is_enum_v<E>
	constexpr std::string_view enum_to_string(E value) {
		if constexpr(Enumerable) {
			template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
				if(value == [:e:]) {
					return std::meta::identifier_of(e);
				}
			}
		}
		return "<unnamed>";
	}

	template <typename T>
	consteval std::string_view stringify() {
		return std::meta::identifier_of(^^T);
	}

	template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
		requires std::is_enum_v<E>
	constexpr std::string stringify(E value, bool with_type = true) {
		if(with_type) {
			return std::string {std::meta::identifier_of(^^E)} + "::" + enum_to_string(value);
		}
		return std::string {enum_to_string(value)};
	}
}	 // namespace impl
class type_database_t {
	template <typename T>
	constexpr impl::value_type_t determine_value_type() {
		if constexpr(std::is_enum_v<T>) {
			return impl::value_type_t::enumeration;
		} else if constexpr(impl::IsArrayLike<T>) {
			return impl::value_type_t::array;
		} else if constexpr(psl::ser::impl::IsSerializableObject<T>) {
			return impl::value_type_t::object;
		} else {
			return impl::value_type_t::value;
		}
	}

  public:
	type_database_t() = default;

	template <typename T>
	constexpr void register_type(std::string_view alternate_name = {}) {
		_register_type<T>(alternate_name);
	}

	void print() const {
		fmt::println("Database contains {} types ({} incomplete):", m_Types.size(), m_IncompleteTypes.size());
		for(auto const& [id, type] : m_Types) {
			print_type(id);
			if(type.value_type == impl::value_type_t::enumeration) {
				print_enum(id);
			}
		}
	}

  private:
	template <typename T>
	constexpr impl::type_id_t _register_type(std::string_view alternate_name = {}) {
		using namespace impl;
		auto spec = psl::ser::to_spec<T>();
		if(auto it = m_RegisteredTypeNames.find(spec.name); it != m_RegisteredTypeNames.end()) {
			return it->second;
		}
		type_id_t type_id = 0;
		if(auto it = m_IncompleteTypes.find(spec.name); it != m_IncompleteTypes.end()) {
			type_id = it->second;
			m_IncompleteTypes.erase(it);
		} else {
			type_id = m_NextTypeId++;
		}

		auto qualified_name = [&spec, &alternate_name]() constexpr -> std::string {
			if(!alternate_name.empty()) {
				return std::string {alternate_name};
			}
			if constexpr(std::is_fundamental_v<T>) {
				return std::string {spec.name};
			} else {
				std::string name = {};
				auto parent_name = []<std::meta::info Identifier>(auto const& parent_name) constexpr -> std::string {
					if constexpr(parent_of(Identifier) == ^^::) {
						return std::string {std::meta::display_string_of(Identifier)};
					} else {
						constexpr auto parent = parent_of(Identifier);
						return parent_name.template operator()<parent>(parent_name) +
							   "::" + std::meta::display_string_of(Identifier);
					}
				};
				return parent_name.template operator()<^^T>(parent_name);
			}
		};
		auto type_entry = m_Types.insert({type_id, type_info_t {qualified_name(), determine_value_type<T>()}});
		m_RegisteredTypeNames.insert({spec.name, type_id});

		template for(constexpr auto field : psl::ser::impl::object_info_t<T>::get_fields_meta()) {
			_register_type<typename[:type_of(field):]>();
		}

		if constexpr(has_template_arguments(^^T)) {
			template for(constexpr auto arg : define_static_array(template_arguments_of(^^T))) {
				using arg_t = typename[:arg:];
				_register_type<arg_t>();
			}
		}

		if constexpr(std::is_enum_v<T>) {
			type_id_t underlying_type = _register_type<typename std::underlying_type<T>::type>();
			auto [it, inserted]		  = m_EnumTypes.insert({type_id, enum_type_info_t {type_id, underlying_type}});
			if(inserted) {
				template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^T))) {
					it->second.values.push_back(
					  {std::string(std::meta::identifier_of(e)), std::to_string(std::to_underlying([:e:]))});
				}
			}
		}

		for(auto const& field : spec.fields) {
			impl::type_id_t field_type_id = 0;
			if(!m_RegisteredTypeNames.contains(field.type) && !m_IncompleteTypes.contains(field.type)) {
				field_type_id = m_NextTypeId++;
				m_IncompleteTypes.insert({field.type, field_type_id});
			} else if(auto it = m_IncompleteTypes.find(field.type); it != m_IncompleteTypes.end()) {
				field_type_id = it->second;
			} else {
				field_type_id = m_RegisteredTypeNames.at(field.type);
			}
			type_entry.first->second.fields.push_back(impl::type_field_info_t {
			  .name			 = std::string(field.name),
			  .type			 = field_type_id,
			  .is_optional	 = field.initial_value.has_value(),
			  .initial_value = {},
			  .annotations	 = {},
			});
		}

		return type_id;
	}

	void print_type(impl::type_id_t type_id, int indent = 2) const {
		if(m_Types.contains(type_id)) {
			auto const& type = m_Types.at(type_id);
			fmt::println(
			  "{:s}Type[{} - {}]: {}", std::string(indent, ' '), type_id, stringify(type.value_type, false), type.name);
			for(auto const& field : type.fields) {
				print_type(field.type, indent + 2);
			}
		} else {
			fmt::println("{:s}Type[{}]: <incomplete>", std::string(indent, ' '), type_id);
		}
	}
	void print_enum(impl::type_id_t type_id, int indent = 2) const {
		if(m_EnumTypes.contains(type_id)) {
			auto const& enum_type = m_EnumTypes.at(type_id);
			fmt::println("{:s}  Type[{} - {}] - underlying",
						 std::string(indent, ' '),
						 enum_type.underlying_type,
						 m_Types.at(enum_type.underlying_type).name);
			for(auto const& [name, value] : enum_type.values) {
				fmt::println("{:s}  {} = {}", std::string(indent + 2, ' '), name, value);
			}
		} else {
			fmt::println("{:s}EnumType[{}]: <not an enum>", std::string(indent, ' '), type_id);
		}
	}
	std::unordered_map<impl::object_id_t, impl::object_info_t> m_Objects;
	std::unordered_map<impl::type_id_t, impl::type_info_t> m_Types;
	std::unordered_map<impl::type_id_t, impl::enum_type_info_t> m_EnumTypes;
	std::unordered_map<std::string_view, impl::type_id_t> m_RegisteredTypeNames;
	std::unordered_map<std::string_view, impl::type_id_t> m_IncompleteTypes;
	impl::type_id_t m_NextTypeId	 = 1;
	impl::object_id_t m_NextObjectId = 1;
};
}	 // namespace psl::par
