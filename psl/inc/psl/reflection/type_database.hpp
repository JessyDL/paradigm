#pragma once
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#if defined(PE_REFLECTION)
	#include "psl/reflection.hpp"
	#include <meta>
#endif

namespace psl::refl {
namespace impl {
	using db_type_id_t = uint64_t;

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
		object_type_t value_type;
		std::variant<std::monostate,
					 db_enum_type_info_t,
					 db_array_type_info_t,
					 db_object_type_info_t,
					 db_value_type_info_t>
		  data;
	};

#if defined(PE_REFLECTION)
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
		requires(IsParsable<T>)
	consteval auto make_parser_fn() {
		auto constexpr value_type = determine_object_type<T>();
		if constexpr(value_type == object_type_t::enumeration) {
			return [](void* target, std::string_view value) -> bool {
				auto res = psl::refl::impl::to_enum<T>(value);
				if(res) {
					reinterpret_cast<T*>(target)[0] = *res;
					return true;
				}
				return false;
			};
		} else if constexpr(value_type == object_type_t::value) {
			if constexpr(std::is_same_v<T, char>) {
				return [](void* target, std::string_view value) -> bool {
					reinterpret_cast<T*>(target)[0] = value[0];
					return value.size() == 1;
				};
			} else if constexpr(HasStringFromConverter<T>) {
				return [](void* target, std::string_view value) -> bool {
					try {
						if(psl::from_string<T>(value, reinterpret_cast<T*>(target)[0])) {
							return true;
						}
						return false;
					} catch(...) {
						return false;
					}
					std::unreachable();
				};
			}
		} else if constexpr(value_type == object_type_t::object) {
			// todo(jdl): implement object parsing from a string
			return [](void* target, std::span<std::span<std::string_view>> values) -> bool {
				// objects and arrays cannot be constructed from a
				// string
				return false;
			};
		} else if constexpr(value_type == object_type_t::array) {
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
#endif
}	 // namespace impl


class type_database_t;
namespace impl {
	type_database_t& GetTypeDatabase();
}

class type_database_t {
	struct type_state_t {
		impl::db_type_id_t id;
		bool complete = false;
	};

  public:
	type_database_t() = default;

	template <typename T>
	constexpr void register_type(std::string_view alternate_name = {}) {
		impl::assert_invalid_fields<T>();
#if defined(PE_REFLECTION)
		_register_type<T>(alternate_name);
#endif
	}

	size_t incomplete() const noexcept {
		return std::accumulate(m_RegisteredTypeNames.begin(),
							   m_RegisteredTypeNames.end(),
							   size_t {0},
							   [](size_t acc, auto const& pair) { return acc + (pair.second.complete ? 0 : 1); });
	}

	void print() const {
		fmt::println("Database contains {} types ({} incomplete):", m_Types.size(), incomplete());
		for(auto const& [id, type] : m_Types) {
			print_type(id);
		}
	}

	static type_database_t& global_instance();

  private:
#if defined(PE_REFLECTION)
	template <typename T>
		requires(!impl::IsParsable<T>)
	constexpr impl::db_type_id_t _register_type(std::string_view alternate_name = {}) {
		return impl::db_type_id_t {0};
	}
	template <typename T>
		requires(impl::IsParsable<T>)
	constexpr impl::db_type_id_t _register_type(std::string_view alternate_name = {}) {
		using namespace impl;
		auto spec			 = psl::refl::impl::object_info_t<T> {}.to_spec();
		db_type_id_t type_id = 0;
		if(auto it = m_RegisteredTypeNames.find(spec.name); it != m_RegisteredTypeNames.end()) {
			if(it->second.complete) {
				return it->second.id;
			}
			it->second.complete = true;
			type_id				= it->second.id;
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
		auto constexpr object_type = impl::determine_object_type<T>();
		m_RegisteredTypeNames.insert({spec.name, {type_id, true}});

		template for(constexpr auto field : psl::refl::impl::object_info_t<T>::get_fields_meta()) {
			_register_type<typename[:type_of(field):]>();
		}

		if constexpr(has_template_arguments(^^T) &&
					 (object_type == impl::object_type_t::enumeration || object_type == impl::object_type_t::array)) {
			template for(constexpr auto arg : define_static_array(template_arguments_of(^^T))) {
				using arg_t = typename[:arg:];
				_register_type<arg_t>();
			}
		}

		auto type_entry = m_Types.insert({type_id, {qualified_name(), object_type}});
		if constexpr(object_type == impl::object_type_t::enumeration) {
			std::vector<std::pair<std::string, std::string>> values;
			template for(constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^T))) {
				values.push_back({std::string(std::meta::identifier_of(e)), std::to_string(std::to_underlying([:e:]))});
			}
			type_entry.first->second.data = db_enum_type_info_t {_register_type<std::underlying_type_t<T>>(), values};
		} else if constexpr(object_type == impl::object_type_t::array) {
			using contained_type		  = typename T::value_type;
			auto value_type_id			  = _register_type<contained_type>();
			type_entry.first->second.data = db_array_type_info_t {
			  .value_type = value_type_id,
			  .factory	  = impl::make_parser_fn<T>(),
			};
		} else if constexpr(object_type == impl::object_type_t::object) {
			db_object_type_info_t object_info = {};
			object_info.factory				  = impl::make_parser_fn<T>();
			for(auto const& field : spec.fields) {
				impl::db_type_id_t field_type_id = 0;
				if(auto it = m_RegisteredTypeNames.find(field.type); it != m_RegisteredTypeNames.end()) {
					field_type_id = it->second.id;
				} else {
					field_type_id = m_NextTypeId++;
					m_RegisteredTypeNames.insert({field.type, {field_type_id, false}});
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
		} else if constexpr(object_type == impl::object_type_t::value) {
			type_entry.first->second.data = db_value_type_info_t {
			  .factory = impl::make_parser_fn<T>(),
			};
		}

		return type_id;
	}
#endif

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
	std::unordered_map<std::string_view, type_state_t> m_RegisteredTypeNames;
	impl::db_type_id_t m_NextTypeId = 1;
};

template <typename T, psl::details::fixed_astring AlternateName = "">
class register_type_t {
	static consteval bool check() {
		template for(constexpr auto ann : define_static_array(std::meta::annotations_of(^^T))) {
			if constexpr(type_of(ann) == ^^psl::refl::register_type_t<T>) {
				return true;
			}
		}
		return false;
	}

	constexpr static std::monostate init() {
		static_assert(
		  check(), "Type was not registered, did you forget to add the [[=psl::refl::register_type<T>()]] annotation?");
		impl::GetTypeDatabase().register_type<T>(AlternateName);
		return {};
	}
	inline static std::monostate instance = init();
	static constexpr std::integral_constant<std::monostate const&, instance> helper {};
};

template <typename T>
consteval register_type_t<T> register_type() {
	return {};
}
}	 // namespace psl::refl
