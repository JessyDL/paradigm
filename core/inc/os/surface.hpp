#pragma once
#include "data/window.hpp"
#include "psl/meta.hpp"
#include "resource/resource.hpp"

#if defined(SURFACE_WIN32)
	#ifndef _WINDEF_
struct HWND__;	  // Forward or never
typedef HWND__* HWND;
struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
	#endif
#elif defined(SURFACE_XCB)
	#include <xcb/xcb.h>
#elif defined(PLATFORM_ANDROID)
	#include <android/asset_manager.h>
	#include <android/native_activity.h>
	#include <android_native_app_glue.h>
	#include <sys/system_properties.h>
#endif

namespace core::systems
{
class input;
}
namespace core::ivk
{
class swapchain;
}

namespace core::os
{
/// \brief primitive object that create a surface we can render on.
///
/// create a surface we can render on, which, depending on the platform could be
/// anything from a resizeable window to the sole surface we can present on (ex. mobile and console platforms).
class surface
{
  public:
	surface(core::resource::cache_t& cache,
			const core::resource::metadata& metaData,
			psl::meta::file* metaFile,
			core::resource::handle<data::window> data);
	~surface();
	surface(const surface&)			   = delete;
	surface(surface&&)				   = delete;
	surface& operator=(const surface&) = delete;
	surface& operator=(surface&&)	   = delete;

	/// \brief returns the window data that was used to initialize this window.
	/// \warning the data can potentially have changed since initialization by external sources.
	const core::data::window& data() const;

	/// \brief marks the surface to be terminated.
	///
	/// when invoking this method, the surface will be marked for destruction.
	/// \warning this is a suggestion, and actual shutdown might still
	/// take some time.
	void terminate();

	/// \returns if the surface is "open" (i.e initialized and not-terminated)
	bool open() const;

	/// \brief ticks the input system and returns if the surface is still open() after.
	/// \returns if the surface has been terminated (false) or is still open (true).
	bool tick();

	/// \brief suggests the surface to become focused.
	///
	/// the strength of the focus suggestion is based on the platform specific implementation
	/// of focus.
	/// \param[in] value sets the focus value.
	void focus(bool value);
	/// \returns if the platform considers this surface to be 'in focus'.
	bool focused() const;

	/// \brief suggests a new size for the surface.
	/// \warning depending on the platform, resize becomes a no-op, and in general should
	/// be considered a no-op unless the surface is rendering windowed, or borderless-windowed.
	/// \param[in] width the suggested width of the surface.
	/// \param[in] height the suggested height of the surface.
	/// \returns if the resize was successfull
	/// \note the changing of the surface to a new resolution, regardless of being the requested
	/// width and/or height is considered a success. Only when the surface cannot change its resolution
	/// at all will resize return false.
	bool resize(uint32_t width, uint32_t height);

	/// \brief the input instance associated with this surface.
	///
	/// every surface has its own input system even though that some platforms cannot have multiple
	/// surfaces, or that their input systems boil down to a singleton.
	/// by returning a specific instance, regardless of this we ensure that all access and handling is uniform
	/// across the various platforms.
	core::systems::input& input() const noexcept;

	/// \brief this method will be called by the swapchain class, so that the surface knows who to notify of resize
	/// events, etc.. \todo can we hide this?
	void register_swapchain(core::resource::handle<core::ivk::swapchain> swapchain);

	void trap_cursor(bool state) noexcept;
	bool is_cursor_trapped() const noexcept;
#if defined(SURFACE_WIN32)
	HINSTANCE surface_instance() const { return win32_instance; };
	HWND surface_handle() const { return win32_window; };
#elif defined(SURFACE_XCB)
	xcb_connection_t* connection() const { return _xcb_connection; }
	xcb_window_t surface_handle() const { return _xcb_window; };
#elif defined(SURFACE_D2D) || defined(PLATFORM_ANDROID)
#else
	#error no suitable surface was selected
#endif
  private:
	/// \brief platform specific method that initializes the surface and its resources.
	bool init_surface();
	/// \brief platform specific method that deinitializes the surface and its resources.
	void deinit_surface();
	/// \brief platform specific method that updates the surface and checks for messages.
	void update_surface();
	/// \brief platform specific method that gets called on resize events.
	void resize_surface();
#if defined(SURFACE_XCB)
	void handle_event(const xcb_generic_event_t* event);
#endif

	core::resource::handle<data::window> m_Data;
	std::vector<core::resource::handle<core::ivk::swapchain>> m_Swapchains;
	bool m_Focused {false};
	bool m_Open {false};
	bool m_IndicatorClipped {false};
	bool m_IndicatorVisible {true};
	bool m_IndicatorLocked {false};
	core::systems::input* m_InputSystem;
#if defined(SURFACE_WIN32)
	HINSTANCE win32_instance = NULL;
	HWND win32_window		 = NULL;
	psl::pstring_t win32_class_name;
	static uint64_t win32_class_id_counter;
	// void TranslateInputMessage(MSG *msg);
#elif defined(SURFACE_XCB)
	xcb_connection_t* _xcb_connection;
	xcb_screen_t* screen;
	xcb_window_t _xcb_window;
	xcb_intern_atom_reply_t* atom_wm_delete_window;
#endif
};
}	 // namespace core::os
