#pragma once
#include "psl/serialization/decoder.hpp"
#include "psl/serialization/encoder.hpp"
#include "psl/serialization/serializer.hpp"

namespace psl::serialization {
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
class polymorphic final : public polymorphic_base {
  public:
	polymorphic() {
		auto& pData = accessor::polymorphic_data();
		if(pData.find(ID) != pData.end()) {
			LOG_FATAL("Encountered duplicate Polymorphic ID in the serialization: ", ID);
			exit(-1);
		}


		static polymorphic_data_t data;

		data.id = ID;
		auto lambda {[]() { return new T(); }};
		data.factory = new invocable_wrapper<decltype(lambda)>(std::forward<decltype(lambda)>(lambda));

		data.encode = [](void* this_, encode_to_format& s) {
			T* t	= static_cast<T*>(this_);
			p.value = psl::utility::to_string(accessor::polymorphic_id(t));
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
static const void notify_base(psl::string8_t name, uint64_t ID) {
	accessor::polymorphic_data()[accessor::id<Base>()]->derived.emplace_back(name, ID);
	if constexpr(sizeof...(Rest) > 0) {
		notify_base<Rest...>(name, ID);
	}
}

template <typename T, typename... Base>
static const uint64_t register_polymorphic() {
	static psl::serialization::polymorphic<T> polymorphic_container;
	if constexpr(sizeof...(Base) > 0) {
		notify_base<Base...>(accessor::name<T>(), accessor::id<T>());
	}
	return accessor::id<T>();
}
}	 // namespace psl::serialization
