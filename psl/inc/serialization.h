#pragma once
#include "format.h"
#include "string_utils.h"
#include "platform_utils.h"
#include "binary_utils.h"
#include <array>
#include <utility>
#include <stack>
#include <unordered_set>
#include "template_utils.h"
#include "crc32.h"
#include "logging.h"
#define SUPPORT_POLYMORPH

// we give a start offset because of the first character is used as a check
#define get_char(name, ii) ::psl::serialization::details::get_nth_character<ii + 1>(name)
#define get_char_2(str, size) get_char(str, size - 2)
#define get_char_3(str, size) get_char(str, size - 3), get_char_2(str, size)
#define get_char_4(str, size) get_char(str, size - 4), get_char_3(str, size)
#define get_char_5(str, size) get_char(str, size - 5), get_char_4(str, size)
#define get_char_6(str, size) get_char(str, size - 6), get_char_5(str, size)
#define get_char_7(str, size) get_char(str, size - 7), get_char_6(str, size)
#define get_char_8(str, size) get_char(str, size - 8), get_char_7(str, size)
#define get_char_9(str, size) get_char(str, size - 9), get_char_8(str, size)
#define get_char_10(str, size) get_char(str, size - 10), get_char_9(str, size)
#define get_char_11(str, size) get_char(str, size - 11), get_char_10(str, size)
#define get_char_12(str, size) get_char(str, size - 12), get_char_11(str, size)
#define get_char_13(str, size) get_char(str, size - 13), get_char_12(str, size)
#define get_char_14(str, size) get_char(str, size - 14), get_char_13(str, size)
#define get_char_15(str, size) get_char(str, size - 15), get_char_14(str, size)
#define get_char_16(str, size) get_char(str, size - 16), get_char_15(str, size)
#define get_char_17(str, size) get_char(str, size - 17), get_char_16(str, size)
#define get_char_18(str, size) get_char(str, size - 18), get_char_17(str, size)
#define get_char_19(str, size) get_char(str, size - 19), get_char_18(str, size)
#define get_char_20(str, size) get_char(str, size - 20), get_char_19(str, size)
#define get_char_21(str, size) get_char(str, size - 21), get_char_20(str, size)
#define get_char_22(str, size) get_char(str, size - 22), get_char_21(str, size)
#define get_char_23(str, size) get_char(str, size - 23), get_char_22(str, size)
#define get_char_24(str, size) get_char(str, size - 24), get_char_23(str, size)
#define get_char_25(str, size) get_char(str, size - 25), get_char_24(str, size)
#define get_char_26(str, size) get_char(str, size - 26), get_char_25(str, size)
#define get_char_27(str, size) get_char(str, size - 27), get_char_26(str, size)
#define get_char_28(str, size) get_char(str, size - 28), get_char_27(str, size)
#define get_char_29(str, size) get_char(str, size - 29), get_char_28(str, size)
#define get_char_30(str, size) get_char(str, size - 30), get_char_29(str, size)
#define get_char_31(str, size) get_char(str, size - 31), get_char_30(str, size)
#define get_char_32(str, size) get_char(str, size - 32), get_char_31(str, size)
#define get_char_33(str, size) get_char(str, size - 33), get_char_32(str, size)
#define get_char_34(str, size) get_char(str, size - 34), get_char_33(str, size)
#define get_char_35(str, size) get_char(str, size - 35), get_char_34(str, size)
#define get_char_36(str, size) get_char(str, size - 36), get_char_35(str, size)
#define get_char_37(str, size) get_char(str, size - 37), get_char_36(str, size)
#define get_char_38(str, size) get_char(str, size - 38), get_char_37(str, size)
#define get_char_39(str, size) get_char(str, size - 39), get_char_38(str, size)
#define get_char_40(str, size) get_char(str, size - 40), get_char_39(str, size)
#define get_char_41(str, size) get_char(str, size - 41), get_char_40(str, size)
#define get_char_42(str, size) get_char(str, size - 42), get_char_41(str, size)
#define get_char_43(str, size) get_char(str, size - 43), get_char_42(str, size)
#define get_char_44(str, size) get_char(str, size - 44), get_char_43(str, size)
#define get_char_45(str, size) get_char(str, size - 45), get_char_44(str, size)
#define get_char_46(str, size) get_char(str, size - 46), get_char_45(str, size)
#define get_char_47(str, size) get_char(str, size - 47), get_char_46(str, size)
#define get_char_48(str, size) get_char(str, size - 48), get_char_47(str, size)
#define get_char_49(str, size) get_char(str, size - 49), get_char_48(str, size)
#define get_char_50(str, size) get_char(str, size - 50), get_char_49(str, size)
#define get_char_51(str, size) get_char(str, size - 51), get_char_50(str, size)
#define get_char_52(str, size) get_char(str, size - 52), get_char_51(str, size)
#define get_char_53(str, size) get_char(str, size - 53), get_char_52(str, size)
#define get_char_54(str, size) get_char(str, size - 54), get_char_53(str, size)
#define get_char_55(str, size) get_char(str, size - 55), get_char_54(str, size)
#define get_char_56(str, size) get_char(str, size - 56), get_char_55(str, size)
#define get_char_57(str, size) get_char(str, size - 57), get_char_56(str, size)
#define get_char_58(str, size) get_char(str, size - 58), get_char_57(str, size)
#define get_char_59(str, size) get_char(str, size - 59), get_char_58(str, size)
#define get_char_60(str, size) get_char(str, size - 60), get_char_59(str, size)
#define get_char_61(str, size) get_char(str, size - 61), get_char_60(str, size)
#define get_char_62(str, size) get_char(str, size - 62), get_char_61(str, size)
#define get_char_63(str, size) get_char(str, size - 63), get_char_62(str, size)
#define get_char_64(str, size) get_char(str, size - 64), get_char_63(str, size)
#define get_char_65(str, size) get_char(str, size - 65), get_char_64(str, size)
#define get_char_66(str, size) get_char(str, size - 66), get_char_65(str, size)
#define get_char_67(str, size) get_char(str, size - 67), get_char_66(str, size)
#define get_char_68(str, size) get_char(str, size - 68), get_char_67(str, size)
#define get_char_69(str, size) get_char(str, size - 69), get_char_68(str, size)
#define get_char_70(str, size) get_char(str, size - 70), get_char_69(str, size)
#define get_char_71(str, size) get_char(str, size - 71), get_char_70(str, size)
#define get_char_72(str, size) get_char(str, size - 72), get_char_71(str, size)
#define get_char_73(str, size) get_char(str, size - 73), get_char_72(str, size)
#define get_char_74(str, size) get_char(str, size - 74), get_char_73(str, size)
#define get_char_75(str, size) get_char(str, size - 75), get_char_74(str, size)
#define get_char_76(str, size) get_char(str, size - 76), get_char_75(str, size)
#define get_char_77(str, size) get_char(str, size - 77), get_char_76(str, size)
#define get_char_78(str, size) get_char(str, size - 78), get_char_77(str, size)
#define get_char_79(str, size) get_char(str, size - 79), get_char_78(str, size)
#define get_char_80(str, size) get_char(str, size - 80), get_char_79(str, size)
#define get_char_81(str, size) get_char(str, size - 81), get_char_80(str, size)
#define get_char_82(str, size) get_char(str, size - 82), get_char_81(str, size)
#define get_char_83(str, size) get_char(str, size - 83), get_char_82(str, size)
#define get_char_84(str, size) get_char(str, size - 84), get_char_83(str, size)
#define get_char_85(str, size) get_char(str, size - 85), get_char_84(str, size)
#define get_char_86(str, size) get_char(str, size - 86), get_char_85(str, size)
#define get_char_87(str, size) get_char(str, size - 87), get_char_86(str, size)
#define get_char_88(str, size) get_char(str, size - 88), get_char_87(str, size)
#define get_char_89(str, size) get_char(str, size - 89), get_char_88(str, size)
#define get_char_90(str, size) get_char(str, size - 90), get_char_89(str, size)
#define get_char_91(str, size) get_char(str, size - 91), get_char_90(str, size)
#define get_char_92(str, size) get_char(str, size - 92), get_char_91(str, size)
#define get_char_93(str, size) get_char(str, size - 93), get_char_92(str, size)
#define get_char_94(str, size) get_char(str, size - 94), get_char_93(str, size)
#define get_char_95(str, size) get_char(str, size - 95), get_char_94(str, size)
#define get_char_96(str, size) get_char(str, size - 96), get_char_95(str, size)
#define get_char_97(str, size) get_char(str, size - 97), get_char_96(str, size)
#define get_char_98(str, size) get_char(str, size - 98), get_char_97(str, size)
#define get_char_99(str, size) get_char(str, size - 99), get_char_98(str, size)
#define get_char_100(str, size) get_char(str, size - 100), get_char_99(str, size)
#define get_char_101(str, size) get_char(str, size - 101), get_char_100(str, size)
#define get_char_102(str, size) get_char(str, size - 102), get_char_101(str, size)
#define get_char_103(str, size) get_char(str, size - 103), get_char_102(str, size)
#define get_char_104(str, size) get_char(str, size - 104), get_char_103(str, size)
#define get_char_105(str, size) get_char(str, size - 105), get_char_104(str, size)
#define get_char_106(str, size) get_char(str, size - 106), get_char_105(str, size)
#define get_char_107(str, size) get_char(str, size - 107), get_char_106(str, size)
#define get_char_108(str, size) get_char(str, size - 108), get_char_107(str, size)
#define get_char_109(str, size) get_char(str, size - 109), get_char_108(str, size)
#define get_char_110(str, size) get_char(str, size - 110), get_char_109(str, size)
#define get_char_111(str, size) get_char(str, size - 111), get_char_110(str, size)
#define get_char_112(str, size) get_char(str, size - 112), get_char_111(str, size)
#define get_char_113(str, size) get_char(str, size - 113), get_char_112(str, size)
#define get_char_114(str, size) get_char(str, size - 114), get_char_113(str, size)
#define get_char_115(str, size) get_char(str, size - 115), get_char_114(str, size)
#define get_char_116(str, size) get_char(str, size - 116), get_char_115(str, size)
#define get_char_117(str, size) get_char(str, size - 117), get_char_116(str, size)
#define get_char_118(str, size) get_char(str, size - 118), get_char_117(str, size)
#define get_char_119(str, size) get_char(str, size - 119), get_char_118(str, size)
#define get_char_120(str, size) get_char(str, size - 120), get_char_119(str, size)
#define get_char_121(str, size) get_char(str, size - 121), get_char_120(str, size)
#define get_char_122(str, size) get_char(str, size - 122), get_char_121(str, size)
#define get_char_123(str, size) get_char(str, size - 123), get_char_122(str, size)
#define get_char_124(str, size) get_char(str, size - 124), get_char_123(str, size)
#define get_char_125(str, size) get_char(str, size - 125), get_char_124(str, size)
#define get_char_126(str, size) get_char(str, size - 126), get_char_125(str, size)
#define get_char_127(str, size) get_char(str, size - 127), get_char_126(str, size)
#define get_char_128(str, size) get_char(str, size - 128), get_char_127(str, size)

#define const_str(str, size) ::psl::serialization::details::check_str<size>(str), get_char_##size(str, size)

namespace psl
{
	template <char... C>
	class template_string final
	{
	  public:
		static constexpr char const* data() noexcept { return &internal_buffer[0]; }

		static constexpr unsigned int size() noexcept { return sizeof...(C); };

		static constexpr char const* cbegin() noexcept { return &internal_buffer[0]; }

		static constexpr char const* cend() noexcept { return &internal_buffer[sizeof...(C)]; }

	  private:
		static constexpr std::array<char const, sizeof...(C) + 1> internal_buffer = {C..., '\0'};
		static constexpr size_t internal_size									  = sizeof...(C);
	};

} // namespace psl

/// \brief serialization primitives and helpers
///
/// In this namespace you'll find the utilities to help you in serializing and deserializing
/// data with (almost) zero-cost overhead, with exception being the polymorphic variant which has
/// a lookup cost.
namespace psl::serialization
{
	namespace details
	{
		class dummy;
	}
	// forward declarations
	template <typename T, char... Char>
	struct property;

	template <typename T>
	struct ptr;

	template <typename T>
	class polymorphic;

	class serializer;

	// todo check this

	template <typename S>
	void serialize(S& x, details::dummy& y){};

	// special classes to mark classes that can act as serialization codecs
	class codec
	{};
	class encoder : codec
	{};
	class decoder : codec
	{};

	class encode_to_format;
	class decode_from_format;
	template <typename CODEC>
	struct vtable
	{
		void (*serialize)(void* this_, CODEC& s);
	};

	struct polymorphic_base
	{
		virtual ~polymorphic_base(){};
		virtual uint64_t PolymorphicID() const = 0;
	};

	class invocable_wrapper_base
	{
	  public:
		virtual ~invocable_wrapper_base() = default;
		virtual void* operator()()		  = 0;
	};

	template <typename INV>
	class invocable_wrapper final : public invocable_wrapper_base
	{
	  public:
		invocable_wrapper(INV&& inv) : t(std::forward<INV>(inv))
		{
#if !defined(PLATFORM_ANDROID) // todo android currently does not support is_invocable
			static_assert(std::is_invocable<INV>::value, "Must be an invocable");
#endif
		};
		void* operator()() override { return t(); };

		INV t;
	};

	struct polymorphic_data
	{
		uint64_t id;
		vtable<encode_to_format> const* encoder;
		vtable<decode_from_format> const* decoder;
		invocable_wrapper_base* factory;

		std::vector<std::pair<psl::string8_t, uint64_t>> derived;
		~polymorphic_data() { delete(factory); };
	};


	// friendly helper to access privates, see serializer's static_assert for usage
	class accessor
	{
	  public:
		template <typename S, typename T>
		inline static auto serialize_fn(S& s, T& obj);

		template <typename T>
		inline static auto to_string(T& t, psl::format::container& container, psl::format::data& parent)
			-> decltype(t.to_string(container, parent))
		{
			t.to_string(container, parent);
		}


		// defined local copy in case the target uses a private serialization name
#ifdef false
		template <typename T, typename = void>
		struct has_name : std::false_type
		{};

		template <typename T>
		struct has_name<T, std::void_t<decltype(&(std::declval<T>().serialization_name))>> : std::true_type
		{};


		template <typename T, typename = void>
		struct has_poly_name : std::false_type
		{};

		template <typename T>
		struct has_poly_name<T, std::void_t<decltype(&(std::declval<T>().polymorphic_name))>> : std::true_type
		{};
#else
		// todo CLang fails this expansion
		template <typename T, typename = void>
		struct has_name : std::true_type
		{};


		template <typename T, typename = void>
		struct has_poly_name : std::true_type
		{};

#endif

		template <typename T, typename = void>
		struct is_polymorphic : std::false_type
		{};
		template <typename T>
		struct is_polymorphic<T, std::void_t<decltype(std::declval<T>().polymorphic_id())>> : std::true_type
		{};


		template <typename S, typename T>
		inline static auto serialize(S& s, T& obj) -> decltype(obj.serialize(s));

		template <typename S, typename T>
		inline static auto serialize_directly(S& s, T& obj) -> decltype(obj.serialize(s))
		{
			obj.serialize(s);
		}

		template <typename T>
		inline static constexpr const char* name()
		{
			static_assert(has_name<T>::value,
						  "\n\tPlease make sure your class fullfills any of the following requirements:\n"
						  "\t\t - has a public variable \"static constexpr const char* serialization_name\"\n"
						  "\t\t - or a private variable \"static constexpr const char* serialization_name\" and added "
						  "\"friend class psl::serialization::accessor\"\n");
			return T::serialization_name;
		}

		template <typename T>
		inline static constexpr uint64_t id()
		{
			static_assert(has_poly_name<T>::value,
						  "\n\tPlease make sure your class fullfills any of the following requirements:\n"
						  "\t\t - has a public variable \"static constexpr const char* polymorphic_name\"\n"
						  "\t\t - or a private variable \"static constexpr const char* polymorphic_name\" and added "
						  "\"friend class psl::serialization::accessor\"\n");

			return utility::crc64(T::polymorphic_name);
		}

		template <typename T>
		inline static uint64_t polymorphic_id(T& obj)
		{
			static_assert(is_polymorphic<T>::value,
						  "\nYou are missing, or have incorrectly defined the following requirements:"
						  "\n\t- virtual const psl::serialization::polymorphic_base& polymorphic_id() { return "
						  "polymorphic_container; }"
						  "\n\t- static const psl::serialization::polymorphic<YOUR TYPE HERE> polymorphic_container;");
			return obj.polymorphic_id();
		}

		template <typename T>
		inline static uint64_t polymorphic_id(T* obj)
		{
			static_assert(is_polymorphic<T>::value,
						  "\nYou are missing, or have incorrectly defined the following requirements:"
						  "\n\t- virtual const psl::serialization::polymorphic_base& polymorphic_id() { return "
						  "polymorphic_container; }"
						  "\n\t- static const psl::serialization::polymorphic<YOUR TYPE HERE> polymorphic_container;");
			return obj->polymorphic_id();
		}


		template <typename T>
		constexpr static bool supports_polymorphism()
		{
			return is_polymorphic<T>::value;
		}


		static std::unordered_map<uint64_t, polymorphic_data*>& polymorphic_data();

	  private:
	};

	// helpers to figure out various details about properties, and classes you wish to serialize
	namespace details
	{
		template <typename T>
		struct is_codec
		{
			static constexpr bool value{std::is_base_of<codec, T>::value};
		};

		template <typename T>
		struct is_encoder
		{
			static constexpr bool value{std::is_base_of<encoder, T>::value};
		};

		template <typename T>
		struct is_decoder
		{
			static constexpr bool value{std::is_base_of<decoder, T>::value};
		};


		template <typename T>
		struct is_range
		{
			static constexpr bool value{utility::templates::is_trivial_container<T>::value ||
										utility::templates::is_complex_container<T>::value};
		};
		template <typename T>
		struct is_keyed_range
		{
			static constexpr bool value{utility::templates::is_associative_container<T>::value};
		};

		template <size_t index, size_t length>
		constexpr char get_nth_character(char const (&str)[length]) noexcept
		{
			if constexpr(index >= length)
			{
				static_assert(utility::templates::always_false_v<decltype(length)>, "Incorrect length for string");
				return '0';
			}
			else
			{
				return str[index];
			}
		}

		// Helper for compile time str, throws an error when an invalid construct is detected
		template <size_t expected, size_t length>
		constexpr char check_str(char const (&str)[length]) noexcept
		{
			if constexpr(expected != length - 1)
			{
				static_assert(utility::templates::always_false_v<decltype(length)>,
							  "length and expected do not match, please check the values.");
			}
			if constexpr(0 == length)
			{
				static_assert(utility::templates::always_false_v<decltype(length)>,
							  "The name can not be empty, please add atleast one character.");
			}
			return str[0];
		}


		// These helpers check if the "member function serialize" exists, and is in the correct form.
		template <typename S, typename T, typename SFINEA = void>
		struct member_function_serialize : std::false_type
		{};

		template <typename S, typename T>
		struct member_function_serialize<
			S, T, std::void_t<decltype(::psl::serialization::accessor::serialize(std::declval<S&>(), std::declval<T&>()))>>
			: std::true_type
		{};


		// These helpers check if the "function serialize" exists, and is in the correct form.
		template <typename S, typename T, typename SFINEA = void>
		struct function_serialize : std::false_type
		{};

		template <typename S, typename T>
		struct function_serialize<
			S, T, std::void_t<decltype(psl::serialization::serialize(std::declval<S&>(), std::declval<T&>()))>>
			: std::true_type
		{};

		// the next helpers check if the type is a property, and if so, what type of property.
		template <typename>
		struct is_property : std::false_type
		{};
		template <typename A, char... C>
		struct is_property<psl::serialization::property<A, C...>> : std::true_type
		{};

		template <typename T, typename Encoder = encode_to_format>
		struct is_collection
		{
			static constexpr bool value{function_serialize<Encoder, T>::value ||
										member_function_serialize<Encoder, T>::value};
		};

		template <typename T, typename Encoder>
		struct is_collection<T*, Encoder>
		{
			static constexpr bool value{function_serialize<Encoder, T>::value ||
										member_function_serialize<Encoder, T>::value};
		};
		template <typename T, typename Encoder = encode_to_format>
		struct is_collection_range
		{
			static constexpr bool value{
				is_range<T>::value &&
				(function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value ||
				 member_function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value)};
		};
		template <typename T, typename Encoder>
		struct is_collection_range<T*, Encoder>
		{
			static constexpr bool value{
				is_range<T>::value &&
				(function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value ||
				 member_function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value)};
		};

		template <typename T>
		struct is_pointer : std::false_type
		{};
		template <typename T, char... Char>
		struct is_pointer<psl::serialization::property<ptr<T>, Char...>> : std::true_type
		{};

		template <typename T>
		struct property
		{
			using property_type = property<T>;

		  public:
			// constructors
			template <typename = typename std::enable_if<std::is_default_constructible<T>::value>::type>
			property() : value{} {};

			template <typename... InitT>
			property(InitT&&... val) : value(std::forward<InitT>(val)...)
			{
				static_assert(sizeof(T) == sizeof(property_type), "size of T and property T should match");
			};

			// if we detect that the type is a range, then there's a good chance they expose a range operator
			// todo CLang doesn't like this at all
			/*template<typename = typename std::enable_if<details::is_range<T>::value>::type>
			property(std::initializer_list<typename utility::binary::get_contained_type<T>::type>&& val) : value(val)
			{
				static_assert(sizeof(T) == sizeof(property_type), "size of T and property T should match");
			};*/

			template <typename Tv = T>
			property(typename std::enable_if<
					 details::is_range<Tv>::value,
					 std::initializer_list<typename utility::binary::get_contained_type<T>::type>&&>::type val)
				: value(val)
			{
				static_assert(sizeof(T) == sizeof(property_type), "size of T and property T should match");
			};

#ifdef _MSC_VER
#pragma region
#endif
			template <typename = typename std::enable_if<std::is_move_constructible<T>::value>::type>
			property(property_type&& other) : value(std::forward<T>(other.value))
			{}
			template <typename = typename std::enable_if<std::is_copy_constructible<T>::value>::type>
			property(const property_type& other) : value(other.value)
			{}
			template <typename = typename std::enable_if<std::is_move_assignable<T>::value>::type>
			property& operator=(property_type&& other)
			{
				if(this != &other)
				{
					value = std::move(other.value);
				}
				return *this;
			}
			template <typename = typename std::enable_if<std::is_copy_assignable<T>::value>::type>
			property& operator=(const property_type& other)
			{
				if(this != &other)
				{
					value = other.value;
				}
				return *this;
			}

			// operators
			// CLang doesn't accept this
			/*	template<typename Param, typename =
			   std::enable_if<utility::templates::operators::has_subscript_operator<T, Param>::value>> auto
			   operator[](Param&& p) -> decltype(std::declval<T>().operator[](std::declval<Param>()))
				{
					return value[p];
				}*/


			// relational operators
			template <typename X, typename = std::enable_if<utility::templates::operators::has_equality<T, X>::value>>
			bool operator==(const property<X>& other) const noexcept
			{
				return operator==(other.value);
			}
			template <typename X, typename = std::enable_if<utility::templates::operators::has_equality<T, X>::value>>
			bool operator==(const X& other) const noexcept
			{
				return value == other;
			}

			template <typename X, typename = std::enable_if<utility::templates::operators::has_inequality<T, X>::value>>
			bool operator!=(const property<X>& other) const noexcept
			{
				return operator!=(other.value);
			}
			template <typename X, typename = std::enable_if<utility::templates::operators::has_inequality<T, X>::value>>
			bool operator!=(const X& other) const noexcept
			{
				return value != other;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_greater_than<T, X>::value>>
			bool operator>(const property<X>& other) const noexcept
			{
				return operator>(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_greater_than<T, X>::value>>
			bool operator>(const X& other) const noexcept
			{
				return value > other;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_greater_equal<T, X>::value>>
			bool operator>=(const property<X>& other) const noexcept
			{
				return operator>=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_greater_equal<T, X>::value>>
			bool operator>=(const X& other) const noexcept
			{
				return value >= other;
			}

			template <typename X, typename = std::enable_if<utility::templates::operators::has_less_than<T, X>::value>>
			bool operator<(const property<X>& other) const noexcept
			{
				return operator<(other.value);
			}
			template <typename X, typename = std::enable_if<utility::templates::operators::has_less_than<T, X>::value>>
			bool operator<(const X& other) const noexcept
			{
				return value < other;
			}

			template <typename X, typename = std::enable_if<utility::templates::operators::has_less_equal<T, X>::value>>
			bool operator<=(const property<X>& other) const noexcept
			{
				return operator<=(other.value);
			}
			template <typename X, typename = std::enable_if<utility::templates::operators::has_less_equal<T, X>::value>>
			bool operator<=(const X& other) const noexcept
			{
				return value <= other;
			}

			// arithmetic operators
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_plus<T, X>::value>>
			T operator+(const property<X>& other) const
			{
				return value + other.value;
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_plus<T, X>::value>>
			T operator+(const X& other) const noexcept
			{
				return value + other;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_minus<T, X>::value>>
			T operator-(const property<X>& other) const
			{
				return value - other.value;
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_minus<T, X>::value>>
			T operator-(const X& other) const
			{
				return value - other;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_modulus<T, X>::value>>
			T operator%(const property<X>& other) const
			{
				return value % other.value;
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_modulus<T, X>::value>>
			T operator%(const X& other) const
			{
				return value % other;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_multiply<T, X>::value>>
			T operator*(const property<X>& other) const
			{
				return value * other.value;
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_multiply<T, X>::value>>
			T operator*(const X& other) const
			{
				return value * other;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_divide<T, X>::value>>
			T operator/(const property<X>& other) const
			{
				return value / other.value;
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_arithmetic_divide<T, X>::value>>
			T operator/(const X& other) const
			{
				return value / other;
			}

			// compound assignment operators
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_plus<T, X>::value>>
			property_type& operator+=(const property<X>& other)
			{
				return operator+=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_plus<T, X>::value>>
			property_type& operator+=(const X& other)
			{
				value += other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_minus<T, X>::value>>
			property_type& operator-=(const property<X>& other)
			{
				return operator-=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_minus<T, X>::value>>
			property_type& operator-=(const X& other)
			{
				value -= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_multiply<T, X>::value>>
			property_type& operator*=(const property<X>& other)
			{
				return operator*=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_multiply<T, X>::value>>
			property_type& operator*=(const X& other)
			{
				value *= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_divide<T, X>::value>>
			property_type& operator/=(const property<X>& other)
			{
				return operator/=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_divide<T, X>::value>>
			property_type& operator/=(const X& other)
			{
				value /= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_modulus<T, X>::value>>
			property_type& operator%=(const property<X>& other)
			{
				return operator%=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_modulus<T, X>::value>>
			property_type& operator%=(const X& other)
			{
				value %= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_and<T, X>::value>>
			property_type& operator&=(const property<X>& other)
			{
				return operator&=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_and<T, X>::value>>
			property_type& operator&=(const X& other)
			{
				value &= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_or<T, X>::value>>
			property_type& operator|=(const property<X>& other)
			{
				return operator|=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_or<T, X>::value>>
			property_type& operator|=(const X& other)
			{
				value |= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_xor<T, X>::value>>
			property_type& operator^=(const property<X>& other)
			{
				return operator^=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_xor<T, X>::value>>
			property_type& operator^=(const X& other)
			{
				value ^= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_left_shift<T, X>::value>>
			property_type& operator<<=(const property<X>& other)
			{
				return operator<<=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_left_shift<T, X>::value>>
			property_type& operator<<=(const X& other)
			{
				value <<= other;
				return *this;
			}

			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_right_shift<T, X>::value>>
			property_type& operator>>=(const property<X>& other)
			{
				return operator>>=(other.value);
			}
			template <typename X,
					  typename = std::enable_if<utility::templates::operators::has_assignment_right_shift<T, X>::value>>
			property_type& operator>>=(const X& other)
			{
				value >>= other;
				return *this;
			}


#ifdef _MSC_VER
#pragma endregion all operators
#endif

			// general functionality
			operator T&() { return value; }

			T* operator->() { return &value; }

			/// \brief enclosed value for this property.
			T value;
		};

	} // namespace details


	template <typename T, typename CODEC>
	vtable<CODEC> const vtable_for = {[](void* this_, CODEC& s) {
		accessor::serialize_directly<CODEC, T>(s, *static_cast<T*>(this_));
		// static_assert(false, "use of undefined codec");
	}};

	/// \brief contains the polymorphic lambda constructor for your polymorphic type
	///
	/// This will be used by the deserializer to create the correct type for the format node.
	/// All polymorphic types are internally mapped to an ID (generated from a name you give).
	/// These values are saved in the format, and on deserialization are used to figure out how
	/// to construct the object in the format.
	/// Aside from the fact you need to register your polymorphic types (once), there should be no further
	/// interaction with this class.
	template <typename T>
	class polymorphic final : public polymorphic_base
	{
	  public:
		polymorphic()
		{
			if(accessor::polymorphic_data().find(ID) != accessor::polymorphic_data().end())
			{
				LOG_FATAL("Encountered duplicate Polymorphic ID in the serialization: ", ID);
				exit(-1);
			}


			static polymorphic_data data;

			data.id = ID;
			auto lambda{[]() { return new T(); }};
			data.factory = new invocable_wrapper<decltype(lambda)>(std::forward<decltype(lambda)>(lambda));

			data.encoder					 = &vtable_for<T, psl::serialization::encode_to_format>;
			data.decoder					 = &vtable_for<T, psl::serialization::decode_from_format>;
			accessor::polymorphic_data()[ID] = &data;
		}

		virtual ~polymorphic(){};
		uint64_t PolymorphicID() const override { return accessor::id<T>(); };
		static constexpr uint64_t ID{accessor::id<T>()};
	};

	template <typename Base, typename... Rest>
	static const void notify_base(psl::string8_t name, uint64_t ID)
	{
		accessor::polymorphic_data()[accessor::id<Base>()]->derived.emplace_back(name, ID);
		if constexpr(sizeof...(Rest) > 0)
		{
			notify_base<Rest...>(name, ID);
		}
	}

	template <typename T, typename... Base>
	static const uint64_t register_polymorphic()
	{
		static psl::serialization::polymorphic<T> polymorphic_container;
		if constexpr(sizeof...(Base) > 0)
		{
			notify_base<Base...>(accessor::name<T>(), accessor::id<T>());
		}
		return accessor::id<T>();
	}


	/// \brief wrapper class to signify a data member can be serialized and deserialized.
	///
	/// The property<T> wrapper signifies that a data member is elligeble to be de/serialized.
	/// The second template part (aside from the contained type T), is the name that exists in the
	/// binary.
	/// Properties have no runtime overhead (they only cost during de/serialization stages), and they have
	/// the size of the enclosed type.
	/// \todo write example.
	template <typename T, char... Char>
	struct property final : public details::property<T>
	{
		using property_type = property<T, Char...>;
		using property_base = details::property<T>;

	  public:
		using property_base::property_base;

		using property_base::operator==;
		using property_base::operator!=;
		using property_base::operator>=;
		using property_base::operator<=;
		using property_base::operator>;
		using property_base::operator<;

		using property_base::operator+;
		using property_base::operator-;
		using property_base::operator/;
		using property_base::operator*;
		using property_base::operator%;

		using property_base::operator+=;
		using property_base::operator-=;
		using property_base::operator/=;
		using property_base::operator*=;
		using property_base::operator%=;
		using property_base::operator<<=;
		using property_base::operator>>=;
		using property_base::operator|=;
		using property_base::operator^=;
		using property_base::operator&=;

		/*! @fn psl::serialization::property<T, char...>::value @copydoc psl::serialization::details::property<T>::value
		*
		 */

#ifdef _MSC_VER
#pragma region
#endif

		// relational operators
		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_equality<T, X>::value>>
		bool operator==(const property<X, Name...>& other) const noexcept
		{
			return operator==(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_inequality<T, X>::value>>
		bool operator!=(const property<X, Name...>& other) const noexcept
		{
			return operator!=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_greater_than<T, X>::value>>
		bool operator>(const property<X, Name...>& other) const noexcept
		{
			return operator>(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_greater_equal<T, X>::value>>
		bool operator>=(const property<X, Name...>& other) const noexcept
		{
			return operator>=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_less_than<T, X>::value>>
		bool operator<(const property<X, Name...>& other) const noexcept
		{
			return operator<(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_less_equal<T, X>::value>>
		bool operator<=(const property<X, Name...>& other) const noexcept
		{
			return operator<=(other.value);
		}

		// arithmetic operators
		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_arithmetic_plus<T, X>::value>>
		T operator+(const property<X, Name...>& other) const
		{
			return this->value + other.value;
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_arithmetic_minus<T, X>::value>>
		T operator-(const property<X, Name...>& other) const
		{
			return this->value - other.value;
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_arithmetic_modulus<T, X>::value>>
		T operator%(const property<X, Name...>& other) const
		{
			return this->value % other.value;
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_arithmetic_multiply<T, X>::value>>
		T operator*(const property<X, Name...>& other) const
		{
			return this->value * other.value;
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_arithmetic_divide<T, X>::value>>
		T operator/(const property<X, Name...>& other) const
		{
			return this->value / other.value;
		}

		// compound assignment operators
		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_plus<T, X>::value>>
		property_type& operator+=(const property<X, Name...>& other)
		{
			return operator+=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_minus<T, X>::value>>
		property_type& operator-=(const property<X, Name...>& other)
		{
			return operator-=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_multiply<T, X>::value>>
		property_type& operator*=(const property<X, Name...>& other)
		{
			return operator*=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_divide<T, X>::value>>
		property_type& operator/=(const property<X, Name...>& other)
		{
			return operator/=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_modulus<T, X>::value>>
		property_type& operator%=(const property<X, Name...>& other)
		{
			return operator%=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_and<T, X>::value>>
		property_type& operator&=(const property<X, Name...>& other)
		{
			return operator&=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_or<T, X>::value>>
		property_type& operator|=(const property<X, Name...>& other)
		{
			return operator|=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_xor<T, X>::value>>
		property_type& operator^=(const property<X, Name...>& other)
		{
			return operator^=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_left_shift<T, X>::value>>
		property_type& operator<<=(const property<X, Name...>& other)
		{
			return operator<<=(other.value);
		}

		template <typename X, char... Name,
				  typename = std::enable_if<utility::templates::operators::has_assignment_right_shift<T, X>::value>>
		property_type& operator>>=(const property<X, Name...>& other)
		{
			return operator>>=(other.value);
		}


#ifdef _MSC_VER
#pragma endregion all operators
#endif
		/// \returns a reference to the enclosed value.
		operator T&() { return this->value; }

		/// \returns a pointer to the enclosed value.
		T* operator->() { return &this->value; }

		/// \returns the name assigned to this variation of the property.
		static const psl::string8::view name() { return m_Name.data(); }

		static const psl::template_string<Char...> m_Name;
		/// \brief utility::crc64() id that has been generated based on the property name.
		static const uint64_t ID{utility::crc64<sizeof...(Char)>(m_Name.data())};
	};
	template <typename T, char... Char>
	const psl::template_string<Char...> property<T, Char...>::m_Name{};

	// property version that supports optionally polymorphic types
	template <typename T, char... Char>
	struct property<T*, Char...> final
	{
		using property_type = property<T*, Char...>;
		constexpr static bool supports_polymorphism() { return accessor::supports_polymorphism<T>(); }

		property() : value{nullptr} {};
		~property() { delete(value); }
		template <char... Name>
		property(const property<T*, Name...>& other) : value(new T(*other.value)){};
		template <char... Name>
		property(property<T*, Name...>&& other) : value(other.value)
		{
			other.value = nullptr;
		};
		template <char... Name>
		property& operator=(const property<T, Name...>& other)
		{
			if(this != &other)
			{
				value = new T(*other.value);
			}
			return *this;
		};
		template <char... Name>
		property& operator=(property<T, Name...>&& other)
		{
			if(this != &other)
			{
				value		= (other.value);
				other.value = nullptr;
			}
			return *this;
		};

		// takes ownership of the passed pointer
		property(T* ptr) : value(ptr){};
		template <typename X, typename... Args>
		property(Args... args) : value(new X(std::forward<Args>(args)...)){};

		operator T&() { return *value; }
		T* operator->() { return value; }

		T* value;

		// naming
		static const psl::string8::view name() { return m_Name.data(); }

		static const psl::template_string<Char...> m_Name;
		static const uint64_t ID{utility::crc64<sizeof...(Char)>(m_Name.data())};
	};

	template <typename T, char... Char>
	const psl::template_string<Char...> property<T*, Char...>::m_Name{};

	// property version that supports indirection to other properties, this only supports that and is the format
	// equivalent of reference nodes it stores a pointer to the other property's value, but does not own the value and
	// so should not clear it
	template <typename T, char... Char>
	struct property<ptr<T>, Char...> final
	{
	  private:
		using property_type = property<ptr<T>, Char...>;

	  public:
		property()  = default;
		~property() = default;
		template <char... Name>
		property(const property<ptr<T>, Name...>& other) : value(other.value){};
		template <char... Name>
		property(property<ptr<T>, Name...>&& other) : value(other.value){};
		template <char... Name>
		property& operator=(const property<ptr<T>, Name...>& other)
		{
			if(this != &other)
			{
				value = (other.value);
			}
			return *this;
		};
		template <char... Name>
		property& operator=(property<ptr<T>, Name...>&& other)
		{
			if(this != &other)
			{
				value = (other.value);
			}
			return *this;
		};

		template <typename P>
		property(P& value) : value(&value)
		{
			static_assert(
				details::is_collection<P>::value,
				"It looks like you are trying to reference to an element in a range, in order for this to work the "
				"type has to be a collection type, we cannot directly reference value range elements.");
			// static_assert(details::is_property<P>::value, "the type has to be a property");
		};

		template <typename P, char... Name>
		property(property<P, Name...>& property)
			: value(&(property.value)){

			  };

		property(const property_type& property) : value(property.value){};

		bool operator==(const property_type& other) const { return value == other.value; }

		bool operator==(const T& other) const { return value == &other; }
		bool operator==(const T*& other) const { return value == &other; }
		template <char... Name>
		bool operator==(const property<T, Name...>& other) const
		{
			return value == &other.value;
		}

		bool operator!=(const property_type& other) const { return value != other.value; }
		bool operator!=(const T& other) const { return value != &other; }
		bool operator!=(const T*& other) const { return value != &other; }
		template <char... Name>
		bool operator!=(const property<T, Name...>& other) const
		{
			return value != &other.value;
		}

		operator T*() { return value; }

		T* operator->() { return value; }

		T* value{nullptr};

		// naming
		static const psl::string8::view name() { return m_Name.data(); }

		static const psl::template_string<Char...> m_Name;
		static const uint64_t ID{utility::crc64<sizeof...(Char)>(m_Name.data())};
	};
	template <typename T, char... Char>
	const psl::template_string<Char...> property<ptr<T>, Char...>::m_Name{};

	class decode_from_format : decoder
	{
		using codec_t	 = decode_from_format;
		using container_t = psl::format::container;

	  public:
		decode_from_format(psl::format::container& container, psl::format::handle* root,
						   std::unordered_map<uint64_t, invocable_wrapper_base*> factory = {})
			: m_Container(container), m_CollectionStack{{root}}, m_Factory(factory){};

		template <typename T, char... Char>
		decode_from_format& operator<<(property<T, Char...>& property)
		{
			parse(property);
			return *this;
		}

		template <typename... Args>
		void parse(Args&&... args)
		{
			(parse_internal(args), ...);
		}

		// catch-all for normal types
		template <typename T, char... Char>
		void parse(property<T, Char...>& property)
		{
			parse_internal(property.value, property.name());
		}


		// catch-all for normal, and polymorphic types
		template <typename T, char... Char>
		void parse(property<T*, Char...>& property)
		{
			if constexpr(details::is_collection<T>::value)
			{
				property.value = create_polymorphic_collection<T>();
				parse_collection(*property.value, property.name);
			}
			else
			{
				property.value = new T();
				parse_internal(*property.value, property.name());
			}
		}

		// catches all reference types
		template <typename T, char... Char>
		void parse(property<ptr<T>, Char...>& property)
		{
			if constexpr(details::is_range<T>::value)
			{
				static_assert(utility::templates::always_false_v<T>, "not implemented yet");
			}
			else
			{
				auto& node = (m_CollectionStack.size() > 0)
								 ? m_Container.find(m_CollectionStack.top()->get(), property.name())
								 : m_Container.find(property.name());
				if(!node.exists()) return;
				m_PointerNodes[(std::uintptr_t)&property.value] = &node;
				m_ReferenceMap[&node]							= (std::uintptr_t)&property.value;
			}
		}

		void resolve_references()
		{
			for(auto ref : m_ReferenceNodes)
			{
				auto value_opt{ref.second->get().as_reference()};
				if(!value_opt) continue;

				if(auto it = m_ReferenceMap.find(value_opt.value()); it != m_ReferenceMap.end())
				{
					std::memcpy((void*)ref.first, &it->second, sizeof(void*));
				}
			}
			for(auto ptr : m_PointerNodes)
			{
				auto value_opt{ptr.second->get().as_reference()};
				if(!value_opt) continue;

				if(auto it = m_ReferenceMap.find(value_opt.value()); it != m_ReferenceMap.end())
				{
					std::memcpy((void*)ptr.first, &it->second, sizeof(void*));
				}
			}
		}

	  private:
		template <typename T>
		void parse_collection(T& property, std::optional<psl::string8::view> override_name = {})
		{
			if(m_CollectionStack.size() > 0)
			{
				parse_collection(property,
								 m_Container.index_of(m_CollectionStack.top()->get(),
													  (override_name) ? override_name.value() : accessor::name<T>()));
			}
			else
			{
				parse_collection(property,
								 m_Container.index_of((override_name) ? override_name.value() : accessor::name<T>()));
			}
		}

		template <typename T>
		void parse_collection(T& property, psl::format::nodes_t index)
		{
			if(index == std::numeric_limits<psl::format::nodes_t>::max()) return;

			auto& collection = m_Container[index];
			m_CollectionStack.push(&collection);

			m_ReferenceMap[&collection] = (std::uintptr_t)&property;

			if constexpr(details::member_function_serialize<codec_t, T>::value &&
						 !details::function_serialize<codec_t, T>::value)
			{
				accessor::serialize(*this, property);
			}
			else if constexpr(!details::member_function_serialize<codec_t, T>::value &&
							  details::function_serialize<codec_t, T>::value)
			{
				accessor::serialize_fn(*this, property);
			}
			else
			{
				static_assert(utility::templates::always_false_v<T>,
							  "\n\tPlease define one of the following for the serializer:\n"
							  "\t\t- a member function of the type template<typename S> void serialize(S& s) {};\n"
							  "\t\t- a function in the namespace 'serialization' of the signature: template<typename "
							  "S> void serialize(S& s, T& target) {};\n"
							  "\t\t- if you did declare a member function, but wanted the serialize to be private, "
							  "please add 'friend class psl::serialization::serializer' to your class");
			}
			m_CollectionStack.pop();
		}

		template <typename T>
		T* create_polymorphic_collection(std::optional<psl::string8::view> override_name = {})
		{
			if(m_CollectionStack.size() > 0)
			{
				auto& collection = m_Container.find(m_CollectionStack.top()->get(),
													(override_name) ? override_name.value() : accessor::name<T>());
				m_CollectionStack.push(&collection);
			}
			else
			{
				auto& collection = m_Container.find((override_name) ? override_name.value() : accessor::name<T>());
				m_CollectionStack.push(&collection);
			}

			auto& polymorphic_id = m_Container.find(m_CollectionStack.top()->get(), "POLYMORPHIC_ID");
			T* target			 = nullptr;
			if(polymorphic_id.exists())
			{
				auto value_opt{polymorphic_id.get().as_value_content()};

				uint64_t id = stoull(psl::string8_t(value_opt.value().second));
				if(auto it = m_Factory.find(id); it != m_Factory.end())
				{
					target = (T*)((*it->second)());
				}
				else if(auto it = accessor::polymorphic_data().find(id); it != accessor::polymorphic_data().end())
				{
					target = (T*)((*it->second->factory)());
				}
				else
				{
					target = new T();
				}
			}
			else
			{
				target = new T();
			}

			m_CollectionStack.pop();
			return target;
		}

		template <typename T>
		void parse_internal(T& value, psl::string8::view name)
		{
			constexpr bool is_range		 = details::is_range<T>::value;
			constexpr bool is_collection = details::is_collection<typename std::conditional<
				is_range, typename std::remove_pointer<typename utility::binary::get_contained_type<T>::type>::type,
				T>::type>::value;

			if constexpr(is_collection && !is_range)
			{
				parse_collection(value, name);
			}
			else if constexpr(is_collection && is_range)
			{
				using contained_t = typename utility::binary::get_contained_type<T>::type;

				size_t size = 0;
				if(m_CollectionStack.size() > 0)
				{
					auto& collection = m_Container.find(m_CollectionStack.top()->get(), name);
					m_CollectionStack.push(&collection);
					auto value_opt{collection.get().as_collection()};
					size = value_opt.value_or(0);
				}
				else
				{
					auto& collection = m_Container.find(name);
					m_CollectionStack.push(&collection);
					auto value_opt{collection.get().as_collection()};
					size = value_opt.value_or(0);
				}

				m_ReferenceMap[m_CollectionStack.top()] = (std::uintptr_t)&value;
				value.clear();
				value.reserve(size);
				size_t begin		= m_Container.index_of(m_CollectionStack.top()->get()) + 1u;
				size_t end			= begin + size;
				size_t actual_index = 0;
				for(auto i = begin; i < end; ++i)
				{
					if((size_t)(m_Container[psl::format::nodes_t(i)].get().depth()) - 1u ==
					   (size_t)(m_CollectionStack.top()->get().depth()))
					{
						if constexpr(std::is_pointer<contained_t>::value)
						{
							using deref_t = typename std::remove_pointer<contained_t>::type;
							value.emplace_back(
								create_polymorphic_collection<deref_t>(utility::to_string(actual_index)));
							parse_collection(*value[actual_index], (psl::format::nodes_t)(i));
						}
						else
						{
							value.emplace_back();
							parse_collection(value[actual_index], (psl::format::nodes_t)(i));
						}
						++actual_index;
					}
				}
				m_CollectionStack.pop();
			}
			else if constexpr(details::is_keyed_range<T>::value)
			{
				size_t size = 0;
				if(m_CollectionStack.size() > 0)
				{
					auto& collection = m_Container.find(m_CollectionStack.top()->get(), name);
					m_CollectionStack.push(&collection);

					auto value_opt{collection.get().as_collection()};
					size = value_opt.value_or(0);
				}
				else
				{
					auto& collection = m_Container.find(name);
					m_CollectionStack.push(&collection);

					auto value_opt{collection.get().as_collection()};
					size = value_opt.value_or(0);
				}

				size_t begin		= m_Container.index_of(m_CollectionStack.top()->get()) + 1u;
				size_t end			= begin + size;
				//size_t actual_index = 0;

				static_assert(details::is_keyed_range<T>::value, "never seen");

				for(auto i = begin; i < end; ++i)
				{
					if((size_t)(m_Container[psl::format::nodes_t(i)].get().depth()) - 1u ==
					   (size_t)(m_CollectionStack.top()->get().depth()))
					{
						// auto& pair = value.emplace(, typename T::value_type{});
						auto& sub_val = value[utility::from_string<typename utility::templates::get_key_type<T>::type>(
							m_Container[psl::format::nodes_t(i)].get().name())];
						parse_collection(sub_val, (psl::format::nodes_t)(i));
					}
				}
				m_CollectionStack.pop();
			}
			else if constexpr(is_range)
			{
				using contained_t = typename utility::binary::get_contained_type<T>::type;

				auto& node = (m_CollectionStack.size() > 0) ? m_Container.find(m_CollectionStack.top()->get(), name)
															: m_Container.find(name);
				if(!node.exists()) return;
				value.clear();

				auto value_opt{node.get().as_value_range_content()};
				if(value_opt)
				{

					auto data_content{std::move(value_opt.value())};
					value.reserve(data_content.size());

					for(const auto& it : data_content)
					{
						if constexpr(std::is_pointer<contained_t>::value)
						{
							using deref_t = typename std::remove_pointer<contained_t>::type;
							value.emplace_back(new deref_t(
								utility::from_string<typename std::remove_pointer<contained_t>::type>(it.second)));
						}
						else
						{
							value.emplace_back(
								utility::from_string<typename utility::binary::get_contained_type<T>::type>(it.second));
						}
					}
				}
				m_ReferenceMap[&node] = (std::uintptr_t)&value;
			}
			else
			{
				auto& node = (m_CollectionStack.size() > 0) ? m_Container.find(m_CollectionStack.top()->get(), name)
															: m_Container.find(name);
				if(!node.exists()) return;

				if(auto value_opt = node.get().as_value_content(); value_opt)
				{
					value = utility::from_string<T>(value_opt.value().second);
				}
				m_ReferenceMap[&node] = (std::uintptr_t)&value;
			}
		}

		psl::format::container& m_Container;
		std::stack<psl::format::handle*> m_CollectionStack;

		// reference resolving
		std::unordered_map<std::uintptr_t, psl::format::handle*> m_ReferenceNodes;
		std::unordered_map<std::uintptr_t, psl::format::handle*> m_PointerNodes;
		std::unordered_map<psl::format::handle*, std::uintptr_t> m_ReferenceMap;

		// polymorphic override
		std::unordered_map<uint64_t, invocable_wrapper_base*> m_Factory;
	};

	class encode_to_format : encoder
	{
		using codec_t	 = encode_to_format;
		using container_t = psl::format::container;

	  public:
		encode_to_format(psl::format::container& container, psl::format::handle* root)
			: m_Container(container), m_CollectionStack{{root}} {};

		template <typename T, char... Char>
		encode_to_format& operator<<(property<T, Char...>& property)
		{
			parse(property);
			return *this;
		}

		template <typename... Props>
		void parse(Props&&... props)
		{
			(parse(props), ...);
		}

		template <typename T, char... Char>
		void parse(property<T, Char...>& property)
		{
			parse(property.value, property.name());
		}

		template <typename T, char... Char>
		void parse(property<T*, Char...>& property)
		{
			parse(*property.value, property.name());
		}

		template <typename T, char... Char>
		void parse(property<ptr<T>, Char...>& property)
		{
			if constexpr(details::is_range<T>::value)
			{
				static_assert(utility::templates::always_false_v<T>, "not implemented yet");
			}
			else
			{
				if(property.value == nullptr) return;

				m_Container.add_reference(m_CollectionStack.top()->get(), property.name(), m_Container[0].get());
				m_ReferenceMap[(std::uintptr_t)&property.value] =
					&m_Container[(psl::format::nodes_t)(m_Container.size() - 1u)];
				m_ToBeResolvedReferenceMap[(std::uintptr_t)(property.value)] =
					&m_Container[(psl::format::nodes_t)(m_Container.size() - 1u)];
			}
		}

		void resolve_references()
		{
			for(auto ref : m_ToBeResolvedReferenceMap)
			{
				if(auto target = m_ReferenceMap.find(ref.first); target != m_ReferenceMap.end())
				{
					m_Container.set_reference(ref.second->get(), target->second->get());
				}
			}
		}

	  private:
		template <typename T>
		void parse_collection(T& property, std::optional<psl::string8::view> override_name = {})
		{
			auto& collection = m_Container.add_collection(
				m_CollectionStack.top()->get(), (override_name) ? override_name.value() : accessor::name<T>());
			m_CollectionStack.push(&collection);
			m_ReferenceMap[(std::uintptr_t)&property] = &collection;
			if constexpr(details::member_function_serialize<codec_t, T>::value)
			{
				accessor::serialize(*this, property);
			}
			else if constexpr(details::function_serialize<codec_t, T>::value)
			{
				accessor::serialize_fn(*this, property);
			}
			else
			{
				static_assert(utility::templates::always_false_v<T>,
							  "\n\tPlease define one of the following for the serializer:\n"
							  "\t\t- a member function of the type template<typename S> void serialize(S& s) {};\n"
							  "\t\t- a function in the namespace 'serialization' of the signature: template<typename "
							  "S> void serialize(S& s, T& target) {};\n"
							  "\t\t- if you did declare a member function, but wanted the serialize to be private, "
							  "please add 'friend class psl::serialization::serializer' to your class");
			}
			m_CollectionStack.pop();
		}
		template <typename T>
		void parse(T& value, psl::string8::view name)
		{
			constexpr bool is_range = details::is_range<T>::value;
			using contained_t		= typename utility::binary::get_contained_type<T>::type;

			constexpr bool is_collection = details::is_collection<
				typename std::conditional<is_range, typename std::remove_pointer<contained_t>::type, T>::type>::value;


			if constexpr(is_collection && !is_range)
			{
				parse_collection(value, name);
			}
			else if constexpr(is_collection && is_range)
			{
				auto& collection = m_Container.add_collection(m_CollectionStack.top()->get(), name);
				m_ReferenceMap[(std::uintptr_t)&value] = &collection;
				m_CollectionStack.push(&collection);
				size_t index = 0;

				if constexpr(std::is_pointer<contained_t>::value)
				{
					for(contained_t& it : value)
					{
						parse_collection(*it, utility::to_string(index++));
					}
				}
				else
				{
					for(contained_t& it : value)
					{
						parse_collection(it, utility::to_string(index++));
					}
				}
				m_CollectionStack.pop();
			}
			else if constexpr(details::is_keyed_range<T>::value)
			{
				auto& collection = m_Container.add_collection(m_CollectionStack.top()->get(), name);
				m_ReferenceMap[(std::uintptr_t)&value] = &collection;
				m_CollectionStack.push(&collection);
				for(auto& pair : value)
				{
					static_assert(details::is_collection<typename utility::templates::get_value_type<T>::type>::value,
								  "can only use associative containers when their value is a collection");
					parse_collection(
						pair.second,
						utility::to_string<typename utility::templates::get_key_type<T>::type>(pair.first));
				}
				m_CollectionStack.pop();
			}
			else if constexpr(is_range)
			{
				psl::string8_t intermediate;
				std::vector<size_t> locations;
				std::vector<psl::string8::view> res;
				res.reserve(value.size());
				if constexpr(std::is_pointer<contained_t>::value)
				{
					for(const auto& it : value)
					{
						intermediate.append(utility::to_string<typename std::remove_pointer<contained_t>::type>(*it));
						locations.push_back(intermediate.size());
					}
				}
				else
				{
					for(const auto& it : value)
					{
						intermediate.append(utility::to_string<contained_t>(it));
						locations.push_back(intermediate.size());
					}
				}

				size_t previous = 0u;
				for(const auto& loc : locations)
				{
					res.emplace_back(&intermediate[previous], loc - previous);
					previous = loc;
				}
				m_Container.add_value_range(m_CollectionStack.top()->get(), name, res);
				m_ReferenceMap[(std::uintptr_t)&value] = &m_Container[(psl::format::nodes_t)(m_Container.size() - 1u)];
			}
			else
			{
				m_Container.add_value(m_CollectionStack.top()->get(), name, utility::to_string<contained_t>(value));
				m_ReferenceMap[(std::uintptr_t)&value] = &m_Container[(psl::format::nodes_t)(m_Container.size() - 1u)];
			}
		}


		psl::format::container& m_Container;
		std::stack<psl::format::handle*> m_CollectionStack;

		std::unordered_map<std::uintptr_t, psl::format::handle*> m_ReferenceMap;
		std::unordered_map<std::uintptr_t, psl::format::handle*> m_ToBeResolvedReferenceMap;
	};
	template <typename T>
	vtable<encode_to_format> const vtable_for<T, encode_to_format> = {[](void* this_, encode_to_format& s) {
		T* t = static_cast<T*>(this_);
		psl::serialization::property<psl::string8_t, const_str("POLYMORPHIC_ID", 14)> p{
			utility::to_string(accessor::polymorphic_id(t))};
		s.parse(p);
		accessor::serialize_directly<encode_to_format, T>(s, *t);
		// t->serialize(s);
	}};
	class serializer
	{
		template <typename T, typename Encoder>
		inline static void check()
		{
			static_assert(accessor::has_name<T>::value,
						  "\n\tPlease make sure your class fullfills any of the following requirements:\n"
						  "\t\t - has a public variable \"static constexpr const char* serialization_name\"\n"
						  "\t\t - or a private variable \"static constexpr const char* serialization_name\" and added "
						  "\"friend class psl::serialization::accessor\"\n");
			static_assert(is_member_fn<T, Encoder>() || is_fn<T, Encoder>(),
						  "\n\tPlease define one of the following for the serializer:\n"
						  "\t\t- a member function of the type template<typename S> void serialize(S& s) {};\n"
						  "\t\t- a function in the namespace 'serialization' of the signature: template<typename S> "
						  "void serialize(S& s, T& target) {};\n"
						  "\t\t- if you did declare a member function, but wanted the serialize to be private, please "
						  "add 'friend class psl::serialization::serializer' to your class");
		}

		template <typename T, typename Encoder>
		static constexpr bool is_member_fn()
		{
			return details::member_function_serialize<Encoder, T>::value &&
				   !details::function_serialize<Encoder, T>::value;
		}

		template <typename T, typename Encoder>
		static constexpr bool is_fn()
		{
			return !details::member_function_serialize<Encoder, T>::value &&
				   details::function_serialize<Encoder, T>::value;
		}

	  public:
		~serializer()
		{
			for(auto& it : m_Factory)
			{
				delete(it.second);
			}
		}

		template <typename Encoder, typename T>
		void serialize(T* target, psl::format::container& out, std::optional<psl::string8::view> name = {})
		{
			if(target == nullptr) return;
			serialize<Encoder, T>(*target, out, name);
		}

		template <typename Encoder, typename T>
		void serialize(T& target, psl::string8::view filename, std::optional<psl::string8::view> name = {})
		{
			psl::format::container out;
			serialize<Encoder, T>(target, out, name);
			utility::platform::file::write(psl::from_string8_t(filename), psl::from_string8_t(out.to_string()));
		};

		template <typename Encoder, typename T>
		void serialize(T* target, psl::string8::view filename, std::optional<psl::string8::view> name = {})
		{
			if(target == nullptr) return;
			psl::format::container out;
			serialize<Encoder, T>(*target, out, name);
			utility::platform::file::write(psl::from_string8_t(filename), psl::from_string8_t(out.to_string()));
		};

		template <typename Encoder, typename T>
		void serialize(T& target, psl::format::container& out, std::optional<psl::string8::view> name = {})
		{
			check<T, Encoder>();
			auto& collection = out.add_collection((name) ? name.value() : accessor::name<T>());
			Encoder encoder(out, &collection);

			if constexpr(details::member_function_serialize<Encoder, T>::value &&
						 !details::function_serialize<Encoder, T>::value)
			{
				accessor::serialize(encoder, target);
			}
			else
			{
				accessor::serialize_fn(encoder, target);
			}
			encoder.resolve_references();
		};

		template <typename Encoder, typename T>
		bool deserialize(T*& target, psl::format::container& out, std::optional<psl::string8::view> name = {})
		{
			if(target == nullptr)
			{
				auto& collection = out.find((name) ? name.value() : accessor::name<T>());
				if(!collection.exists()) return false;
				auto& polymorphic_id = out.find(collection.get(), "POLYMORPHIC_ID");
				if(polymorphic_id.exists())
				{
					auto value_opt{polymorphic_id.get().as_value_content()};

					uint64_t id = stoull(psl::string8_t(value_opt.value().second));
					if(auto it = m_Factory.find(id); it != m_Factory.end())
					{
						target = (T*)((*it->second)());
					}
					else if(auto poly_it = accessor::polymorphic_data().find(id); poly_it != accessor::polymorphic_data().end())
					{
						target = (T*)((*poly_it->second->factory)());
					}
					else
					{
						target = new T();
					}
				}
				else
				{
					target = new T();
				}
			}
			return deserialize<Encoder, T>(*target, out, name);
		}

		template <typename Encoder, typename T>
		bool deserialize(T*& target, psl::string8::view filename, std::optional<psl::string8::view> name = {})
		{
			auto res = utility::platform::file::read(psl::from_string8_t(filename));
			if(!res) return false;
			psl::format::container cont{psl::to_string8_t(res.value())};
			return deserialize<Encoder, T>(target, cont, name);
		}

		template <typename Encoder, typename T>
		bool deserialize(T& target, psl::string8::view filename, std::optional<psl::string8::view> name = {})
		{
			auto res = utility::platform::file::read(psl::from_string8_t(filename));
			if(!res) return false;
			psl::format::container cont{psl::to_string8_t(res.value())};
			return deserialize<Encoder, T>(target, cont, name);
		}

		template <typename Encoder, typename T>
		bool deserialize(T& target, psl::format::container& out, std::optional<psl::string8::view> name = {})
		{
			check<T, Encoder>();

			auto& collection = out.find((name) ? name.value() : accessor::name<T>());
			if(!collection.exists()) return false;

			Encoder encoder(out, &collection, m_Factory);

			if constexpr(details::member_function_serialize<Encoder, T>::value &&
						 !details::function_serialize<Encoder, T>::value)
			{
				accessor::serialize(encoder, target);
			}
			else
			{
				accessor::serialize_fn(encoder, target);
			}
			encoder.resolve_references();
			return true;
		}

		template <typename T, typename F>
		void add_factory(F&& f)
		{
			m_Factory[accessor::id<T>()] = new invocable_wrapper<F>(std::forward<F>(f));
		}

		std::unordered_map<uint64_t, invocable_wrapper_base*> m_Factory;
	};
	template <typename S, typename T>
	inline auto accessor::serialize_fn(S& s, T& obj)
	{
		if constexpr(accessor::supports_polymorphism<T>())
		{
			static_assert(utility::templates::always_false_v<S>,
						  "You should not attempt to use functions in a polymorphic setting. Please use methods.");
		}
		else
		{
			psl::serialization::serialize(s, obj);
		}
	}
	template <typename S, typename T>
	inline auto accessor::serialize(S& s, T& obj) -> decltype(obj.serialize(s))
	{
		if constexpr(accessor::supports_polymorphism<T>())
		{
			auto id = polymorphic_id(obj);
			if constexpr(details::is_encoder<S>::value)
			{
				polymorphic_data()[id]->encoder->serialize(&obj, s);
			}
			else if constexpr(details::is_decoder<S>::value)
			{
				polymorphic_data()[id]->decoder->serialize(&obj, s);
			}
		}
		else
		{
			obj.serialize(s);
		}
	}
} // namespace serialization
