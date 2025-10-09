#pragma once

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

namespace psl::refl {
namespace impl {
	using db_type_id_t = uint64_t;

	enum class db_value_type_t { value, object, array, enumeration };

	struct db_type_field_info_t {
		std::string name;
		db_type_id_t type;
		bool is_optional = false;
		std::optional<std::string_view> initial_value;
		std::vector<std::string> annotations;
	};

	struct db_enum_type_info_t {
		db_type_id_t underlying_type;
		std::vector<std::pair<std::string, std::string>> values;
		std::function<bool(void* target, std::string_view)> factory;
	};

	struct db_array_type_info_t {
		db_type_id_t value_type;
		std::function<bool(void* target, std::span<std::string_view>)> factory;
	};

	struct db_object_type_info_t {
		std::vector<db_type_field_info_t> fields;
		std::function<bool(void* target, std::span<std::span<std::string_view>>)> factory;
	};

	struct db_value_type_info_t {
		std::function<bool(void* target, std::string_view)> factory;
	};

	struct db_type_info_t {
		std::string name;
		db_value_type_t value_type;
		std::variant<std::monostate,
					 db_enum_type_info_t,
					 db_array_type_info_t,
					 db_object_type_info_t,
					 db_value_type_info_t>
		  data;
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

	template <typename T>
	constexpr impl::db_value_type_t determine_value_type() {
		if constexpr(std::is_enum_v<T>) {
			return impl::db_value_type_t::enumeration;
		} else if constexpr(impl::IsArrayLike<T>) {
			return impl::db_value_type_t::array;
		} else if constexpr(psl::refl::impl::IsSerializableObject<T>) {
			return impl::db_value_type_t::object;
		} else {
			return impl::db_value_type_t::value;
		}
	}

	template <typename T>
	struct is_parsable : std::false_type {};

	template <typename T>
		requires(determine_value_type<T>() == db_value_type_t::object)
	struct is_parsable<T> {
		static constexpr bool value = []() {
			bool result = true;
			template for(constexpr auto field : psl::refl::impl::object_info_t<T>::get_fields_meta()) {
				result &= is_parsable<typename[:type_of(field):]>::value;
			}
			return result;
		}();
	};

	template <typename T>
		requires(determine_value_type<T>() == db_value_type_t::enumeration)
	struct is_parsable<T> : std::true_type {};

	template <typename T>
		requires(determine_value_type<T>() == db_value_type_t::value && (requires {
					 { psl::utility::from_string<T>(std::string_view {}) } -> std::same_as<T>;
				 } || std::is_same_v<T, char>))
	struct is_parsable<T> : std::true_type {};

	template <typename T>
		requires(determine_value_type<T>() == db_value_type_t::array)
	struct is_parsable<T> : is_parsable<typename T::value_type> {};

	template <typename T>
	concept IsParseAble = is_parsable<T>::value;

	template <typename T>
		requires(!IsParseAble<T>)
	consteval auto make_parser_fn() {
		return [](void*, std::span<std::string_view>) -> bool { return false; };
	}

	template <typename T>
		requires(IsParseAble<T>)
	consteval auto make_parser_fn() {
		auto constexpr value_type = determine_value_type<T>();
		if constexpr(value_type == db_value_type_t::enumeration) {
			return [](void* target, std::string_view value) -> bool {
				auto res = psl::refl::impl::to_enum<T>(value);
				if(res) {
					reinterpret_cast<T*>(target)[0] = *res;
					return true;
				}
				return false;
			};
		} else if constexpr(value_type == db_value_type_t::value) {
			if constexpr(std::is_same_v<T, char>) {
				return [](void* target, std::string_view value) -> bool {
					reinterpret_cast<T*>(target)[0] = value[0];
					return value.size() == 1;
				};
			} else if constexpr(requires {
									{ psl::utility::from_string<T>(std::string_view {}) } -> std::same_as<T>;
								}) {
				return [](void* target, std::string_view value) -> bool {
					try {
						reinterpret_cast<T*>(target)[0] = psl::utility::from_string<T>(value);
						return true;
					} catch(...) {
						return false;
					}
					std::unreachable();
				};
			}
		} else if constexpr(value_type == db_value_type_t::object) {
			// todo(jdl): implement object parsing from a string
			return [](void* target, std::span<std::span<std::string_view>> values) -> bool {
				// objects and arrays cannot be constructed from a
				// string
				return false;
			};
		} else if constexpr(value_type == db_value_type_t::array) {
			return [](void* target, std::span<std::string_view> values) -> bool {
				using contained_type = typename T::value_type;
				auto constexpr fn	 = make_parser_fn<contained_type>();
				for(auto value : values) {
					if(!fn(target, value)) {
						return false;
					}
					target = reinterpret_cast<contained_type*>(target) + 1;
				}
				return true;
			};
		}
	}
}	 // namespace impl
class type_database_t {
  public:
	type_database_t() = default;

	template <impl::IsParseAble T>
	constexpr void register_type(std::string_view alternate_name = {}) {
		_register_type<T>(alternate_name);
	}

	void print() const {
		fmt::println("Database contains {} types ({} incomplete):", m_Types.size(), m_IncompleteTypes.size());
		for(auto const& [id, type] : m_Types) {
			print_type(id);
		}
	}

  private:
	template <typename T>
		requires(!impl::IsParseAble<T>)
	constexpr impl::db_type_id_t _register_type(std::string_view alternate_name = {}) {
		return impl::db_type_id_t {0};
	}
	template <typename T>
		requires(impl::IsParseAble<T>)
	constexpr impl::db_type_id_t _register_type(std::string_view alternate_name = {}) {
		using namespace impl;
		auto spec = psl::refl::impl::object_info_t<T> {}.to_spec();
		if(auto it = m_RegisteredTypeNames.find(spec.name); it != m_RegisteredTypeNames.end()) {
			return it->second;
		}
		db_type_id_t type_id = 0;
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
		auto constexpr object_type = impl::determine_value_type<T>();
		m_RegisteredTypeNames.insert({spec.name, type_id});

		template for(constexpr auto field : psl::refl::impl::object_info_t<T>::get_fields_meta()) {
			_register_type<typename[:type_of(field):]>();
		}

		if constexpr(has_template_arguments(^^T) && (object_type == impl::db_value_type_t::enumeration ||
													 object_type == impl::db_value_type_t::array)) {
			template for(constexpr auto arg : define_static_array(template_arguments_of(^^T))) {
				using arg_t = typename[:arg:];
				_register_type<arg_t>();
			}
		}

		auto type_entry = m_Types.insert({type_id, {qualified_name(), object_type}});
		if constexpr(object_type == impl::db_value_type_t::enumeration) {
			std::vector<std::pair<std::string, std::string>> values;
			template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^T))) {
				values.push_back({std::string(std::meta::identifier_of(e)), std::to_string(std::to_underlying([:e:]))});
			}
			type_entry.first->second.data = db_enum_type_info_t {_register_type<std::underlying_type_t<T>>(), values};
		} else if constexpr(object_type == impl::db_value_type_t::array) {
			using contained_type		  = typename T::value_type;
			auto value_type_id			  = _register_type<contained_type>();
			type_entry.first->second.data = db_array_type_info_t {
			  .value_type = value_type_id,
			  .factory	  = impl::make_parser_fn<T>(),
			};
		} else if constexpr(object_type == impl::db_value_type_t::object) {
			db_object_type_info_t object_info = {};
			object_info.factory				  = impl::make_parser_fn<T>();
			for(auto const& field : spec.fields) {
				impl::db_type_id_t field_type_id = 0;
				if(!m_RegisteredTypeNames.contains(field.type) && !m_IncompleteTypes.contains(field.type)) {
					field_type_id = m_NextTypeId++;
					m_IncompleteTypes.insert({field.type, field_type_id});
				} else if(auto it = m_IncompleteTypes.find(field.type); it != m_IncompleteTypes.end()) {
					field_type_id = it->second;
				} else {
					field_type_id = m_RegisteredTypeNames.at(field.type);
				}
				object_info.fields.push_back(impl::db_type_field_info_t {
				  .name			 = std::string(field.name),
				  .type			 = field_type_id,
				  .is_optional	 = field.initial_value.has_value(),
				  .initial_value = {},
				  .annotations	 = {},
				});
			}

			type_entry.first->second.data = std::move(object_info);
		} else if constexpr(object_type == impl::db_value_type_t::value) {
			type_entry.first->second.data = db_value_type_info_t {
			  .factory = impl::make_parser_fn<T>(),
			};
		}

		return type_id;
	}

	void print_type(impl::db_type_id_t type_id, int indent = 2) const {
		if(m_Types.contains(type_id)) {
			auto const& type = m_Types.at(type_id);
			fmt::println(
			  "{:s}{}: {} [ID: {}]", std::string(indent, ' '), stringify(type.value_type, false), type.name, type_id);
			indent += 2;
			std::visit(
			  [&](auto const& arg) {
				  using T = std::decay_t<decltype(arg)>;
				  if constexpr(std::is_same_v<T, impl::db_enum_type_info_t>) {
					  fmt::println("{:s}underlying: {} [ID: {}, Size: {}]",
								   std::string(indent, ' '),
								   m_Types.at(arg.underlying_type).name,
								   arg.underlying_type,
								   arg.values.size());
					  for(auto const& [name, value] : arg.values) {
						  fmt::println("{:s}{} = {}", std::string(indent + 2, ' '), name, value);
					  }
				  } else if constexpr(std::is_same_v<T, impl::db_array_type_info_t>) {
					  fmt::println("{:s}underlying: {{{}, ...}} [ID: {}]",
								   std::string(indent, ' '),
								   m_Types.at(arg.value_type).name,
								   arg.value_type);
				  } else if constexpr(std::is_same_v<T, impl::db_object_type_info_t>) {
					  for(auto const& field : arg.fields) {
						  fmt::println("{:s}field: {} : {} [ID: {}, Optional: {}]",
									   std::string(indent, ' '),
									   field.name,
									   m_Types.at(field.type).name,
									   field.type,
									   field.is_optional);
					  }
				  } else if constexpr(std::is_same_v<T, impl::db_value_type_info_t>) {
					  // value type, nothing more to print
				  }
			  },
			  type.data);
		} else {
			fmt::println("{:s}Type[{}]: <incomplete>", std::string(indent, ' '), type_id);
		}
	}

	std::unordered_map<impl::db_type_id_t, impl::db_type_info_t> m_Types;
	std::unordered_map<impl::db_type_id_t, impl::db_enum_type_info_t> m_EnumTypes;
	std::unordered_map<std::string_view, impl::db_type_id_t> m_RegisteredTypeNames;
	std::unordered_map<std::string_view, impl::db_type_id_t> m_IncompleteTypes;
	impl::db_type_id_t m_NextTypeId = 1;
};
}	 // namespace psl::refl
