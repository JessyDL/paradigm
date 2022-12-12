#pragma once
#include "psl/serialization/serializer.hpp"
#include "psl/ustring.hpp"
#include "psl/string_utils.hpp"

namespace psl::serialization
{
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
			parse_internal(property.value, property.name());
		}

		template <auto Name, typename T>
		void parse(property<Name, T*>& property)
		{
			parse_internal(*property.value, property.name());
		}

		template <psl::details::fixed_astring Name, typename T >
		void parse(T& property)
		{
			parse_internal(property, Name);
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
		void parse_internal(T& value, psl::string8::view name)
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
}
