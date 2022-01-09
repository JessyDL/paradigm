#if defined(SURFACE_WAYLAND)
#include "os/surface.h"
using namespace core::os;
using namespace core;

/*static*/ void
surface::registryGlobalCb(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
	surface* self = reinterpret_cast<surface*>(data);
	self->registryGlobal(registry, name, interface, version);
}

/*static*/ void surface::seatCapabilitiesCb(void* data, wl_seat* seat, uint32_t caps)
{
	surface* self = reinterpret_cast<surface*>(data);
	self->seatCapabilities(seat, caps);
}

/*static*/ void surface::pointerEnterCb(void* data,
										wl_pointer* pointer,
										uint32_t serial,
										wl_surface* surface,
										wl_fixed_t sx,
										wl_fixed_t sy)
{}

/*static*/ void surface::pointerLeaveCb(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface) {}

/*static*/ void surface::pointerMotionCb(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
	surface* self = reinterpret_cast<surface*>(data);
	self->pointerMotion(pointer, time, sx, sy);
}
void surface::pointerMotion(wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
	//(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
}

/*static*/ void surface::pointerButtonCb(void* data,
										 wl_pointer* pointer,
										 uint32_t serial,
										 uint32_t time,
										 uint32_t button,
										 uint32_t state)
{
	surface* self = reinterpret_cast<surface*>(data);
	self->pointerButton(pointer, serial, time, button, state);
}

void surface::pointerButton(struct wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{}

/*static*/ void surface::pointerAxisCb(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	surface* self = reinterpret_cast<surface*>(data);
	self->pointerAxis(pointer, time, axis, value);
}

void surface::pointerAxis(wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {}

/*static*/ void
surface::keyboardKeymapCb(void* data, struct wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size)
{}

/*static*/ void surface::keyboardEnterCb(void* data,
										 struct wl_keyboard* keyboard,
										 uint32_t serial,
										 struct wl_surface* surface,
										 struct wl_array* keys)
{}

/*static*/ void
surface::keyboardLeaveCb(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface)
{}

/*static*/ void surface::keyboardKeyCb(void* data,
									   struct wl_keyboard* keyboard,
									   uint32_t serial,
									   uint32_t time,
									   uint32_t key,
									   uint32_t state)
{
	surface* self = reinterpret_cast<surface*>(data);
	self->keyboardKey(keyboard, serial, time, key, state);
}

void surface::keyboardKey(struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	switch(key)
	{
		// case KEY_ESC: terminate(); break;
	}
}

/*static*/ void surface::keyboardModifiersCb(void* data,
											 struct wl_keyboard* keyboard,
											 uint32_t serial,
											 uint32_t mods_depressed,
											 uint32_t mods_latched,
											 uint32_t mods_locked,
											 uint32_t group)
{}

void surface::seatCapabilities(wl_seat* seat, uint32_t caps)
{
	if((caps & WL_SEAT_CAPABILITY_POINTER) && !m_Pointer)
	{
		m_Pointer												 = wl_seat_get_pointer(seat);
		static const struct wl_pointer_listener pointer_listener = {
		  pointerEnterCb,
		  pointerLeaveCb,
		  pointerMotionCb,
		  pointerButtonCb,
		  pointerAxisCb,
		};
		wl_pointer_add_listener(m_Pointer, &pointer_listener, this);
	}
	else if(!(caps & WL_SEAT_CAPABILITY_POINTER) && m_Pointer)
	{
		wl_pointer_destroy(m_Pointer);
		m_Pointer = nullptr;
	}

	if((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !m_Keyboard)
	{
		m_Keyboard												   = wl_seat_get_keyboard(seat);
		static const struct wl_keyboard_listener keyboard_listener = {
		  keyboardKeymapCb,
		  keyboardEnterCb,
		  keyboardLeaveCb,
		  keyboardKeyCb,
		  keyboardModifiersCb,
		};
		wl_keyboard_add_listener(m_Keyboard, &keyboard_listener, this);
	}
	else if(!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && m_Keyboard)
	{
		wl_keyboard_destroy(m_Keyboard);
		m_Keyboard = nullptr;
	}
}

void surface::registryGlobal(wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
	if(strcmp(interface, "wl_compositor") == 0)
	{
		m_Compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 3);
	}
	else if(strcmp(interface, "wl_shell") == 0)
	{
		m_Shell = (wl_shell*)wl_registry_bind(registry, name, &wl_shell_interface, 1);
	}
	else if(strcmp(interface, "wl_seat") == 0)
	{
		m_Seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);

		static const struct wl_seat_listener seat_listener = {
		  seatCapabilitiesCb,
		};
		wl_seat_add_listener(m_Seat, &seat_listener, this);
	}
}

/*static*/ void surface::registryGlobalRemoveCb(void* data, struct wl_registry* registry, uint32_t name) {}


static void PingCb(void* data, struct wl_shell_surface* shell_surface, uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

static void
ConfigureCb(void* data, struct wl_shell_surface* shell_surface, uint32_t edges, int32_t width, int32_t height)
{}

static void PopupDoneCb(void* data, struct wl_shell_surface* shell_surface) {}

bool surface::init_surface()
{
	m_Display = wl_display_connect(NULL);
	if(!m_Display)
	{
		core::os::log->critical("failed to connect to the wayland display");
		exit(1);
	}

	m_Registry = wl_display_get_registry(m_Display);
	if(!m_Registry)
	{
		core::os::log->critical("failed to register the wayland display");
		exit(1);
	}

	static const struct wl_registry_listener registry_listener = {registryGlobalCb, registryGlobalRemoveCb};
	wl_registry_add_listener(m_Registry, &registry_listener, this);
	wl_display_dispatch(m_Display);
	wl_display_roundtrip(m_Display);
	if(!m_Compositor || !m_Shell || !m_Seat)
	{
		core::os::log->critical("failed to bind the wayland protocols");
		exit(1);
	}


	m_Surface	   = wl_compositor_create_surface(m_Compositor);
	m_ShellSurface = wl_shell_get_shell_surface(m_Shell, m_Surface);

	static const struct wl_shell_surface_listener shell_surface_listener = {PingCb, ConfigureCb, PopupDoneCb};

	wl_shell_surface_add_listener(m_ShellSurface, &shell_surface_listener, this);
	wl_shell_surface_set_toplevel(m_ShellSurface);
	wl_shell_surface_set_title(m_ShellSurface, m_Data->name().data());

	return true;
}

void surface::deinit_surface()
{
	wl_shell_surface_destroy(m_ShellSurface);
	wl_surface_destroy(m_Surface);
	if(m_Keyboard) wl_keyboard_destroy(m_Keyboard);
	if(m_Pointer) wl_pointer_destroy(m_Pointer);
	wl_seat_destroy(m_Seat);
	wl_shell_destroy(m_Shell);
	wl_compositor_destroy(m_Compositor);
	wl_registry_destroy(m_Registry);
	wl_display_disconnect(m_Display);
}

void surface::focus(bool value) {}


void surface::update_surface() {}


void surface::resize_surface() {}
#endif