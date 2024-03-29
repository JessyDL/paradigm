#include "core/gles/program_cache.hpp"
#include "core/gles/program.hpp"
#include "core/resource/resource.hpp"

using namespace core::igles;
using namespace core::resource;

program_cache::program_cache(core::resource::cache_t& cache,
							 const core::resource::metadata& metaData,
							 psl::meta::file* metaFile)
	: m_Cache(&cache) {}


core::resource::handle<core::igles::program> program_cache::get(const psl::UID& uid,
																core::resource::handle<core::data::material_t> data) {
	auto handle = m_Cache->find<program>(uid);
	if(handle.state() != core::resource::status::loaded)
		handle = m_Cache->create_using<program>(uid, data);

	return handle;
}
