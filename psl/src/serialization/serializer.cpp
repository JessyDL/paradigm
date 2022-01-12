#include "psl/serialization/serializer.hpp"

std::unique_ptr<std::unordered_map<uint64_t, psl::serialization::polymorphic_data_t*>> psl::serialization::details::m_PolymorphicData = nullptr;

std::unordered_map<uint64_t, psl::serialization::polymorphic_data_t*>& psl::serialization::accessor::polymorphic_data()
{
	//TODO: verify single instance on both GNU/Linux & Windows
	if(psl::serialization::details::m_PolymorphicData == nullptr)
	{
		psl::serialization::details::m_PolymorphicData = std::make_unique<std::unordered_map<uint64_t, psl::serialization::polymorphic_data_t*>>();
	}

	return *psl::serialization::details::m_PolymorphicData;
}
