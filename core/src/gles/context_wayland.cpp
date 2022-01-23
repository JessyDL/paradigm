
#if defined(SURFACE_WAYLAND)

#include "gles/context.hpp"
#include "gfx/limits.hpp"
#include "logging.hpp"
#include "os/surface.hpp"

using namespace core;
using namespace core::igles;


context::context(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 psl::string8::view name)
{


}

context::~context()
{

}
void context::enable(const core::os::surface& surface)
{


}
bool context::swapbuffers(core::resource::handle<core::os::surface> surface) { return false; }
#endif