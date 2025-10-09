#pragma once
#include <algorithm>
#include <meta>
#include <optional>
#include <string_view>
#include <type_traits>

#include "fmt/ranges.h"
#include "psl/details/fixed_astring.hpp"
#include "psl/string_utils.hpp"

namespace psl::ser {

enum class mode_t { opt_in, opt_out };

struct container_t {
	mode_t mode = mode_t::opt_in;
};

struct field_t {
	static constexpr auto UNVERSIONED = size_t {0};
	bool optional					  = false;
	size_t version					  = UNVERSIONED;
};

consteval field_t field(field_t f = {}) {
	return f;
}

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

struct accessor;

namespace impl {
	static consteval auto get_annotations_of(std::meta::info dm) {
		auto notes = annotations_of(dm);
		std::erase_if(notes, [](std::meta::info ann) { return parent_of(type_of(ann)) != ^^psl::ser; });
		return notes;
	}

	static consteval auto has_annotations_of(std::meta::info dm) {
		return !define_static_array(get_annotations_of(dm)).empty();
	}

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
		auto constexpr notes = define_static_array(get_annotations_of(DM));
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

	template <std::meta::info DM, typename AnnotationType>
		requires(!has_template_arguments(^^AnnotationType))
	consteval auto get_annotation_helper() {
		auto constexpr notes = define_static_array(get_annotations_of(DM));
		if constexpr(std::any_of(notes.begin(), notes.end(), [](std::meta::info note) {
						 return type_of(note) == ^^AnnotationType;
					 })) {
			template for(constexpr auto note : notes) {
				return std::meta::extract<AnnotationType>(note);
			}
		}
		return AnnotationType {};
	}

	template <std::meta::info DM, typename AnnotationType>
		requires(has_template_arguments(^^AnnotationType))
	consteval auto get_annotation_helper() {
		auto constexpr notes = define_static_array(get_annotations_of(DM));
		if constexpr(std::any_of(notes.begin(), notes.end(), [](std::meta::info note) {
						 return has_template_arguments(type_of(note)) &&
								template_of(type_of(note)) == template_of(^^AnnotationType);
					 })) {
			template for(constexpr auto note : notes) {
				if constexpr(has_template_arguments(type_of(note)) &&
							 template_of(type_of(note)) == template_of(^^AnnotationType)) {
					return std::meta::extract<typename[:type_of(note):]>(note);
				}
			}
		} else {
			return AnnotationType {};
		}
		std::unreachable();
	}

	/// \brief This template will create a new type based on T, but only containing the fields that are annotated with
	/// serialization fields.
	template <typename T>
	struct serialize_instance_t {
	  private:
		struct internal_type {
			friend psl::ser::accessor;
		};

		static consteval void make_aggregate();
		consteval {
			make_aggregate();
		}

	  public:
		struct type : private internal_type {
			friend T;
			friend psl::ser::accessor;
		};
		type value;
	};

	struct field_spec_t {
		std::string_view name;
		std::string_view type;
		std::optional<std::string_view> initial_value;
		std::vector<std::string> annotations;
	};

	struct field_info_t {
		template <std::meta::info Member>
		static consteval auto get() -> field_info_t;

		constexpr field_spec_t to_spec() const noexcept {
			return field_spec_t {
			  .name			 = name,
			  .type			 = type,
			  .initial_value = {},
			  .annotations	 = {},
			};
		}

		std::string_view name;
		std::string_view type;
		std::string_view serialization_name;
		std::span<std::string_view const> alternative_names;
		bool is_optional;
		size_t version;
	};

	struct type_spec_t {
		std::string_view name;
		std::vector<field_spec_t> fields;
	};

	template <auto... Values>
	struct value_container_t {};

	template <std::size_t N, std::size_t M, typename T>
	consteval auto array_to_tuple(const std::span<T, M>& arr) {
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return std::make_tuple(arr[Is]...);
		}(std::make_index_sequence<N> {});
	}

	template <typename ObjectType>
	struct object_info_t {
	  private:
		using EvaluatedObjectType = std::conditional_t<std::is_default_constructible_v<ObjectType>,
													   ObjectType,
													   typename serialize_instance_t<ObjectType>::type>;
		static consteval auto get_fields_array();
		using fields_array_t = decltype(get_fields_array());

	  public:
		static consteval auto get_fields_meta();

		static constexpr bool requires_annotations =
		  get_annotation_helper<^^ObjectType, container_t>().mode == mode_t::opt_out;
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
									 std::string(std::meta::identifier_of(^^EvaluatedObjectType)) + "'");
		}

		constexpr type_spec_t to_spec() const {
			type_spec_t result {};
			result.name = name;
			for(auto const& field : fields) {
				result.fields.push_back(field.to_spec());
			}
			return result;
		}

		std::string_view name				= std::meta::display_string_of(^^ObjectType);
		std::string_view serialization_name = (get_annotation_helper<^^ObjectType, name_t<"">>().name.size() == 0)
												? std::meta::display_string_of(^^ObjectType)
												: get_annotation_helper<^^ObjectType, name_t<"">>().name;

		// todo(jdl): make this an aggregate so that we can init the spec as a constexpr
		fields_array_t fields = get_fields_array();
	};

	template <std::meta::info Member>
	consteval auto field_info_t::get() -> field_info_t {
		constexpr auto get_serialization_name = []() constexpr {
			constexpr auto alt_name = get_annotation_helper<Member, psl::ser::name_t<"">>();
			if constexpr(alt_name.name.size() != 0) {
				return std::string_view {alt_name.name};
			}
			return std::string_view {std::meta::identifier_of(Member)};
		};
		constexpr auto field = get_annotation_helper<Member, field_t>();
		return field_info_t {.name				 = std::meta::identifier_of(Member),
							 .type				 = std::meta::display_string_of(type_of(Member)),
							 .serialization_name = get_serialization_name(),
							 .alternative_names	 = get_annotation_helper<Member, alternative_names_t<>>().names,
							 .is_optional		 = field.optional,
							 .version			 = field.version};
	}

	template <typename U, typename T>
	concept IsInternalSerializationInstance = std::same_as<U, typename impl::serialize_instance_t<T>::internal_type>;
}	 // namespace impl

template <typename U, typename T>
concept IsSerializationInstance = std::same_as<U, typename impl::serialize_instance_t<T>::type>;

struct accessor {
	template <typename Type>
	consteval auto get_members_untyped() {
		std::vector<std::meta::info> result;
		if constexpr(std::is_class_v<Type>) {
			constexpr auto ctx = std::meta::access_context::current();
			template for(constexpr auto member : define_static_array(
						   ::std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<Type>, ctx))) {
				constexpr auto has_annotations = impl::has_annotations_of(member);
				if constexpr(!(impl::get_annotation_helper<^^Type, container_t>().mode != psl::ser::mode_t::opt_out) ||
							 has_annotations) {
					result.push_back(member);
				}
			}
			template for(constexpr auto base : define_static_array(bases_of(^^Type, ctx))) {
				auto base_result = get_members_untyped<typename[:type_of(base):]>();
				result.insert(result.end(), base_result.begin(), base_result.end());
			}
		}
		return result;
	}

	template <typename Type>
	consteval auto get_members() {
		std::vector<std::meta::info> result;
		if constexpr(std::is_class_v<Type>) {
			constexpr auto object_info = impl::object_info_t<Type> {};
			constexpr auto ctx		   = std::meta::access_context::current();
			template for(constexpr auto member : define_static_array(
						   ::std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<Type>, ctx))) {
				if constexpr(!object_info.requires_annotations ||
							 object_info.has_field(std::meta::identifier_of(member))) {
					result.push_back(member);
				}
			}
			template for(constexpr auto base : define_static_array(bases_of(^^Type, ctx))) {
				auto base_result = get_members<typename[:type_of(base):]>();
				result.insert(result.end(), base_result.begin(), base_result.end());
			}
		}
		return result;
	}

	template <typename Type>
	consteval auto members_count() -> size_t {
		size_t result {0};
		if constexpr(std::is_class_v<Type>) {
			constexpr auto object_info = impl::object_info_t<Type> {};
			constexpr auto ctx		   = std::meta::access_context::current();
			template for(constexpr auto member : define_static_array(
						   ::std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<Type>, ctx))) {
				if constexpr(!object_info.requires_annotations ||
							 object_info.has_field(std::meta::identifier_of(member))) {
					result++;
				}
			}
			template for(constexpr auto base : define_static_array(bases_of(^^Type, ctx))) {
				result += members_count<typename[:type_of(base):]>();
			}
		}
		return result;
	}

	template <typename Type>
	consteval auto has_members() -> bool {
		return members_count<Type>() > 0;
	}

	template <typename Type>
	consteval auto generic_has_members() -> bool {
		if constexpr(std::is_class_v<Type>) {
			constexpr auto ctx = std::meta::access_context::unchecked();
			template for(constexpr auto member : define_static_array(
						   ::std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<Type>, ctx))) {
				if constexpr(!psl::ser::impl::has_annotations_of(member)) {
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
	template <typename ObjectType>
	consteval auto object_info_t<ObjectType>::get_fields_array() {
		std::array<field_info_t, accessor {}.get_members_untyped<ObjectType>().size()> result {};
		size_t idx = 0;
		template for(constexpr auto member : define_static_array(accessor {}.get_members_untyped<ObjectType>())) {
			result[idx++] = field_info_t::get<member>();
		}
		return result;
	}

	template <typename ObjectType>
	consteval auto object_info_t<ObjectType>::get_fields_meta() {
		return define_static_array(accessor {}.get_members_untyped<ObjectType>());
	}

	template <typename T>
	consteval void serialize_instance_t<T>::make_aggregate() {
		std::vector<std::meta::info> new_members;
		template for(constexpr auto member : define_static_array(accessor {}.get_members_untyped<T>())) {
			new_members.push_back(data_member_spec(type_of(member), {.name = std::meta::identifier_of(member)}));
		}
		define_aggregate(^^typename serialize_instance_t<T>::internal_type, new_members);
	}

	template <typename Type>
	concept IsSerializableObject =
	  has_annotation_helper<^^Type, container_t>() || accessor {}.generic_has_members<Type>();
}	 // namespace impl


template <typename T>
using serialize_type_t = psl::ser::impl::serialize_instance_t<T>::type;

namespace impl {
	template <typename Spec, typename Result = Spec>
	constexpr auto parse(Result& result, auto& args) -> Result& {
		constexpr auto object_info = impl::object_info_t<Spec> {};

		template for(constexpr auto member : define_static_array(accessor {}.get_members<Result>())) {
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
						parse<type_t, type_t>(result.[:member:], sub_args);
					} else {
						auto result = psl::ser::serialize_type_t<type_t> {};
						parse<type_t, psl::ser::serialize_type_t<type_t>>(result, sub_args);
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
				fmt::println("Missing required argument for field '{} {}' of with serialization name '{}'{}",
							 std::meta::display_string_of(type),
							 field.name,
							 std::string(field.serialization_name),
							 field.alternative_names.empty() ? ""
															 : fmt::format(" or any of the alternative names '{}'",
																		   fmt::join(field.alternative_names, ", ")));

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
	using type	   = psl::ser::serialize_type_t<Spec>;
	auto result	   = type {};
	impl::parse<Spec>(result, args_copy);
	return Spec {std::move(result)};
}

template <impl::IsSerializableObject Spec>
constexpr auto to_spec() -> impl::type_spec_t {
	return impl::object_info_t<Spec> {}.to_spec();
}

template <typename Spec>
	requires(!impl::IsSerializableObject<Spec>)
constexpr auto to_spec() -> impl::type_spec_t {
	return impl::object_info_t<Spec> {}.to_spec();
}
}	 // namespace psl::ser
