#include "stdafx.h"
#include "systems/input.h"
#include "os/surface.h"

#ifdef SURFACE_XCB
using namespace core::systems;
using kc = input::keycode;

const kc XCB_NATIVE_TO_HID[256] = {
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED, // 7

	kc::UNDEFINED,
	kc::ESC,
	kc::KB_1,
	kc::KB_2,
	kc::KB_3,
	kc::KB_4,
	kc::KB_5,
	kc::KB_6, // 15

	kc::KB_7,
	kc::KB_8,
	kc::KB_9,
	kc::KB_0,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::DEL,
	kc::TAB, // 23

	kc::A,
	kc::Z,
	kc::E,
	kc::R,
	kc::T,
	kc::Y,
	kc::U,
	kc::I, // 31

	kc::O,
	kc::P,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::ENTER,
	kc::LEFT_CONTROL,
	kc::Q,
	kc::S, // 39

	kc::D,
	kc::F,
	kc::G,
	kc::H,
	kc::J,
	kc::K,
	kc::L,
	kc::M, // 47

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::LEFT_SHIFT,
	kc::UNDEFINED,
	kc::W,
	kc::X,
	kc::C,
	kc::V, // 55

	kc::B,
	kc::N,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::RIGHT_CONTROL,
	kc::UNDEFINED, // 63

	kc::LEFT_ALT,
	kc::SPACE,
	kc::CAPSLOCK,
	kc::F1,
	kc::F2,
	kc::F3,
	kc::F4,
	kc::F5, // 71

	kc::F6,
	kc::F7,
	kc::F8,
	kc::F9,
	kc::F10,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED, // 79

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED, // 87

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::F11, // 95

	kc::F12,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED, // 103

	kc::UNDEFINED,
	kc::RIGHT_CONTROL,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::RIGHT_ALT,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UP, // 111

	kc::UNDEFINED,
	kc::LEFT,
	kc::RIGHT,
	kc::UNDEFINED,
	kc::DOWN,
	kc::UNDEFINED,
	kc::INSERT,
	kc::DELETE_FORWARD, // 119

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED, // 127

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,

	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
	kc::UNDEFINED,
};

void input::handle_event(core::os::surface *const surface, const xcb_generic_event_t *event,
						 xcb_intern_atom_reply_t &delete_wm)
{
	switch(event->response_type & 0x7f)
	{
	case XCB_CLIENT_MESSAGE:
		if((*(xcb_client_message_event_t *)event).data.data32[0] == delete_wm.atom)
		{
			surface->terminate();
		}
		break;
	case XCB_DESTROY_NOTIFY: surface->terminate(); break;
	case XCB_CONFIGURE_NOTIFY:
	{
		const xcb_configure_notify_event_t *cfgEvent = (const xcb_configure_notify_event_t *)event;
		if(cfgEvent->width != surface->data().width() || cfgEvent->height != surface->data().height())
		{
			surface->resize(cfgEvent->width, cfgEvent->height);
		}
	}
	break;
	case XCB_KEY_PRESS:
	{
		const xcb_key_press_event_t *keypress_event = reinterpret_cast<const xcb_key_press_event_t *>(event);
		xcb_keycode_t code							= keypress_event->detail;
		core::systems::log->info(std::to_string((uint8_t)code));
		// return;
		m_OnKeyPressed.Execute((input::keycode)XCB_NATIVE_TO_HID[(uint8_t)code]);
		m_HeldKeys.emplace_back((input::keycode)XCB_NATIVE_TO_HID[(uint8_t)code]);
		// core::systems::log->info(keycode_names[(uint8_t)XCB_NATIVE_TO_HID[(uint8_t)code]]);
	}
	break;
	case XCB_KEY_RELEASE:
	{
		const xcb_key_release_event_t *keypress_event = reinterpret_cast<const xcb_key_release_event_t *>(event);
		xcb_keycode_t code							  = keypress_event->detail;
		// return;
		m_OnKeyReleased.Execute((input::keycode)XCB_NATIVE_TO_HID[(uint8_t)code]);
		m_HeldKeys.erase(
			std::remove(std::begin(m_HeldKeys), std::end(m_HeldKeys), (input::keycode)XCB_NATIVE_TO_HID[(uint8_t)code]),
			std::end(m_HeldKeys));
	}
	break;
	case XCB_MOTION_NOTIFY:
	{
		xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;

		mouse_delta mDelta;
		mDelta.x = motion->event_x - m_Cursor.x;
		mDelta.y = motion->event_y - m_Cursor.y;

		m_Cursor.x		= motion->event_x;
		m_Cursor.y		= motion->event_y;
		m_Cursor.width  = surface->data().width();
		m_Cursor.height = surface->data().height();

		m_OnMouseMoveDelta(mDelta);
		m_OnMouseMoveCoordinates(m_Cursor);
	}
	break;
	case XCB_BUTTON_RELEASE:
	{
		xcb_button_release_event_t *bp = (xcb_button_release_event_t *)event;
		core::systems::log->info(std::to_string(bp->detail));
		switch((uint8_t)bp->detail)
		{
		case 1u:
		{
			m_OnMouseReleased(mousecode::BTN_1);
			m_HeldMouseButtons.erase(
				std::remove(std::begin(m_HeldMouseButtons), std::end(m_HeldMouseButtons), mousecode::BTN_1),
				std::end(m_HeldMouseButtons));
		}
		break;
		case 3u:
		{
			m_OnMouseReleased(mousecode::BTN_2);
			m_HeldMouseButtons.erase(
				std::remove(std::begin(m_HeldMouseButtons), std::end(m_HeldMouseButtons), mousecode::BTN_2),
				std::end(m_HeldMouseButtons));
		}
		break;
		};
	}
	break;
	case XCB_BUTTON_PRESS:
	{
		xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
		core::systems::log->info(std::to_string(bp->detail));
		switch((uint8_t)bp->detail)
		{
		case 1u:
		{
			m_OnMousePressed(mousecode::BTN_1);
			m_HeldMouseButtons.emplace_back(mousecode::BTN_1);
		}
		break;
		case 3u:
		{
			m_OnMousePressed(mousecode::BTN_2);
			m_HeldMouseButtons.emplace_back(mousecode::BTN_2);
		}
		break;
		case 4u:
		case 5u:
		{
			scroll_delta delta;
			delta.y = (short)(bp->detail - 4) * 2 - 1;
			m_OnScroll(delta);
		}
		break;
		};
	}
	break;
	}
}

void input::tick(core::os::surface *const surface, xcb_connection_t *_xcb_connection,
				 xcb_intern_atom_reply_t &delete_wm)
{
	xcb_flush(_xcb_connection);
	xcb_generic_event_t *event;
	while((event = xcb_poll_for_event(_xcb_connection)))
	{
		handle_event(surface, event, delete_wm);
		free(event);
	}
}
#endif