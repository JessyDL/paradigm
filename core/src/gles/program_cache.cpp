#include "gles/program_cache.h"
#include "resource/resource.hpp"
#include "gles/program.h"

using namespace core::igles;
using namespace core::resource;

program_cache::program_cache(const psl::UID& uid, cache& cache) : m_Cache(&cache) {}


core::resource::handle<core::igles::program> program_cache::get(const psl::UID& uid,
	core::resource::handle<core::data::material> data)
{
	auto handle = create_shared<program>(*m_Cache, uid);
	if(handle.resource_state() != core::resource::state::LOADED) handle.load(data);

	return handle;
}