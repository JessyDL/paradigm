#pragma once
#include "gfx/context.h"
#include "gfx/types.h"
#include "os/surface.h"
#include "psl/application_utils.h"
#include "resource/resource.hpp"

// for implementation file
#include "gfx/swapchain.h"

namespace assembler {
/// \brief console window
/// \details allows for user input and output.
class console {
  public:
	console(core::gfx::graphics_backend backend, core::resource::handle<core::data::window> window_data = {})
		: m_Cache(
			psl::meta::library(utility::application::path::library + "resources.metalib", {psl::to_string(backend)})) {
		using namespace core::gfx;
		using namespace core::os;
		using namespace core;
		assert(backend != graphics_backend::undefined);

		if(!window_data) {
			window_data = m_Cache.instantiate<data::window>("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
			window_data->name("console window");
		}
		m_Surface = m_Cache.create<surface>(window_data);
		if(!m_Surface) {
			core::log->critical("Could not create a OS surface to draw on.");
			return;
		}

		m_Context			  = m_Cache.create<core::gfx::context>(backend, psl::string8_t {"console"});
		auto swapchain_handle = m_Cache.create<core::gfx::swapchain>(m_Surface, m_Context);
	};

	void write(psl::string_view value);
	void endl();
	psl::string_view query(size_t line = 0);

  private:
	core::gfx::graphics_backend m_Backend {};
	core::resource::cache m_Cache;
	core::resource::handle<core::os::surface> m_Surface;
	core::resource::handle<core::gfx::context> m_Context;
};
}	 // namespace assembler