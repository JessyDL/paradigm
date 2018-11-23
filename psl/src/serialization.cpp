
#include "serialization.h"

//std::unique_ptr<std::unordered_map<uint64_t, serialization::polymorphic_data*>> serialization::accessor::m_PolymorphicData;
//std::unordered_map<uint64_t, serialization::polymorphic_data*>* serialization::accessor::initialized;


std::unordered_map<uint64_t, psl::serialization::polymorphic_data*>& psl::serialization::accessor::polymorphic_data()
{
	static std::unique_ptr<std::unordered_map<uint64_t, serialization::polymorphic_data*>> m_PolymorphicData;

	// figure out why multiple instances exist, why the seperate library linking results in multi initialization
	if (m_PolymorphicData == nullptr)
	{
		m_PolymorphicData = std::make_unique<std::unordered_map<uint64_t, psl::serialization::polymorphic_data*>>();
	}
	
	return *m_PolymorphicData;
}
