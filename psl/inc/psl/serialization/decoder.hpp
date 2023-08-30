#pragma once
#include "psl/serialization/serializer.hpp"
#include "psl/string_utils.hpp"
#include "psl/ustring.hpp"

namespace psl::serialization {
class decode_from_format : decoder {
	using codec_t	  = decode_from_format;
	using container_t = psl::format::container;

  public:
	decode_from_format(psl::format::container& container,
					   psl::format::handle* root,
					   std::unordered_map<uint64_t, invocable_wrapper_base*> factory = {})
		: m_Container(container), m_CollectionStack {{root}}, m_Factory(factory) {};

	template <auto Name, typename T>
	decode_from_format& operator<<(property<Name, T>& property) {
		parse(property);
		return *this;
	}

	template <typename... Args>
	void parse(Args&&... args) {
		(parse_internal(args), ...);
	}

	// catch-all for normal types
	template <auto Name, typename T>
	void parse(property<Name, T>& property) {
		parse_internal(property.value, property.name());
	}


	// catch-all for normal, and polymorphic types
	template <auto Name, typename T>
	void parse(property<Name, T*>& property) {
		if constexpr(details::is_collection<T>::value) {
			property.value = create_polymorphic_collection<T>();
			parse_collection(*property.value, property.name());
		} else {
			property.value = new T();
			parse_internal(*property.value, property.name());
		}
	}


	template <psl::details::fixed_astring Name, typename T>
	void parse(T& property) {
		parse_internal(property, Name);
	}

	void resolve_references() {
		for(auto ref : m_ReferenceNodes) {
			auto value_opt {ref.second->get().as_reference()};
			if(!value_opt)
				continue;

			if(auto it = m_ReferenceMap.find(value_opt.value()); it != m_ReferenceMap.end()) {
				std::memcpy((void*)ref.first, &it->second, sizeof(void*));
			}
		}
		for(auto ptr : m_PointerNodes) {
			auto value_opt {ptr.second->get().as_reference()};
			if(!value_opt)
				continue;

			if(auto it = m_ReferenceMap.find(value_opt.value()); it != m_ReferenceMap.end()) {
				std::memcpy((void*)ptr.first, &it->second, sizeof(void*));
			}
		}
	}

  private:
	template <typename T>
	void parse_collection(T& property, std::optional<psl::string8::view> override_name = {}) {
		if(m_CollectionStack.size() > 0) {
			parse_collection(property,
							 m_Container.index_of(m_CollectionStack.top()->get(),
												  (override_name) ? override_name.value() : accessor::name<T>()));
		} else {
			parse_collection(property,
							 m_Container.index_of((override_name) ? override_name.value() : accessor::name<T>()));
		}
	}

	template <typename T>
	void parse_collection(T& property, psl::format::nodes_t index) {
		if(index == std::numeric_limits<psl::format::nodes_t>::max())
			return;

		auto& collection = m_Container[index];
		m_CollectionStack.push(&collection);

		m_ReferenceMap[&collection] = (std::uintptr_t)&property;

		if constexpr(details::member_function_serialize<codec_t, T>::value &&
					 !details::function_serialize<codec_t, T>::value) {
			accessor::serialize(*this, property);
		} else if constexpr(!details::member_function_serialize<codec_t, T>::value &&
							details::function_serialize<codec_t, T>::value) {
			accessor::serialize_fn(*this, property);
		} else {
			static_assert(psl::utility::templates::always_false_v<T>,
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
	T* create_polymorphic_collection(std::optional<psl::string8::view> override_name = {}) {
		if(m_CollectionStack.size() > 0) {
			auto& collection = m_Container.find(m_CollectionStack.top()->get(),
												(override_name) ? override_name.value() : accessor::name<T>());
			m_CollectionStack.push(&collection);
		} else {
			auto& collection = m_Container.find((override_name) ? override_name.value() : accessor::name<T>());
			m_CollectionStack.push(&collection);
		}

		auto& polymorphic_id = m_Container.find(m_CollectionStack.top()->get(), "POLYMORPHIC_ID");
		T* target			 = nullptr;
		if(polymorphic_id.exists()) {
			auto value_opt {polymorphic_id.get().as_value_content()};

			uint64_t id = stoull(psl::string8_t(value_opt.value().second));
			if(auto it = m_Factory.find(id); it != m_Factory.end()) {
				target = (T*)((*it->second)());
			} else if(auto it = accessor::polymorphic_data().find(id); it != accessor::polymorphic_data().end()) {
				target = (T*)((*it->second->factory)());
			} else {
				target = new T();
			}
		} else {
			target = new T();
		}

		m_CollectionStack.pop();
		return target;
	}

	template <typename T>
	void parse_internal(T& value, psl::string8::view name) {
		constexpr bool is_range		 = details::is_range<T>::value;
		constexpr bool is_collection = details::is_collection<typename std::conditional<
		  is_range,
		  typename std::remove_pointer<typename psl::utility::binary::get_contained_type<T>::type>::type,
		  T>::type>::value;

		if constexpr(is_collection && !is_range) {
			parse_collection(value, name);
		} else if constexpr(is_collection && is_range) {
			using contained_t = typename psl::utility::binary::get_contained_type<T>::type;

			size_t size = 0;
			if(m_CollectionStack.size() > 0) {
				auto& collection = m_Container.find(m_CollectionStack.top()->get(), name);
				if(!collection.exists())
					return;
				m_CollectionStack.push(&collection);
				auto value_opt {collection.get().as_collection()};
				size = value_opt.value_or(0);
			} else {
				auto& collection = m_Container.find(name);
				if(!collection.exists())
					return;
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
			for(auto i = begin; i < end; ++i) {
				if((size_t)(m_Container[psl::format::nodes_t(i)].get().depth()) - 1u ==
				   (size_t)(m_CollectionStack.top()->get().depth())) {
					if constexpr(std::is_pointer<contained_t>::value) {
						using deref_t = typename std::remove_pointer<contained_t>::type;
						value.emplace_back(create_polymorphic_collection<deref_t>(psl::utility::to_string(actual_index)));
						parse_collection(*value[actual_index], (psl::format::nodes_t)(i));
					} else {
						value.emplace_back();
						parse_collection(value[actual_index], (psl::format::nodes_t)(i));
					}
					++actual_index;
				}
			}
			m_CollectionStack.pop();
		} else if constexpr(details::is_keyed_range<T>::value) {
			size_t size = 0;
			if(m_CollectionStack.size() > 0) {
				auto& collection = m_Container.find(m_CollectionStack.top()->get(), name);
				m_CollectionStack.push(&collection);

				auto value_opt {collection.get().as_collection()};
				size = value_opt.value_or(0);
			} else {
				auto& collection = m_Container.find(name);
				m_CollectionStack.push(&collection);

				auto value_opt {collection.get().as_collection()};
				size = value_opt.value_or(0);
			}

			size_t begin = m_Container.index_of(m_CollectionStack.top()->get()) + 1u;
			size_t end	 = begin + size;
			// size_t actual_index = 0;

			static_assert(details::is_keyed_range<T>::value, "never seen");

			for(auto i = begin; i < end; ++i) {
				if((size_t)(m_Container[psl::format::nodes_t(i)].get().depth()) - 1u ==
				   (size_t)(m_CollectionStack.top()->get().depth())) {
					// auto& pair = value.emplace(, typename T::value_type{});
					auto& sub_val =
					  value[psl::utility::from_string<typename psl::utility::templates::get_key_type<T>::type>(
					  m_Container[psl::format::nodes_t(i)].get().name())];
					parse_collection(sub_val, (psl::format::nodes_t)(i));
				}
			}
			m_CollectionStack.pop();
		} else if constexpr(is_range) {
			using contained_t = typename psl::utility::binary::get_contained_type<T>::type;

			auto& node = (m_CollectionStack.size() > 0) ? m_Container.find(m_CollectionStack.top()->get(), name)
														: m_Container.find(name);
			if(!node.exists())
				return;
			value.clear();

			auto value_opt {node.get().as_value_range_content()};
			if(value_opt) {
				auto data_content {std::move(value_opt.value())};
				value.reserve(data_content.size());

				for(const auto& it : data_content) {
					if constexpr(std::is_pointer<contained_t>::value) {
						using deref_t = typename std::remove_pointer<contained_t>::type;
						value.emplace_back(new deref_t(
						  psl::utility::from_string<typename std::remove_pointer<contained_t>::type>(it.second)));
					} else {
						value.emplace_back(
						  psl::utility::from_string<typename psl::utility::binary::get_contained_type<T>::type>(it.second));
					}
				}
			}
			m_ReferenceMap[&node] = (std::uintptr_t)&value;
		} else {
			auto& node = (m_CollectionStack.size() > 0) ? m_Container.find(m_CollectionStack.top()->get(), name)
														: m_Container.find(name);
			if(!node.exists())
				return;

			if(auto value_opt = node.get().as_value_content(); value_opt) {
				value = psl::utility::from_string<T>(value_opt.value().second);
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
}	 // namespace psl::serialization
