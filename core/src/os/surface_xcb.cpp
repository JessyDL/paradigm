#include "stdafx.h"
#include "os/surface.h"
#ifdef SURFACE_XCB
#include <xcb/xcb.h>
#include "systems/input.h"

using namespace core::os;
using namespace core;


static inline xcb_intern_atom_reply_t *intern_atom_helper(xcb_connection_t *conn, bool only_if_exists, const char *str)
{
	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
	return xcb_intern_atom_reply(conn, cookie, NULL);
}

bool surface::init_surface()
{
	// Initialize XCB connection
	const xcb_setup_t *setup;
	xcb_screen_iterator_t iter;
	int scr;

	_xcb_connection = xcb_connect(NULL, &scr);
	if(_xcb_connection == NULL)
	{
		printf("Could not find a compatible Vulkan ICD!\n");
		fflush(stdout);
		exit(1);
	}

	setup = xcb_get_setup(_xcb_connection);
	iter  = xcb_setup_roots_iterator(setup);
	while(scr-- > 0) xcb_screen_next(&iter);
	screen = iter.data;

	uint32_t value_mask, value_list[32];
	assert(m_Data->width() > 0);
	assert(m_Data->height() > 0);

	_xcb_window = xcb_generate_id(_xcb_connection);

	value_mask	= XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[0] = screen->black_pixel;
	value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_EXPOSURE |
					XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS |
					XCB_EVENT_MASK_BUTTON_RELEASE;

	auto width  = m_Data->width();
	auto height = m_Data->height();
	if(m_Data->mode() == core::gfx::surface_mode::FULLSCREEN)
	{
		width  = screen->width_in_pixels;
		height = screen->height_in_pixels;
	}

	xcb_create_window(_xcb_connection, XCB_COPY_FROM_PARENT, _xcb_window, screen->root, 0, 0, width, height, 0,
					  XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);

	/* Magic code that will send notification when window is destroyed */
	xcb_intern_atom_reply_t *reply = intern_atom_helper(_xcb_connection, true, "WM_PROTOCOLS");
	atom_wm_delete_window		   = intern_atom_helper(_xcb_connection, false, "WM_DELETE_WINDOW");

	xcb_change_property(_xcb_connection, XCB_PROP_MODE_REPLACE, _xcb_window, (*reply).atom, 4, 32, 1,
						&(*atom_wm_delete_window).atom);

	xcb_change_property(_xcb_connection, XCB_PROP_MODE_REPLACE, _xcb_window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
						m_Data->name().size(), m_Data->name().data());

	free(reply);

	if(m_Data->mode() == core::gfx::surface_mode::FULLSCREEN)
	{
		xcb_intern_atom_reply_t *atom_wm_state = intern_atom_helper(_xcb_connection, false, "_NET_WM_STATE");
		xcb_intern_atom_reply_t *atom_wm_fullscreen =
			intern_atom_helper(_xcb_connection, false, "_NET_WM_STATE_FULLSCREEN");
		xcb_change_property(_xcb_connection, XCB_PROP_MODE_REPLACE, _xcb_window, atom_wm_state->atom, XCB_ATOM_ATOM, 32,
							1, &(atom_wm_fullscreen->atom));
		free(atom_wm_fullscreen);
		free(atom_wm_state);
	}

	xcb_map_window(_xcb_connection, _xcb_window);

	m_Open = true;
	return true;
}

void surface::deinit_surface()
{

	xcb_destroy_window(_xcb_connection, _xcb_window);
	xcb_disconnect(_xcb_connection);
}


void surface::focus(bool value) {}


void surface::update_surface() { m_InputSystem->tick(this, _xcb_connection, *atom_wm_delete_window); }


void surface::resize_surface() {}
#endif