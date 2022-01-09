#include "gles/program_cache.h"
#include "gles/program.h"
#include "resource/resource.hpp"

using namespace core::igles;
using namespace core::resource;

program_cache::program_cache(core::resource::cache& cache,
							 const core::resource::metadata& metaData,
							 psl::meta::file* metaFile) :
	m_Cache(&cache)
{}


core::resource::handle<core::igles::program> program_cache::get(const psl::UID& uid,
																core::resource::handle<core::data::material> data)
{
	auto handle = m_Cache->find<program>(uid);
	if(handle.state() != core::resource::state::loaded) handle = m_Cache->create_using<program>(uid, data);

	return handle;
}