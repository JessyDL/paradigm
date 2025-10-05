#pragma once
#include <experimental/meta>
#include <fmt/ranges.h>
#include <ranges>
#include <utility>

namespace serialization {
enum class mode_t { opt_in, opt_out };
struct container_t {
	mode_t mode = mode_t::opt_in;
};
struct field_t {
	static constexpr auto UNVERSIONED = size_t {0};
	bool optional					  = false;
	size_t version					  = UNVERSIONED;
};

template <psl::details::fixed_astring Name>
struct name_t {
	static constexpr auto name = Name;
};

template <psl::details::fixed_astring... Names>
struct alternative_names_t {
  private:
	static constexpr std::array<std::string_view const, sizeof...(Names)> names_storage {Names...};

  public:
	static constexpr std::span<std::string_view const> names {names_storage};
};

template <typename... Ts>
struct consumer_of_t {
	using type = std::tuple<Ts...>;
};

static consteval auto get_annotations_of(std::meta::info dm) {
	auto notes = annotations_of(dm);
	std::erase_if(notes, [](std::meta::info ann) { return parent_of(type_of(ann)) != ^^serialization; });
	return notes;
}

static consteval auto has_annotations_of(std::meta::info dm) {
	return !define_static_array(get_annotations_of(dm)).empty();
}


template <std::meta::info DM>
consteval auto alternative_names_for() {
	auto constexpr notes = define_static_array(serialization::get_annotations_of(DM));
	if constexpr(std::any_of(notes.begin(), notes.end(), [](std::meta::info note) {
					 return has_template_arguments(type_of(note)) &&
							template_of(type_of(note)) == ^^serialization::alternative_names_t;
				 })) {
		template for(constexpr auto note : notes) {
			if constexpr(has_template_arguments(type_of(note)) &&
						 template_of(type_of(note)) == ^^serialization::alternative_names_t) {
				return std::meta::extract<typename[:type_of(note):]>(note).names;
			}
		}
	} else {
		return std::span<std::string_view> {};
	}
	std::unreachable();
}

template <typename Type, std::meta::access_context Context>
consteval auto nonstatic_data_members_of() {
	std::vector<std::meta::info> result;
	constexpr auto ctx = Context;
	template for(constexpr auto member : define_static_array(nonstatic_data_members_of(^^Type, ctx))) {
		if constexpr(has_annotations_of(member)) {
			result.push_back(member);
		}
	}
	return result;
}

template <typename Type>
consteval auto nonstatic_data_members_of_unchecked() {
	std::vector<std::meta::info> result;
	constexpr auto ctx = std::meta::access_context::unchecked();
	template for(constexpr auto member : define_static_array(nonstatic_data_members_of(^^Type, ctx))) {
		if constexpr(has_annotations_of(member)) {
			result.push_back(member);
		}
	}
	return result;
}

struct accessor;

namespace impl {
	template <typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
		requires std::is_enum_v<E>
	constexpr std::optional<E> to_enum(std::string_view name) {
		if(auto split = name.find("::"); split != std::string_view::npos) {
			if(std::meta::identifier_of(^^E) != name.substr(0, split)) {
				return std::nullopt;
			}
			name = name.substr(split + 2);
		}

		template for(constexpr auto e : std::define_static_array(
					   std::meta::enumerators_of(^^E))) if(name == std::meta::identifier_of(e)) return [:e:];

		return std::nullopt;
	}

	template <std::meta::info DM, typename AnnotationType, bool IsTemplated = has_template_arguments(^^AnnotationType)>
	consteval auto has_annotation_helper() -> bool {
		auto constexpr notes = define_static_array(serialization::get_annotations_of(DM));
		if constexpr(std::any_of(notes.begin(), notes.end(), [](std::meta::info note) {
						 if constexpr(!IsTemplated) {
							 return type_of(note) == ^^AnnotationType;
						 } else {
							 return has_template_arguments(type_of(note)) &&
									template_of(type_of(note)) == template_of(^^AnnotationType);
						 }
					 })) {
			return true;
		}
		return false;
	}

	template <std::meta::info DM, typename AnnotationType, bool IsTemplated = has_template_arguments(^^AnnotationType)>
	consteval auto get_annotation_helper() {
		auto constexpr notes = define_static_array(serialization::get_annotations_of(DM));
		if constexpr(std::any_of(notes.begin(), notes.end(), [](std::meta::info note) {
						 if constexpr(!IsTemplated) {
							 return type_of(note) == ^^AnnotationType;
						 } else {
							 return has_template_arguments(type_of(note)) &&
									template_of(type_of(note)) == template_of(^^AnnotationType);
						 }
					 })) {
			template for(constexpr auto note : notes) {
				if constexpr(!IsTemplated) {
					if constexpr(type_of(note) == ^^AnnotationType) {
						return std::meta::extract<AnnotationType>(note);
					}
				} else {
					if constexpr(has_template_arguments(type_of(note)) &&
								 template_of(type_of(note)) == template_of(^^AnnotationType)) {
						return std::meta::extract<typename[:type_of(note):]>(note);
					}
				}
			}
		} else {
			return AnnotationType {};
		}
		std::unreachable();
	}

	struct field_info_t {
		template <std::meta::info Member>
		static consteval auto get() -> field_info_t {
			constexpr auto field						   = get_annotation_helper<Member, field_t>();
			constexpr auto alternative_names			   = get_annotation_helper<Member, alternative_names_t<>>();
			constexpr auto alternative_serialization_names = get_annotation_helper<Member, name_t<"">>();
			return field_info_t {.name				 = std::meta::identifier_of(Member),
								 .serialization_name = (alternative_serialization_names.name.size() == 0)
														 ? std::meta::identifier_of(Member)
														 : alternative_serialization_names.name,
								 .alternative_names	 = alternative_names.names,
								 .is_optional		 = field.optional,
								 .version			 = field.version};
		}
		std::string_view name;
		std::string_view serialization_name;
		std::span<std::string_view const> alternative_names;
		bool is_optional;
		size_t version;
	};

	template <typename ObjectType>
	struct object_info_t {
		consteval bool has_field(std::string_view name) const {
			for(auto const& field : fields) {
				if(field.name == name) {
					return true;
				}
			}
			return false;
		}

		consteval field_info_t const& get_field(std::string_view name) const {
			for(auto const& field : fields) {
				if(field.name == name) {
					return field;
				}
			}
			throw std::runtime_error("Field '" + std::string(name) + "' not found in type '" +
									 std::string(std::meta::identifier_of(^^ObjectType)) + "'");
		}
		std::string_view name				= std::meta::identifier_of(^^ObjectType);
		std::string_view serialization_name = (get_annotation_helper<^^ObjectType, name_t<"">>().name.size() == 0)
												? std::meta::identifier_of(^^ObjectType)
												: get_annotation_helper<^^ObjectType, name_t<"">>().name;
		std::array<field_info_t,
				   std::ranges::count_if(define_static_array(nonstatic_data_members_of_unchecked<ObjectType>()),
										 [](std::meta::info member) { return has_annotations_of(member); })>
		  fields = []() {
			  std::array<field_info_t,
						 std::ranges::count_if(define_static_array(nonstatic_data_members_of_unchecked<ObjectType>()),
											   [](std::meta::info member) { return has_annotations_of(member); })>
				result {};
			  size_t idx = 0;
			  template for(constexpr auto member :
						   define_static_array(nonstatic_data_members_of_unchecked<ObjectType>())) {
				  result[idx++] = field_info_t::get<member>();
			  }
			  return result;
		  }();
	};

	template <typename Type, std::meta::access_context Context, typename ObjectInfoType = object_info_t<Type>>
	consteval auto nonstatic_data_members_of() {
		std::vector<std::meta::info> result;
		constexpr auto object_info = ObjectInfoType {};
		constexpr auto ctx		   = std::meta::access_context::unchecked();
		template for(constexpr auto member : define_static_array(nonstatic_data_members_of(^^Type, ctx))) {
			if constexpr(object_info.has_field(std::meta::identifier_of(member))) {
				result.push_back(member);
				core::log->info("Found member '{}' of type '{}' with serialization annotations",
								std::meta::identifier_of(member),
								std::meta::identifier_of(^^Type));
			} else {
				core::log->warn("Member '{}' of type '{}' is missing serialization annotations",
								std::meta::identifier_of(member),
								std::meta::identifier_of(^^Type));
			}
		}
		return result;
	}

	template <typename Type, std::meta::access_context Context, typename ObjectInfoType = object_info_t<Type>>
	consteval auto nonstatic_data_members_of_count() {
		std::vector<std::meta::info> result;
		constexpr auto object_info = ObjectInfoType {};
		constexpr auto ctx		   = Context;
		template for(constexpr auto member : define_static_array(nonstatic_data_members_of(^^Type, ctx))) {
			if constexpr(object_info.has_field(std::meta::identifier_of(member))) {
				result.push_back(member);
			}
		}
		return result.size();
	}


	/// \brief This template will create a new type based on T, but only containing the fields that are annotated with
	/// serialization fields.
	template <typename T>
	struct serialize_instance_t {
	  private:
		struct internal_type {
			friend serialization::accessor;
		};
		consteval {
			constexpr auto ctx = std::meta::access_context::unchecked();
			std::vector<std::meta::info> new_members;
			template for(constexpr auto member : define_static_array(nonstatic_data_members_of(^^T, ctx))) {
				if constexpr(!has_annotations_of(member)) {
					continue;
				} else {
					new_members.push_back(
					  data_member_spec(type_of(member), {.name = std::meta::identifier_of(member)}));
				}
			}
			define_aggregate(^^internal_type, new_members);
		}

	  public:
		struct type : private internal_type {
			friend T;
			friend serialization::accessor;
		};
		type value;
	};

	template <typename U, typename T>
	concept IsInternalSerializationInstance = std::same_as<U, typename impl::serialize_instance_t<T>::internal_type>;

	template <typename Type>
	using safe_object_info_t = object_info_t<std::remove_cvref_t<
	  std::conditional_t<std::is_default_constructible_v<Type>, Type, typename serialize_instance_t<Type>::type>>>;

}	 // namespace impl

template <typename U, typename T>
concept IsSerializationInstance = std::same_as<U, typename impl::serialize_instance_t<T>::type>;

struct accessor {
	template <typename Type, typename ObjectInfoType = impl::safe_object_info_t<Type>>
	consteval auto get_members() {
		std::vector<std::meta::info> result;
		if constexpr(std::is_class_v<Type>) {
			constexpr auto object_info = ObjectInfoType {};
			constexpr auto ctx		   = std::meta::access_context::current();
			template for(constexpr auto member : define_static_array(
						   ::std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<Type>, ctx))) {
				if constexpr(object_info.has_field(std::meta::identifier_of(member))) {
					result.push_back(member);
				}
			}
			template for(constexpr auto base : define_static_array(bases_of(^^Type, ctx))) {
				auto base_result = get_members<typename[:type_of(base):], ObjectInfoType>();
				result.insert(result.end(), base_result.begin(), base_result.end());
			}
		}
		return result;
	}

	template <typename Type, typename ObjectInfoType = impl::safe_object_info_t<Type>>
	consteval auto members_count() -> size_t {
		size_t result {0};
		if constexpr(std::is_class_v<Type>) {
			constexpr auto object_info = ObjectInfoType {};
			constexpr auto ctx		   = std::meta::access_context::current();
			template for(constexpr auto member : define_static_array(
						   ::std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<Type>, ctx))) {
				if constexpr(object_info.has_field(std::meta::identifier_of(member))) {
					result++;
				}
			}
			template for(constexpr auto base : define_static_array(bases_of(^^Type, ctx))) {
				result += members_count<typename[:type_of(base):], ObjectInfoType>();
			}
		}
		return result;
	}

	template <typename Type, typename ObjectInfoType = impl::safe_object_info_t<Type>>
	consteval auto has_members() -> bool {
		return members_count<Type, ObjectInfoType>() > 0;
	}

	template <typename Type>
	consteval auto generic_has_members() -> bool {
		if constexpr(std::is_class_v<Type>) {
			constexpr auto ctx = std::meta::access_context::unchecked();
			template for(constexpr auto member : define_static_array(
						   ::std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<Type>, ctx))) {
				if constexpr(!serialization::has_annotations_of(member)) {
					continue;
				}
				return true;
			}
			template for(constexpr auto base : define_static_array(bases_of(^^Type, ctx))) {
				if(generic_has_members<typename[:type_of(base):]>()) {
					return true;
				}
			}
		}
		return false;
	}
};

namespace impl {
	template <typename Type>
	concept IsSerializableObject =
	  has_annotation_helper<^^Type, container_t>() || accessor {}.generic_has_members<Type>();
}	 // namespace impl


template <typename T>
using serialize_type_t = serialization::impl::serialize_instance_t<T>::type;

struct meta_info_t {
	struct field_t {
		std::string_view name;
		std::string_view type;
		bool is_optional;
		std::optional<std::string_view> initial_value;
		std::vector<std::string> annotations;
	};

	std::string_view name;
	std::vector<field_t> fields;

	auto to_string() const -> std::string {
		std::string result = "struct " + std::string(name) + " {\n";
		for(auto const& field : fields) {
			if(!field.annotations.empty()) {
				result += "  [[";
				for(auto const& note : field.annotations) {
					result += std::string(note);
				}
				result += " ]] \n";
			}
			result += "  " + std::string(field.type) + " " + std::string(field.name);
			if(field.is_optional) {
				result += " (optional)";
			}
			if(field.initial_value) {
				result += " = " + std::string(*field.initial_value);
			}
			result += ";\n";
		}
		result += "};\n";
		return result;
	}
};

template <typename Spec>
inline auto meta_info() -> meta_info_t {
	meta_info_t info {};
	info.name		   = std::meta::identifier_of(^^Spec);
	constexpr auto ctx = std::meta::access_context::current();
	template for(constexpr auto member : define_static_array(nonstatic_data_members_of(^^Spec, ctx))) {
		if constexpr(!serialization::has_annotations_of(member)) {
			continue;
		} else {
			constexpr auto type_new	   = type_of(member);
			constexpr bool is_optional = has_template_arguments(type_new) && template_of(type_new) == ^^std::optional;
			using type_new_t		   = typename[:type_new:];
			info.fields.push_back(meta_info_t::field_t {
			  .name			 = std::meta::identifier_of(member),
			  .type			 = std::meta::display_string_of(type_new),
			  .is_optional	 = is_optional,
			  .initial_value = ([]() -> std::optional<std::string_view> {
				  if constexpr(has_template_arguments(type_new) && template_of(type_new) == ^^std::optional) {
					  return std::nullopt;
				  } else if constexpr(std::same_as<type_new_t, bool>) {
					  return "false";
				  } else if constexpr(std::same_as<type_new_t, int> or std::same_as<type_new_t, float> or
									  std::same_as<type_new_t, double>) {
					  return "0";
				  } else if constexpr(std::same_as<type_new_t, std::string> or
									  std::same_as<type_new_t, psl::string8_t>) {
					  return "\"\"";
				  } else if constexpr(std::is_enum_v<type_new_t>) {
					  return {};
				  }
				  return std::nullopt;
			  })(),
			  .annotations =
				[]() {
					std::vector<std::string> result;
					template for(constexpr auto note : define_static_array(serialization::get_annotations_of(member))) {
						if constexpr(type_of(note) == ^^serialization::field_t) {
							auto instance = extract<serialization::field_t>(note);
							result.push_back(std::string(std::meta::display_string_of(type_of(note))) + " {" +
											 (std::string(instance.optional ? ".optional= true" : ".optional= false") +
											  (instance.version != serialization::field_t::UNVERSIONED
												 ? (", .version= " + std::to_string(instance.version))
												 : ", .version= <UNVERSIONED> ") +
											  +"}"));
						} else if constexpr(template_of(type_of(note)) == ^^serialization::alternative_names_t) {
							auto instance = extract<typename[:type_of(note):]>(note);
							std::string names;
							for(auto const& n : instance.names) {
								if(!names.empty()) {
									names += ", ";
								}
								names += std::string(n);
							}
							result.push_back(std::string(" alternative_names_t { \"") + names + "\" }");
						} else {
							result.push_back(std::string(std::meta::display_string_of(type_of(note))));
						}
					}
					return result;
				}(),
			});
		}
	}
	return info;
}

namespace impl {
	template <typename Spec, typename Result = Spec>
	constexpr auto parse(Result& result, auto& args) -> Result& {
		constexpr auto object_info = impl::object_info_t<Spec> {};

		template for(constexpr auto member :
					 define_static_array(accessor {}.get_members<Result, impl::object_info_t<Spec>>())) {
			constexpr auto field = object_info.get_field(std::meta::identifier_of(member));
			constexpr auto type	 = std::meta::type_of(member);

			using type_t = typename[:type:];
			if constexpr(impl::IsSerializableObject<type_t>) {
				auto middle = std::stable_partition(
				  args.begin(), args.end(), [&](std::pair<std::string_view, std::string_view> arg) {
					  if((arg.first.starts_with(field.serialization_name) &&
						  arg.first[field.serialization_name.size()] == '.') ||
						 std::any_of(
						   field.alternative_names.begin(), field.alternative_names.end(), [&](std::string_view name) {
							   return arg.first.starts_with(name) && arg.first[name.size()] == '.';
						   })) {
						  return false;
					  }
					  return true;
				  });
				std::vector<std::pair<std::string_view, std::string_view>> sub_args;
				for(auto it = middle; it != args.end(); ++it) {
					sub_args.push_back({it->first.substr(it->first.find('.') + 1), it->second});
				}
				args.erase(middle, args.end());
				if(!sub_args.empty()) {
					if constexpr(std::is_default_constructible_v<type_t>) {
						parse<type_t>(result.[:member:], sub_args);
					} else {
						auto result = serialization::serialize_type_t<type_t> {};
						parse<type_t>(result, sub_args);
						result.[:member:] = std::move(result);
					}
					continue;
				}
			} else {
				auto it = std::ranges::find_if(args, [&](std::pair<std::string_view, std::string_view> arg) {
					return arg.first == field.serialization_name ||
						   std::ranges::find(field.alternative_names, arg.first) != field.alternative_names.end();
				});

				if(it != args.end()) {
					if constexpr(std::is_enum_v<typename[:type:]>) {
						auto val = impl::to_enum<typename[:type:]>(it->second);
						if(!val) {
							throw std::runtime_error("Could not convert argument '" + std::string(it->second) +
													 "' to enum '" +
													 std::string(std::meta::identifier_of(^^typename[:type:])) + "'");
						}
						result.[:member:] = *val;
					} else {
						result.[:member:] = psl::utility::from_string<typename[:type:]>(it->second);
					}
					args.erase(it);
					continue;
				}
			}
			if(!field.is_optional) {
				core::log->error(
				  "Missing required argument for field '{} {}' of with serialization name '{}'{}",
				  std::meta::display_string_of(type),
				  field.name,
				  std::string(field.serialization_name),
				  field.alternative_names.empty()
					? ""
					: fmt::format(" or any of the alternative names '{}'", fmt::join(field.alternative_names, ", ")));

				throw std::runtime_error("Missing required argument for field '" +
										 std::string(std::meta::identifier_of(member)) + "'");
			}
		}
		return result;
	}
}	 // namespace impl
template <impl::IsSerializableObject Spec>
	requires(std::is_default_constructible_v<Spec>)
constexpr auto parse(std::vector<std::pair<std::string_view, std::string_view>> const& args) {
	auto args_copy = args;
	auto result	   = Spec {};
	impl::parse<Spec>(result, args_copy);
	return result;
}
template <impl::IsSerializableObject Spec>
	requires(!std::is_default_constructible_v<Spec>)
constexpr auto parse(std::vector<std::pair<std::string_view, std::string_view>> const& args) {
	auto args_copy = args;
	using type	   = serialization::serialize_type_t<Spec>;
	auto result	   = type {};
	impl::parse<Spec>(result, args_copy);
	return Spec {std::move(result)};
}
}	 // namespace serialization
