#include "data/skeleton.h"
#include "resource/resource.hpp"

using namespace core::data;
using namespace core::resource;


skeleton::skeleton(core::resource::cache& cache, const core::resource::metadata& metaData,
				   psl::meta::file* metaFile) noexcept
{}

bool skeleton::has_bones() const noexcept { return m_Bones.value.size() > 0; }
size_t skeleton::size() const noexcept { return m_Bones.value.size(); }

size_t skeleton::bone::size() const noexcept { return m_WeightIDs.value.size(); }