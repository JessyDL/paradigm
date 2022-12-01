#pragma once
#include <optional>
#include <stack>
#include <unordered_map>

#include "psl/binary_utils.hpp"
#include "psl/format.hpp"
#include "psl/logging.hpp"
#include "psl/platform_utils.hpp"
#include "psl/serialization/property.hpp"
#include "psl/template_utils.hpp"
#include "psl/ustring.hpp"

namespace psl::serialization
{
	namespace details
	{
		class dummy;
	}

	template <typename S>
	void serialize(S& x, details::dummy& y) {};

	// special classes to mark classes that can act as serialization codecs
	class codec
	{};
	class encoder : codec
	{};
	class decoder : codec
	{};

	class encode_to_format;
	class decode_from_format;

	struct polymorphic_base
	{
		virtual ~polymorphic_base() {};
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
#if !defined(PLATFORM_ANDROID)	  // todo android currently does not support is_invocable
			static_assert(std::is_invocable<INV>::value, "Must be an invocable");
#endif
		};
		void* operator()() override { return t(); };

		INV t;
	};

	struct polymorphic_data_t
	{
		uint64_t id;
		std::function<void(void*, encode_to_format&)> encode;
		std::function<void(void*, decode_from_format&)> decode;
		invocable_wrapper_base* factory;


		std::vector<std::pair<psl::string8_t, uint64_t>> derived;
		~polymorphic_data_t() { delete(factory); };
	};

	namespace details
	{
		extern std::unique_ptr<std::unordered_map<uint64_t, psl::serialization::polymorphic_data_t*>> m_PolymorphicData;
	}
	// friendly helper to access privates, see serializer's static_assert for usage
	class accessor
	{
	  private:
		template <typename T, typename = void>
		struct is_polymorphic
		{
			using type = std::false_type;
		};
		template <typename T>
		struct is_polymorphic<T, std::void_t<decltype(std::declval<T>().polymorphic_id())>>
		{
			using type = std::true_type;
		};

	  public:
		template <typename T>
		static constexpr auto test_has_name() noexcept -> void
		{
			T::serialization_name;
		}
		template <typename T>
		static constexpr auto test_has_polymorphic_name() noexcept -> void
		{
			T::polymorphic_name;
		}

		template <typename T>
		static constexpr auto test_is_polymorphic() noexcept -> typename is_polymorphic<T>::type
		{
			return {};
		}

		template <typename S, typename T>
		inline static auto serialize_fn(S& s, T& obj);

		template <typename T>
		inline static auto to_string(T& t, psl::format::container& container, psl::format::data& parent)
		  -> decltype(t.to_string(container, parent))
		{
			t.to_string(container, parent);
		}


		template <typename S, typename T>
		inline static auto serialize(S& s, T& obj) -> decltype(obj.serialize(s));

		template <typename S, typename T>
		inline static auto serialize_directly(S& s, T& obj) -> decltype(obj.serialize(s))
		{
			obj.serialize(s);
		}

		template <typename T>
		inline static constexpr psl::string8::view name();

		template <typename T>
		inline static consteval uint64_t id();

		template <typename T>
		inline static uint64_t polymorphic_id(T& obj)
		{
			static_assert(is_polymorphic<T>::type::value,
						  "\nYou are missing, or have incorrectly defined the following requirements:"
						  "\n\t- virtual const psl::serialization::polymorphic_base& polymorphic_id() { return "
						  "polymorphic_container; }"
						  "\n\t- static const psl::serialization::polymorphic<YOUR TYPE HERE> polymorphic_container;");

			return obj.polymorphic_id();
		}

		template <typename T>
		inline static uint64_t polymorphic_id(T* obj)
		{
			static_assert(is_polymorphic<T>::type::value,
						  "\nYou are missing, or have incorrectly defined the following requirements:"
						  "\n\t- virtual const psl::serialization::polymorphic_base& polymorphic_id() { return "
						  "polymorphic_container; }"
						  "\n\t- static const psl::serialization::polymorphic<YOUR TYPE HERE> polymorphic_container;");
			return obj->polymorphic_id();
		}


		template <typename T>
		constexpr static bool supports_polymorphism()
		{
			return is_polymorphic<T>::type::value;
		}


		static std::unordered_map<uint64_t, polymorphic_data_t*>& polymorphic_data();

	  private:
	};

	namespace details
	{
		template <typename T>
		concept HasSerializationName = requires
		{
			accessor::template test_has_name<T>();
		};

		template <typename T>
		concept HasSerializationPolymorphicName = requires
		{
			accessor::template test_has_polymorphic_name<T>();
		};

		template <typename T>
		concept IsSerializationPolymorphic = requires
		{
			{
				accessor::template test_is_polymorphic<T>()
				} -> std::same_as<std::true_type>;
		};
	}	 // namespace details

	template <typename T>
	inline constexpr psl::string8::view accessor::name()
	{
		static_assert(details::HasSerializationName<T>,
					  "\n\tPlease make sure your class fullfills any of the following requirements:\n"
					  "\t\t - has a public variable \"static constexpr const char* serialization_name\"\n"
					  "\t\t - or a private variable \"static constexpr const char* serialization_name\" and added "
					  "\"friend class psl::serialization::accessor\"\n");
		return psl::string8::view {T::serialization_name};
	}

	template <typename T>
	inline consteval uint64_t accessor::id()
	{
		static_assert(details::HasSerializationPolymorphicName<T>,
					  "\n\tPlease make sure your class fullfills any of the following requirements:\n"
					  "\t\t - has a public variable \"static constexpr const char* polymorphic_name\"\n"
					  "\t\t - or a private variable \"static constexpr const char* polymorphic_name\" and added "
					  "\"friend class psl::serialization::accessor\"\n");

		return utility::crc64(T::polymorphic_name);
	}

	namespace details
	{
		template <typename T>
		concept IsCodec = std::is_base_of_v<codec, T>;
		template <typename T>
		concept IsEncoder = std::is_base_of_v<encoder, T>;
		template <typename T>
		concept IsDecoder = std::is_base_of_v<decoder, T>;


		// These helpers check if the "member function serialize" exists, and is in the correct form.
		template <typename S, typename T, typename SFINEA = void>
		struct member_function_serialize : std::false_type
		{};

		template <typename S, typename T>
		struct member_function_serialize<
		  S,
		  T,
		  std::void_t<decltype(::psl::serialization::accessor::serialize(std::declval<S&>(), std::declval<T&>()))>> :
			std::true_type
		{};


		// These helpers check if the "function serialize" exists, and is in the correct form.
		template <typename S, typename T, typename SFINEA = void>
		struct function_serialize : std::false_type
		{};

		// using psl::serialization::serialize;

		template <typename S, typename T>
		struct function_serialize<S, T, std::void_t<decltype(serialize(std::declval<S&>(), std::declval<T&>()))>> :
			std::true_type
		{};

		// the next helpers check if the type is a property, and if so, what type of property.
		template <typename>
		struct is_property : std::false_type
		{};
		template <auto Name, typename A>
		struct is_property<psl::serialization::property<Name, A>> : std::true_type
		{};

		template <typename T, typename Encoder = encode_to_format>
		struct is_collection
		{
			static constexpr bool value {function_serialize<Encoder, T>::value ||
										 member_function_serialize<Encoder, T>::value};
		};

		template <typename T, typename Encoder>
		struct is_collection<T*, Encoder>
		{
			static constexpr bool value {function_serialize<Encoder, T>::value ||
										 member_function_serialize<Encoder, T>::value};
		};

		template <typename T>
		struct is_range
		{
			static constexpr bool value {utility::templates::is_trivial_container<T>::value ||
										 utility::templates::is_complex_container<T>::value};
		};

		template <typename T, typename Encoder = encode_to_format>
		struct is_collection_range
		{
			static constexpr bool value {
			  is_range<T>::value &&
			  (function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value ||
			   member_function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value)};
		};
		template <typename T, typename Encoder>
		struct is_collection_range<T*, Encoder>
		{
			static constexpr bool value {
			  is_range<T>::value &&
			  (function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value ||
			   member_function_serialize<Encoder, typename utility::binary::get_contained_type<T>::type>::value)};
		};

		template <typename T>
		struct is_keyed_range
		{
			static constexpr bool value {utility::templates::is_associative_container<T>::value};
		};
	}	 // namespace details

	class serializer
	{
		template <typename T, typename Encoder>
		inline static void check()
		{
			static_assert(details::HasSerializationName<T>,
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
					auto value_opt {polymorphic_id.get().as_value_content()};

					uint64_t id = stoull(psl::string8_t(value_opt.value().second));
					if(auto it = m_Factory.find(id); it != m_Factory.end())
					{
						target = (T*)((*it->second)());
					}
					else if(auto poly_it = accessor::polymorphic_data().find(id);
							poly_it != accessor::polymorphic_data().end())
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
			psl::format::container cont {psl::to_string8_t(res.value())};
			return deserialize<Encoder, T>(target, cont, name);
		}

		template <typename Encoder, typename T>
		bool deserialize(T& target, psl::string8::view filename, std::optional<psl::string8::view> name = {})
		{
			auto res = utility::platform::file::read(psl::from_string8_t(filename));
			if(!res) return false;
			psl::format::container cont {psl::to_string8_t(res.value())};
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
			using psl::serialization::serialize;
			serialize(s, obj);
		}
	}
	template <typename S, typename T>
	inline auto accessor::serialize(S& s, T& obj) -> decltype(obj.serialize(s))
	{
		if constexpr(accessor::supports_polymorphism<T>())
		{
			auto id = polymorphic_id(obj);
			if constexpr(details::IsEncoder<S>)
			{
				polymorphic_data()[id]->encode(&obj, s);
			}
			else if constexpr(details::IsDecoder<S>)
			{
				polymorphic_data()[id]->decode(&obj, s);
			}
		}
		else
		{
			obj.serialize(s);
		}
	}
}	 // namespace psl::serialization