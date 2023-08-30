#if defined(SURFACE_WIN32)
	#include "core/logging.hpp"
	#include "core/os/surface.hpp"
	#include "core/systems/input.hpp"
	#include <Windows.h>

// https://chromium.googlesource.com/chromium/src/+/master/ui/events/keycodes/dom/keycode_converter_data.inc
// https://handmade.network/forums/t/2011-keyboard_inputs_-_scancodes,_raw_input,_text_input,_key_names/2

using namespace core::systems;

const uint8_t WIN_HID_TO_NATIVE[256] = {
  255, 255, 255, 255, 65,  66,	67,	 68,  69,  70,	71,	 72,  73,  74,	75,	 76,  77,  78,	79,	 80,  81,  82,
  83,  84,	85,	 86,  87,  88,	89,	 90,  49,  50,	51,	 52,  53,  54,	55,	 56,  57,  48,	13,	 27,  8,   9,
  32,  189, 187, 219, 221, 220, 255, 186, 222, 192, 188, 190, 191, 20,	112, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 44,  145, 19,	 45,  36,  33,	46,	 35,  34,  39,	37,	 40,  38,  144, 111, 106, 109, 107,
  255, 97,	98,	 99,  100, 101, 102, 103, 104, 105, 96,	 110, 255, 255, 255, 255, 124, 125, 126, 127, 128, 129,
  130, 131, 132, 133, 134, 135, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 17,  16,	18,	 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
const uint8_t WIN_NATIVE_TO_HID[256] = {
  255, 255, 255, 255, 255, 255, 255, 255, 42,  43,	255, 255, 255, 40,	255, 255, 225, 224, 226, 72,  57,  255,
  255, 255, 255, 255, 255, 41,	255, 255, 255, 255, 44,	 75,  78,  77,	74,	 80,  82,  79,	81,	 255, 255, 255,
  70,  73,	76,	 255, 39,  30,	31,	 32,  33,  34,	35,	 36,  37,  38,	255, 255, 255, 255, 255, 255, 255, 4,
  5,   6,	7,	 8,	  9,   10,	11,	 12,  13,  14,	15,	 16,  17,  18,	19,	 20,  21,  22,	23,	 24,  25,  26,
  27,  28,	29,	 255, 255, 255, 255, 255, 98,  89,	90,	 91,  92,  93,	94,	 95,  96,  97,	85,	 87,  255, 86,
  99,  84,	58,	 59,  60,  61,	62,	 63,  64,  65,	66,	 67,  68,  69,	104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 255, 255, 255, 255, 255, 255, 255, 255, 83,  71,	255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 51,	 46,  54,  45,	55,	 56,  53,  255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 47,
  49,  48,	52,	 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};


LRESULT CALLBACK input::win_event_handler(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam) {
	core::os::surface* surface = reinterpret_cast<core::os::surface*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if(surface != nullptr) {
		auto& input = surface->input();
		switch(uMsg) {
		case WM_MOUSEACTIVATE:
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
			switch(wParam) {
			case 0:	   // FALSE or WM_INACTIVE
					   // Pause game, etc
				surface->focus(false);
				break;
			case 1:	   // TRUE or WM_ACTIVE or WM_CLICKACTIVE
			case 2:
				// Reactivate, reset lost device, etc
				surface->focus(true);
				break;
			}
			break;
		case WM_CLOSE:
			surface->terminate();
			return 0;
		case WM_SIZE: {
			// we get here if the window has changed size, we should rebuild most
			// of our window resources before rendering to this window again.
			// ( no need for this because our window sizing by hand is disabled )

			surface->resize(LOWORD(lParam), HIWORD(lParam));
		} break;
		default:
			break;
		}
		if(!surface->focused())
			return DefWindowProc(hWnd, uMsg, wParam, lParam);

		switch(uMsg) {
		case WM_INPUT: {
			UINT dwSize;

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
			LPBYTE lpb = new BYTE[dwSize];
			if(lpb == NULL) {
				return 0;
			}

			if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
				OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

			RAWINPUT* raw = (RAWINPUT*)lpb;
			if(raw->header.dwType == RIM_TYPEMOUSE) {
				POINT pt;
				pt.x = surface->data().width() / 2;
				pt.y = surface->data().height() / 2;
				ScreenToClient(hWnd, &pt);
				mouse_delta mDelta;
				mDelta.x = raw->data.mouse.lLastX;
				mDelta.y = raw->data.mouse.lLastY;
				GetCursorPos(&pt);
				ScreenToClient(hWnd, &pt);
				mouse_coordinate m_coords;
				m_coords.x		= pt.x;
				m_coords.y		= pt.y;
				m_coords.width	= surface->data().width();
				m_coords.height = surface->data().height();
				input.m_OnMouseMoveDelta(mDelta);
				input.m_OnMouseMoveCoordinates(m_coords);

				switch(raw->data.mouse.usButtonFlags) {
				case RI_MOUSE_BUTTON_1_DOWN:
					input.m_OnMousePressed(mousecode::BTN_1);
					input.m_HeldMouseButtons.emplace_back(mousecode::BTN_1);
					break;
				case RI_MOUSE_BUTTON_1_UP:
					input.m_OnMouseReleased(mousecode::BTN_1);
					input.m_HeldMouseButtons.erase(std::remove(std::begin(input.m_HeldMouseButtons),
															   std::end(input.m_HeldMouseButtons),
															   mousecode::BTN_1),
												   std::end(input.m_HeldMouseButtons));
					break;
				case RI_MOUSE_BUTTON_2_DOWN:
					input.m_OnMousePressed(mousecode::BTN_2);
					input.m_HeldMouseButtons.emplace_back(mousecode::BTN_2);
					break;
				case RI_MOUSE_BUTTON_2_UP:
					input.m_OnMouseReleased(mousecode::BTN_2);
					input.m_HeldMouseButtons.erase(std::remove(std::begin(input.m_HeldMouseButtons),
															   std::end(input.m_HeldMouseButtons),
															   mousecode::BTN_2),
												   std::end(input.m_HeldMouseButtons));
					break;
				case RI_MOUSE_BUTTON_3_DOWN:
					input.m_OnMousePressed(mousecode::BTN_3);
					input.m_HeldMouseButtons.emplace_back(mousecode::BTN_3);
					break;
				case RI_MOUSE_BUTTON_3_UP:
					input.m_OnMouseReleased(mousecode::BTN_3);
					input.m_HeldMouseButtons.erase(std::remove(std::begin(input.m_HeldMouseButtons),
															   std::end(input.m_HeldMouseButtons),
															   mousecode::BTN_3),
												   std::end(input.m_HeldMouseButtons));
					break;
				case RI_MOUSE_BUTTON_4_DOWN:
					input.m_OnMousePressed(mousecode::BTN_4);
					input.m_HeldMouseButtons.emplace_back(mousecode::BTN_4);
					break;
				case RI_MOUSE_BUTTON_4_UP:
					input.m_OnMouseReleased(mousecode::BTN_4);
					input.m_HeldMouseButtons.erase(std::remove(std::begin(input.m_HeldMouseButtons),
															   std::end(input.m_HeldMouseButtons),
															   mousecode::BTN_4),
												   std::end(input.m_HeldMouseButtons));
					break;
				case RI_MOUSE_BUTTON_5_DOWN:
					input.m_OnMousePressed(mousecode::BTN_5);
					input.m_HeldMouseButtons.emplace_back(mousecode::BTN_5);
					break;
				case RI_MOUSE_BUTTON_5_UP:
					input.m_OnMouseReleased(mousecode::BTN_5);
					input.m_HeldMouseButtons.erase(std::remove(std::begin(input.m_HeldMouseButtons),
															   std::end(input.m_HeldMouseButtons),
															   mousecode::BTN_5),
												   std::end(input.m_HeldMouseButtons));
					break;
				case RI_MOUSE_WHEEL:
					scroll_delta delta;
					delta.y = (short)LOWORD(raw->data.mouse.usButtonData);
					input.m_OnScroll(delta);
					break;
				}
			} else if(raw->header.dwType == RIM_TYPEKEYBOARD) {
				if(raw->data.keyboard.Flags == RI_KEY_MAKE || raw->data.keyboard.Flags == (RI_KEY_MAKE | RI_KEY_E0) ||
				   raw->data.keyboard.Flags == (RI_KEY_MAKE | RI_KEY_E1)) {
					input.m_OnKeyPressed.Execute((keycode)WIN_NATIVE_TO_HID[raw->data.keyboard.VKey]);
					input.m_HeldKeys.emplace_back((keycode)WIN_NATIVE_TO_HID[raw->data.keyboard.VKey]);
				}
				if(raw->data.keyboard.Flags == RI_KEY_BREAK || raw->data.keyboard.Flags == (RI_KEY_BREAK | RI_KEY_E0) ||
				   raw->data.keyboard.Flags == (RI_KEY_BREAK | RI_KEY_E1)) {
					surface->input().m_OnKeyReleased.Execute((keycode)WIN_NATIVE_TO_HID[raw->data.keyboard.VKey]);
					input.m_HeldKeys.erase(std::remove(std::begin(input.m_HeldKeys),
													   std::end(input.m_HeldKeys),
													   (keycode)WIN_NATIVE_TO_HID[raw->data.keyboard.VKey]),
										   std::end(input.m_HeldKeys));
				}
			}
			delete[] lpb;
			return 0;
		} break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void input::tick() {
	PROFILE_SCOPE(core::profiler)
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	for(const auto& key : m_HeldKeys) m_OnKeyHeld.Execute(key);
	for(const auto& key : m_HeldMouseButtons) m_OnMouseHeld.Execute(key);
}
#endif
