#pragma once
#include <optional>
#include <stack>
#include <unordered_map>

#include "psl/binary_utils.h"
#include "psl/format.h"
#include "psl/logging.h"
#include "psl/platform_utils.h"
#include "psl/serialization/property.hpp"
#include "psl/template_utils.h"
#include "psl/ustring.h"

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

	struct polymorphic_data
	{
		uint64_t id;
		std::function<void(void*, encode_to_format&)> encode;
		std::function<void(void*, decode_from_format&)> decode;
		invocable_wrapper_base* factory;


		std::vector<std::pair<psl::string8_t, uint64_t>> derived;
		~polymorphic_data() { delete(factory); };
	};

	namespace details {
		extern std::unique_ptr<std::unordered_map<uint64_t, psl::serialization::polymorphic_data*>> m_PolymorphicData;
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
		static constexpr auto test_is_polymorphic() noexcept -> is_polymorphic<T>::type
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
		inline static constexpr const char* name();

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


		static std::unordered_map<uint64_t, polymorphic_data*>& polymorphic_data();

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
			}
			->std::same_as<std::true_type>;
		};
	}	 // namespace details

	template <typename T>
	inline constexpr const char* accessor::name()
	{
		static_assert(details::HasSerializationName<T>,
						"\n\tPlease make sure your class fullfills any of the following requirements:\n"
						"\t\t - has a public variable \"static constexpr const char* serialization_name\"\n"
						"\t\t - or a private variable \"static constexpr const char* serialization_name\" and added "
						"\"friend class psl::serialization::accessor\"\n");
		return T::serialization_name;
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

	class decode_from_format : decoder
	{
		using codec_t	  = decode_from_format;
		using container_t = psl::format::container;

	  public:
		decode_from_format(psl::format::container& container,
						   psl::format::handle* root,
						   std::unordered_map<uint64_t, invocable_wrapper_base*> factory = {}) :
			m_Container(container),
			m_CollectionStack {{root}}, m_Factory(factory) {};

		template <auto Name, typename T>
		decode_from_format& operator<<(property<Name, T>& property)
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
		template <auto Name, typename T>
		void parse(property<Name, T>& property)
		{
			parse_internal(property.value, property.name());
		}


		// catch-all for normal, and polymorphic types
		template <auto Name, typename T>
		void parse(property<Name, T*>& property)
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

		void resolve_references()
		{
			for(auto ref : m_ReferenceNodes)
			{
				auto value_opt {ref.second->get().as_reference()};
				if(!value_opt) continue;

				if(auto it = m_ReferenceMap.find(value_opt.value()); it != m_ReferenceMap.end())
				{
					std::memcpy((void*)ref.first, &it->second, sizeof(void*));
				}
			}
			for(auto ptr : m_PointerNodes)
			{
				auto value_opt {ptr.second->get().as_reference()};
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
				auto value_opt {polymorphic_id.get().as_value_content()};

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
			  is_range,
			  typename std::remove_pointer<typename utility::binary::get_contained_type<T>::type>::type,
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
					if(!collection.exists()) return;
					m_CollectionStack.push(&collection);
					auto value_opt {collection.get().as_collection()};
					size = value_opt.value_or(0);
				}
				else
				{
					auto& collection = m_Container.find(name);
					if(!collection.exists()) return;
					m_CollectionStack.push(&collection);
					auto value_opt {collection.get().as_collection()};
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

					auto value_opt {collection.get().as_collection()};
					size = value_opt.value_or(0);
				}
				else
				{
					auto& collection = m_Container.find(name);
					m_CollectionStack.push(&collection);

					auto value_opt {collection.get().as_collection()};
					size = value_opt.value_or(0);
				}

				size_t begin = m_Container.index_of(m_CollectionStack.top()->get()) + 1u;
				size_t end	 = begin + size;
				// size_t actual_index = 0;

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

				auto value_opt {node.get().as_value_range_content()};
				if(value_opt)
				{
					auto data_content {std::move(value_opt.value())};
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
		using codec_t	  = encode_to_format;
		using container_t = psl::format::container;

	  public:
		encode_to_format(psl::format::container& container, psl::format::handle* root) :
			m_Container(container), m_CollectionStack {{root}} {};

		template <auto Name, typename T>
		encode_to_format& operator<<(property<Name, T>& property)
		{
			parse(property);
			return *this;
		}

		template <typename... Props>
		void parse(Props&&... props)
		{
			(parse(props), ...);
		}

		template <auto Name, typename T>
		void parse(property<Name, T>& property)
		{
			parse(property.value, property.name());
		}

		template <auto Name, typename T>
		void parse(property<Name, T*>& property)
		{
			parse(*property.value, property.name());
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
					  pair.second, utility::to_string<typename utility::templates::get_key_type<T>::type>(pair.first));
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

	static property<"POLYMORPHIC_ID", psl::string8_t> p;

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
			auto& pData = accessor::polymorphic_data();
			if(pData.find(ID) != pData.end())
			{
				LOG_FATAL("Encountered duplicate Polymorphic ID in the serialization: ", ID);
				exit(-1);
			}


			static polymorphic_data data;

			data.id = ID;
			auto lambda {[]() { return new T(); }};
			data.factory = new invocable_wrapper<decltype(lambda)>(std::forward<decltype(lambda)>(lambda));

			data.encode = [](void* this_, encode_to_format& s) {
				T* t	= static_cast<T*>(this_);
				p.value = utility::to_string(accessor::polymorphic_id(t));
				s.parse(p);
				accessor::serialize_directly<encode_to_format, T>(s, *t);
				// t->serialize(s);
			};
			data.decode = [](void* this_, decode_from_format& s) {
				accessor::serialize_directly<decode_from_format, T>(s, *static_cast<T*>(this_));
				// static_assert(false, "use of undefined codec");
			};
			pData[ID] = &data;
		}

		virtual ~polymorphic() {};
		uint64_t PolymorphicID() const override { return accessor::id<T>(); };
		static constexpr uint64_t ID {accessor::id<T>()};
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