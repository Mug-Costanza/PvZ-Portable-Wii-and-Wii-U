/*
 * Portions of this file are based on the PopCap Games Framework
 * Copyright (C) 2005-2009 PopCap Games, Inc.
 * 
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later AND LicenseRef-PopCap
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include <SDL.h>
#ifdef __wii__
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../wii/WiiPointer.h"
#include "graphics/Graphics.h"
#include "../wii/SDLWiiHandCursor.h"
#include "misc/SexyMatrix.h"
#endif

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"
#include "misc/KeyCodes.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, WasmStartSoftKeyboard, (), {
	var input = document.getElementById('pvz-soft-keyboard');
	if (!input) return;

	if (!Module.wasmSoftKeyboardState) {
		var state = {
			active: false,
			pendingChars: [],
			pendingKeys: [],
			lastValue: ""
		};

		function syncInputValue() {
			if (!state.active) return;

			var nextValue = input.value || "";
			var prefixLen = 0;
			while (prefixLen < state.lastValue.length && prefixLen < nextValue.length
				&& state.lastValue.charCodeAt(prefixLen) === nextValue.charCodeAt(prefixLen)) {
				prefixLen++;
			}

			for (var i = state.lastValue.length; i > prefixLen; --i) {
				state.pendingKeys.push(8);
			}

			for (var j = prefixLen; j < nextValue.length; ++j) {
				var charCode = nextValue.charCodeAt(j);
				if (charCode > 0 && charCode <= 0x7f) {
					state.pendingChars.push(charCode);
				}
			}

			state.lastValue = nextValue;
		}

		input.addEventListener('input', syncInputValue);
		input.addEventListener('keydown', function(event) {
			if (!state.active) return;

			switch (event.key) {
				case 'Enter':
					state.pendingKeys.push(13);
					event.preventDefault();
					break;
				case 'Escape':
					state.pendingKeys.push(27);
					event.preventDefault();
					break;
				case 'Tab':
					state.pendingKeys.push(9);
					event.preventDefault();
					break;
				case 'Delete':
					state.pendingKeys.push(46);
					event.preventDefault();
					break;
				case 'ArrowLeft':
					state.pendingKeys.push(37);
					event.preventDefault();
					break;
				case 'ArrowRight':
					state.pendingKeys.push(39);
					event.preventDefault();
					break;
				case 'Home':
					state.pendingKeys.push(36);
					event.preventDefault();
					break;
				case 'End':
					state.pendingKeys.push(35);
					event.preventDefault();
					break;
			}
		});

		Module.wasmSoftKeyboardState = state;
	}

	var state = Module.wasmSoftKeyboardState;
	state.active = true;
	state.pendingChars.length = 0;
	state.pendingKeys.length = 0;
	state.lastValue = "";
	input.value = "";
	if (typeof input.focus === 'function') {
		input.focus();
	}
	if (typeof input.setSelectionRange === 'function') {
		input.setSelectionRange(0, 0);
	}
});

EM_JS(void, WasmStopSoftKeyboard, (), {
	var input = document.getElementById('pvz-soft-keyboard');
	var state = Module.wasmSoftKeyboardState;
	if (state) {
		state.active = false;
		state.pendingChars.length = 0;
		state.pendingKeys.length = 0;
		state.lastValue = "";
	}
	if (input) {
		input.value = "";
		if (typeof input.blur === 'function') {
			input.blur();
		}
	}
	if (Module.canvas && typeof Module.canvas.focus === 'function') {
		Module.canvas.focus();
	}
});

EM_JS(int, WasmPopSoftKeyboardChar, (), {
	var state = Module.wasmSoftKeyboardState;
	if (!state || state.pendingChars.length === 0) return 0;
	return state.pendingChars.shift();
});

EM_JS(int, WasmPopSoftKeyboardKey, (), {
	var state = Module.wasmSoftKeyboardState;
	if (!state || state.pendingKeys.length === 0) return 0;
	return state.pendingKeys.shift();
});

EM_JS(int, WasmHasSoftKeyboardEvents, (), {
	var state = Module.wasmSoftKeyboardState;
	if (!state) return 0;
	return (state.pendingChars.length + state.pendingKeys.length) > 0 ? 1 : 0;
});
#endif

using namespace Sexy;

// Map SDL_Keycode to internal KeyCode (Windows VK-compatible).
static KeyCode SDLKeyToKeyCode(SDL_Keycode theSDLKey)
{
	if (theSDLKey >= SDLK_a && theSDLKey <= SDLK_z)
		return static_cast<KeyCode>(theSDLKey - SDLK_a + 'A');

	if (theSDLKey >= SDLK_0 && theSDLKey <= SDLK_9)
		return static_cast<KeyCode>(theSDLKey);

	switch (theSDLKey)
	{
		case SDLK_BACKSPACE:    return KEYCODE_BACK;
		case SDLK_TAB:          return KEYCODE_TAB;
		case SDLK_CLEAR:        return KEYCODE_CLEAR;
		case SDLK_RETURN:       return KEYCODE_RETURN;
		case SDLK_ESCAPE:       return KEYCODE_ESCAPE;
		case SDLK_SPACE:        return KEYCODE_SPACE;
		case SDLK_DELETE:       return KEYCODE_DELETE;

		case SDLK_LEFT:         return KEYCODE_LEFT;
		case SDLK_UP:           return KEYCODE_UP;
		case SDLK_RIGHT:        return KEYCODE_RIGHT;
		case SDLK_DOWN:         return KEYCODE_DOWN;

		case SDLK_INSERT:       return KEYCODE_INSERT;
		case SDLK_HOME:         return KEYCODE_HOME;
		case SDLK_END:          return KEYCODE_END;
		case SDLK_PAGEUP:       return KEYCODE_PRIOR;
		case SDLK_PAGEDOWN:     return KEYCODE_NEXT;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:       return KEYCODE_SHIFT;
		case SDLK_LCTRL:
		case SDLK_RCTRL:        return KEYCODE_CONTROL;
		case SDLK_LALT:
		case SDLK_RALT:         return KEYCODE_MENU;
		case SDLK_PAUSE:        return KEYCODE_PAUSE;
		case SDLK_CAPSLOCK:     return KEYCODE_CAPITAL;
		case SDLK_NUMLOCKCLEAR: return KEYCODE_NUMLOCK;
		case SDLK_SCROLLLOCK:   return KEYCODE_SCROLL;

		case SDLK_KP_0:         return KEYCODE_NUMPAD0;
		case SDLK_KP_1:         return KEYCODE_NUMPAD1;
		case SDLK_KP_2:         return KEYCODE_NUMPAD2;
		case SDLK_KP_3:         return KEYCODE_NUMPAD3;
		case SDLK_KP_4:         return KEYCODE_NUMPAD4;
		case SDLK_KP_5:         return KEYCODE_NUMPAD5;
		case SDLK_KP_6:         return KEYCODE_NUMPAD6;
		case SDLK_KP_7:         return KEYCODE_NUMPAD7;
		case SDLK_KP_8:         return KEYCODE_NUMPAD8;
		case SDLK_KP_9:         return KEYCODE_NUMPAD9;
		case SDLK_KP_MULTIPLY:  return KEYCODE_MULTIPLY;
		case SDLK_KP_PLUS:      return KEYCODE_ADD;
		case SDLK_KP_MINUS:     return KEYCODE_SUBTRACT;
		case SDLK_KP_PERIOD:    return KEYCODE_DECIMAL;
		case SDLK_KP_DIVIDE:    return KEYCODE_DIVIDE;
		case SDLK_KP_ENTER:     return KEYCODE_RETURN;

		case SDLK_F1:           return KEYCODE_F1;
		case SDLK_F2:           return KEYCODE_F2;
		case SDLK_F3:           return KEYCODE_F3;
		case SDLK_F4:           return KEYCODE_F4;
		case SDLK_F5:           return KEYCODE_F5;
		case SDLK_F6:           return KEYCODE_F6;
		case SDLK_F7:           return KEYCODE_F7;
		case SDLK_F8:           return KEYCODE_F8;
		case SDLK_F9:           return KEYCODE_F9;
		case SDLK_F10:          return KEYCODE_F10;
		case SDLK_F11:          return KEYCODE_F11;
		case SDLK_F12:          return KEYCODE_F12;

		default:                return KEYCODE_UNKNOWN;
	}
}

// Synthesize a minimal ASCII char stream from keydown so legacy KeyChar hotkeys still work.
static bool SDLSynthesizeAsciiCharFromKeyDown(const SDL_KeyboardEvent& theEvent, char& theChar)
{
	theChar = 0;

	if (SDL_IsTextInputActive())
		return false;

	SDL_Keycode aSym = theEvent.keysym.sym;
	SDL_Keymod aMods = static_cast<SDL_Keymod>(theEvent.keysym.mod);
	const bool aHasCtrl = (aMods & KMOD_CTRL) != 0;
	const bool aHasAlt = (aMods & KMOD_ALT) != 0;
	const bool aHasGui = (aMods & KMOD_GUI) != 0;
	const bool aHasShift = (aMods & KMOD_SHIFT) != 0;

	if (aHasAlt || aHasGui)
		return false;

	if (aSym >= SDLK_a && aSym <= SDLK_z)
	{
		theChar = aHasCtrl
			? static_cast<char>(aSym - SDLK_a + 1)
			: static_cast<char>(aHasShift ? aSym - SDLK_a + 'A' : aSym);
		return true;
	}

	if (aHasCtrl)
		return false;

	switch (aSym)
	{
		case SDLK_KP_1: theChar = '1'; return true;
		case SDLK_KP_2: theChar = '2'; return true;
		case SDLK_KP_3: theChar = '3'; return true;
		case SDLK_KP_4: theChar = '4'; return true;
		case SDLK_KP_5: theChar = '5'; return true;
		case SDLK_KP_6: theChar = '6'; return true;
		case SDLK_KP_7: theChar = '7'; return true;
		case SDLK_KP_8: theChar = '8'; return true;
		case SDLK_KP_9: theChar = '9'; return true;
		case SDLK_KP_0: theChar = '0'; return true;
		case SDLK_KP_PLUS: theChar = '+'; return true;
		case SDLK_KP_MINUS: theChar = '-'; return true;
		case SDLK_KP_MULTIPLY: theChar = '*'; return true;
		case SDLK_KP_DIVIDE: theChar = '/'; return true;
		case SDLK_KP_PERIOD: theChar = '.'; return true;
		case SDLK_KP_EQUALS: theChar = '='; return true;
		case SDLK_1: theChar = aHasShift ? '!' : '1'; return true;
		case SDLK_2: theChar = aHasShift ? '@' : '2'; return true;
		case SDLK_3: theChar = aHasShift ? '#' : '3'; return true;
		case SDLK_4: theChar = aHasShift ? '$' : '4'; return true;
		case SDLK_5: theChar = aHasShift ? '%' : '5'; return true;
		case SDLK_6: theChar = aHasShift ? '^' : '6'; return true;
		case SDLK_7: theChar = aHasShift ? '&' : '7'; return true;
		case SDLK_8: theChar = aHasShift ? '*' : '8'; return true;
		case SDLK_9: theChar = aHasShift ? '(' : '9'; return true;
		case SDLK_0: theChar = aHasShift ? ')' : '0'; return true;
		case SDLK_MINUS: theChar = aHasShift ? '_' : '-'; return true;
		case SDLK_EQUALS: theChar = aHasShift ? '+' : '='; return true;
		case SDLK_LEFTBRACKET: theChar = aHasShift ? '{' : '['; return true;
		case SDLK_RIGHTBRACKET: theChar = aHasShift ? '}' : ']'; return true;
		case SDLK_BACKSLASH: theChar = aHasShift ? '|' : '\\'; return true;
		case SDLK_SEMICOLON: theChar = aHasShift ? ':' : ';'; return true;
		case SDLK_QUOTE: theChar = aHasShift ? '"' : '\''; return true;
		case SDLK_COMMA: theChar = aHasShift ? '<' : ','; return true;
		case SDLK_PERIOD: theChar = aHasShift ? '>' : '.'; return true;
		case SDLK_SLASH: theChar = aHasShift ? '?' : '/'; return true;
		case SDLK_BACKQUOTE: theChar = aHasShift ? '~' : '`'; return true;
		case SDLK_SPACE: theChar = ' '; return true;
		default: return false;
	}
}

void SexyAppBase::InitInput()
{
	SDL_Init(SDL_INIT_EVENTS);
#ifdef __wii__
	// The pointer and A button reach us through SDL's mouse emulation, but
	// the other controller buttons only as joystick events. Controllers are
	// opened as they connect (SDL_JOYDEVICEADDED), since Wiimotes usually
	// pair after launch.
	// SDL reports each Wiimote's tilt as joystick axes by default, which the
	// cursor code would read as sticks: any Wiimote not pointing at the
	// screen would steer its cursor by how it's held. Only the pointer, the
	// d-pad and real sticks (Nunchuk, Classic, GameCube) should move it.
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	// Sticks and d-pads are polled (WiiUpdatePlayerCursors), never read as
	// events. Queuing them anyway costs a whole UpdateAppStep each, and a
	// held stick's jitter produces a steady stream of them. Joystick state
	// still updates while these are ignored, since button events stay on.
	SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
#endif
}

#ifdef __wii__
// Controller support. The IR pointer and the Wiimote's B/A buttons come from
// SDL's mouse emulation (B = left click, A = right click), but only while the
// Wiimote points at the screen; everything else arrives as joystick events.
// We swap SDL's emulated clicks so A selects and B cancels, matching the
// Classic Controller and GameCube pad (see WiiSwapMouseButton).
// Indices follow SDL's ogc joystick driver in its default (non-split) mode:
//   Wiimote (+ expansion): hat 0 = Wiimote or Classic d-pad; axes 0/1 =
//     Nunchuk / Classic left stick, 2/3 = Classic right stick; buttons
//     0 A, 1 B, 5 +, 7 Z, 8 C (Classic A/B share 0/1).
//   GameCube: hat 0 = d-pad; axes 0/1 = stick, 2/3 = C-stick; buttons
//     0 A, 1 B, 7 Start.

static bool WiiIsGameCube(SDL_JoystickID theWhich)
{
	SDL_Joystick* aJoystick = SDL_JoystickFromInstanceID(theWhich);
	const char* aName = aJoystick ? SDL_JoystickName(aJoystick) : nullptr;
	return aName != nullptr && strncmp(aName, "Gamecube", 8) == 0;
}

static bool WiiHasClassic(SDL_JoystickID theWhich)
{
	SDL_Joystick* aJoystick = SDL_JoystickFromInstanceID(theWhich);
	const char* aName = aJoystick ? SDL_JoystickName(aJoystick) : nullptr;
	return aName != nullptr && strstr(aName, "Classic") != nullptr;
}

// SDL names Wiimotes "Wiimote <channel>" (plus " + Nunchuk"/" + Classic"
// when one's attached); returns the channel, or -1 for other controllers.
static int WiiRemoteChannel(SDL_Joystick* theJoystick)
{
	const char* aName = theJoystick ? SDL_JoystickName(theJoystick) : nullptr;
	int aChannel;
	if (aName == nullptr || sscanf(aName, "Wiimote %d", &aChannel) != 1)
		return -1;
	return aChannel;
}

// Co-op: each controller belongs to the player matching its Wiimote channel
// or GameCube port. Player 1 is SDL's own mouse, drawn with SDL's built-in
// cursor; players 2-4 get cursors of their own (WiiDrawPlayerCursors). The
// game itself has a single mouse, which follows the active player; anyone
// else's click makes them the active player, unless the active player is
// holding something (a seed packet, the shovel, ...), in which case the
// click only collects sun and coins (SexyAppBase::WiiCoopAssistClick).
static constexpr int kWiiMaxPlayers = 4;

// After this long without input, the active player stops holding on to the
// game's mouse even while holding something (or with a press whose release
// never came, e.g. after a disconnect), so nobody gets stuck.
static constexpr uint32_t kWiiActivePlayerIdleMs = 15000;

struct WiiPlayer
{
	float		mX = -1, mY = -1;		// window coordinates (players 2-4); mX < 0 until placed
	uint32_t	mLastInput = 0;			// SDL_GetTicks() of the last move or click
	Uint32		mButtonsToGame = 0;		// SDL_BUTTON() masks of presses that reached the game
	bool		mConnected = false;		// has a controller (players 2-4)
};

static WiiPlayer gWiiPlayers[kWiiMaxPlayers];
static int gWiiActivePlayer = 0;

// [wii-debug] pointer diagnosis: motion events per player, [dropped, delivered]
static unsigned gWiiDebugMotion[kWiiMaxPlayers][2];

// Tags the mouse events we synthesize, so they aren't mistaken for SDL's own
// Wiimote mouse emulation (whose `which` is the Wiimote channel, 0-3).
static constexpr Uint32 kWiiSyntheticMouseID = 0x57494950;	// player 1's controller clicks
static constexpr Uint32 kWiiPlayerMouseID = 0x57494960;		// + player index, players 2-4

static int WiiMousePlayer(Uint32 theWhich)
{
	if (theWhich >= kWiiPlayerMouseID && theWhich < kWiiPlayerMouseID + kWiiMaxPlayers)
		return theWhich - kWiiPlayerMouseID;
	return 0; // SDL's mouse: Wiimote 1's pointer and the warps that move it
}

// The player a controller belongs to: its Wiimote channel or GameCube port.
static int WiiControllerPlayer(SDL_JoystickID theWhich)
{
	SDL_Joystick* aJoystick = SDL_JoystickFromInstanceID(theWhich);
	const char* aName = aJoystick ? SDL_JoystickName(aJoystick) : nullptr;
	int aChannel;
	if (aName == nullptr || (sscanf(aName, "Wiimote %d", &aChannel) != 1 && sscanf(aName, "Gamecube %d", &aChannel) != 1))
		return 0;
	return std::clamp(aChannel, 0, kWiiMaxPlayers - 1);
}

// SDL's Wiimote emulation clicks left on B and right on A; swap so A selects.
static Uint8 WiiSwapMouseButton(const SDL_MouseButtonEvent& theEvent)
{
	if (theEvent.which >= kWiiMaxPlayers) // one of ours
		return theEvent.button;
	if (theEvent.button == SDL_BUTTON_LEFT)
		return SDL_BUTTON_RIGHT;
	if (theEvent.button == SDL_BUTTON_RIGHT)
		return SDL_BUTTON_LEFT;
	return theEvent.button;
}

// Places a player 2-4 cursor in the middle of the screen the first time it's
// needed.
static void WiiPlacePlayerCursor(WiiPlayer& thePlayer, SDL_Window* theWindow)
{
	if (thePlayer.mX >= 0 || theWindow == nullptr)
		return;
	int aW, aH;
	SDL_GetWindowSize(theWindow, &aW, &aH);
	thePlayer.mX = aW / 2.0f;
	thePlayer.mY = aH / 2.0f;
}

static void WiiSendMouseButton(int thePlayer, bool theDown, Uint8 theButton)
{
	SDL_Window* aWindow = SDL_GetMouseFocus();
	int x, y;
	Uint32 aWhich;
	if (thePlayer == 0)
	{
		SDL_GetMouseState(&x, &y);
		aWhich = kWiiSyntheticMouseID;
	}
	else
	{
		WiiPlayer& aPlayer = gWiiPlayers[thePlayer];
		WiiPlacePlayerCursor(aPlayer, aWindow);
		x = (int)aPlayer.mX;
		y = (int)aPlayer.mY;
		aWhich = kWiiPlayerMouseID + thePlayer;
	}
	SDL_Event anEvent = {};
	anEvent.type = theDown ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	anEvent.button.windowID = aWindow ? SDL_GetWindowID(aWindow) : 0;
	anEvent.button.which = aWhich;
	anEvent.button.button = theButton;
	anEvent.button.state = theDown ? SDL_PRESSED : SDL_RELEASED;
	anEvent.button.clicks = 1;
	anEvent.button.x = x;
	anEvent.button.y = y;
	SDL_PushEvent(&anEvent);
}

enum class WiiButtonAction { None, Menu, LeftClick, RightClick };

static WiiButtonAction WiiButtonToAction(const SDL_JoyButtonEvent& theEvent)
{
	if (WiiIsGameCube(theEvent.which))
	{
		switch (theEvent.button)
		{
		case 0: return WiiButtonAction::LeftClick;	// A
		case 1: return WiiButtonAction::RightClick;	// B
		case 7: return WiiButtonAction::Menu;		// Start
		default: return WiiButtonAction::None;
		}
	}
	switch (theEvent.button)
	{
	case 5: return WiiButtonAction::Menu;			// +
	case 3: return WiiButtonAction::LeftClick;		// 2 (sideways: the main button)
	case 2: return WiiButtonAction::RightClick;		// 1 (sideways: cancel)
	case 7: return WiiButtonAction::LeftClick;		// Nunchuk Z
	case 8: return WiiButtonAction::RightClick;		// Nunchuk C
	case 0:
	case 1:
		// A selects, B cancels, on both the Classic and the Wiimote itself.
		// Wiimote 1's own A/B are handled by SDL while it points (swapped
		// in WiiSwapMouseButton); handle them here otherwise. SDL never sees
		// the other Wiimotes' pointers, so theirs always come here.
		if (!WiiHasClassic(theEvent.which) && WiiControllerPlayer(theEvent.which) == 0 && WiiPointerOnScreen(0))
			return WiiButtonAction::None;
		return theEvent.button == 0 ? WiiButtonAction::LeftClick : WiiButtonAction::RightClick;
	default:
		return WiiButtonAction::None;
	}
}

// Moves each player's cursor with their d-pad and sticks while their Wiimote
// isn't pointing at the screen, and follows the pointers of Wiimotes 2-4
// (SDL follows Wiimote 1's itself). Player 1's cursor is SDL's mouse, so it's
// warped; players 2-4 send motion events of their own, which go through the
// same routing as SDL's (WiiRoutePlayerMouseEvent).
// Called once per main-loop iteration.
void WiiUpdatePlayerCursors()
{
	WiiEnablePointers();

	static uint32_t sLastTick = SDL_GetTicks();
	static float sX = -1, sY = -1; // player 1's position while stick-driven
	uint32_t aTick = SDL_GetTicks();
	float aSeconds = std::min((aTick - sLastTick) / 1000.0f, 0.1f);
	sLastTick = aTick;

	SDL_Window* aWindow = SDL_GetMouseFocus();

	// [wii-debug] pointer diagnosis, once a second
	static uint32_t sLastReport = 0;
	if (aTick - sLastReport >= 1000)
	{
		sLastReport = aTick;
		int aMouseX, aMouseY;
		Uint32 aMouseButtons = SDL_GetMouseState(&aMouseX, &aMouseY);
		printf("[wii-debug] coop: focus=%d active=%d sdl mouse=%d,%d btn=%x joysticks=%d\n", aWindow != nullptr, gWiiActivePlayer,
			aMouseX, aMouseY, (unsigned)aMouseButtons, SDL_NumJoysticks());
		for (int i = 0; i < kWiiMaxPlayers; i++)
		{
			float aFX = -1, aFY = -1, anAngle = 0;
			bool aPointing = WiiPointerPosition(i, &aFX, &aFY);
			WiiPointerAngle(i, &anAngle);
			const WiiPlayer& aPlayer = gWiiPlayers[i];
			printf("[wii-debug] coop:   p%d ir=%d (%.2f,%.2f) pos=%.0f,%.0f conn=%d btns=%x motion delivered=%u dropped=%u\n", i + 1, aPointing,
				aFX, aFY, aPlayer.mX, aPlayer.mY, aPlayer.mConnected, (unsigned)aPlayer.mButtonsToGame,
				gWiiDebugMotion[i][1], gWiiDebugMotion[i][0]);
			gWiiDebugMotion[i][0] = gWiiDebugMotion[i][1] = 0;
			char aState[160];
			WiiDebugPointerState(i, aState, sizeof(aState));
			printf("[wii-debug] coop:     wpad %s\n", aState);
		}
	}

	if (aWindow == nullptr)
	{
		sX = -1; // re-sync from the real position next time
		return;
	}

	constexpr int kDeadZone = 8000;
	constexpr float kStickSpeed = 700.0f;	// px/s at full deflection
	constexpr float kPadSpeed = 350.0f;		// px/s for the d-pad
	float aDX[kWiiMaxPlayers] = {}, aDY[kWiiMaxPlayers] = {};
	bool aConnected[kWiiMaxPlayers] = {};
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		// Opened on SDL_JOYDEVICEADDED; skip any that aren't open yet.
		SDL_Joystick* aJoystick = SDL_JoystickFromInstanceID(SDL_JoystickGetDeviceInstanceID(i));
		if (aJoystick == nullptr)
			continue;
		int aPlayer = WiiControllerPlayer(SDL_JoystickInstanceID(aJoystick));
		aConnected[aPlayer] = true;

		if (SDL_JoystickNumHats(aJoystick) > 0)
		{
			Uint8 aHat = SDL_JoystickGetHat(aJoystick, 0);
			if (WiiRemoteChannel(aJoystick) >= 0 && !WiiHasClassic(SDL_JoystickInstanceID(aJoystick)))
			{
				// The Wiimote's d-pad is always read as held sideways, d-pad
				// on the left: its own up arrow points left (same rotation SDL
				// applies with SDL_WII_JOYSTICK_SIDEWAYS). The pointer works
				// the same whichever way it's held. Hat 0 is the Classic
				// Controller's d-pad when one's attached, so that's left as is.
				Uint8 aRotated = 0;
				if (aHat & SDL_HAT_UP) aRotated |= SDL_HAT_LEFT;
				if (aHat & SDL_HAT_DOWN) aRotated |= SDL_HAT_RIGHT;
				if (aHat & SDL_HAT_LEFT) aRotated |= SDL_HAT_DOWN;
				if (aHat & SDL_HAT_RIGHT) aRotated |= SDL_HAT_UP;
				aHat = aRotated;
			}
			if (aHat & SDL_HAT_LEFT) aDX[aPlayer] -= kPadSpeed;
			if (aHat & SDL_HAT_RIGHT) aDX[aPlayer] += kPadSpeed;
			if (aHat & SDL_HAT_UP) aDY[aPlayer] -= kPadSpeed;
			if (aHat & SDL_HAT_DOWN) aDY[aPlayer] += kPadSpeed;
		}
		for (int aStick = 0; aStick < 2 && aStick * 2 + 1 < SDL_JoystickNumAxes(aJoystick); aStick++)
		{
			int aAX = SDL_JoystickGetAxis(aJoystick, aStick * 2);
			int aAY = SDL_JoystickGetAxis(aJoystick, aStick * 2 + 1);
			// Squared response: small deflections give fine control.
			auto aCurve = [](int v) {
				if (std::abs(v) < kDeadZone) return 0.0f;
				float f = (std::abs(v) - kDeadZone) / float(32767 - kDeadZone);
				return (v < 0 ? -1.0f : 1.0f) * std::min(f, 1.0f) * std::min(f, 1.0f);
			};
			aDX[aPlayer] += aCurve(aAX) * kStickSpeed;
			aDY[aPlayer] += aCurve(aAY) * kStickSpeed;
		}
	}

	int aW, aH;
	SDL_GetWindowSize(aWindow, &aW, &aH);

	// Player 1: SDL's mouse. Its pointer is SDL's job.
	if (WiiPointerOnScreen(0))
		sX = -1;
	else if (aDX[0] != 0 || aDY[0] != 0)
	{
		if (sX < 0)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			sX = (float)x;
			sY = (float)y;
		}
		sX = std::clamp(sX + aDX[0] * aSeconds, 0.0f, (float)(aW - 1));
		sY = std::clamp(sY + aDY[0] * aSeconds, 0.0f, (float)(aH - 1));
		SDL_WarpMouseInWindow(aWindow, (int)sX, (int)sY);
	}

	// Players 2-4.
	for (int aPlayerIndex = 1; aPlayerIndex < kWiiMaxPlayers; aPlayerIndex++)
	{
		WiiPlayer& aPlayer = gWiiPlayers[aPlayerIndex];
		aPlayer.mConnected = aConnected[aPlayerIndex];
		float aOldX = aPlayer.mX, aOldY = aPlayer.mY;
		float aFX, aFY;
		if (WiiPointerPosition(aPlayerIndex, &aFX, &aFY))
		{
			aPlayer.mX = aFX * aW;
			aPlayer.mY = aFY * aH;
		}
		else if (aDX[aPlayerIndex] != 0 || aDY[aPlayerIndex] != 0)
		{
			WiiPlacePlayerCursor(aPlayer, aWindow);
			aPlayer.mX += aDX[aPlayerIndex] * aSeconds;
			aPlayer.mY += aDY[aPlayerIndex] * aSeconds;
		}
		else
			continue;

		aPlayer.mX = std::clamp(aPlayer.mX, 0.0f, (float)(aW - 1));
		aPlayer.mY = std::clamp(aPlayer.mY, 0.0f, (float)(aH - 1));
		if ((int)aPlayer.mX == (int)aOldX && (int)aPlayer.mY == (int)aOldY)
			continue;

		SDL_Event anEvent = {};
		anEvent.type = SDL_MOUSEMOTION;
		anEvent.motion.windowID = SDL_GetWindowID(aWindow);
		anEvent.motion.which = kWiiPlayerMouseID + aPlayerIndex;
		anEvent.motion.x = (int)aPlayer.mX;
		anEvent.motion.y = (int)aPlayer.mY;
		anEvent.motion.xrel = (int)aPlayer.mX - (int)aOldX;
		anEvent.motion.yrel = (int)aPlayer.mY - (int)aOldY;
		SDL_PushEvent(&anEvent);
	}
}

// Decides whether a mouse event from any player reaches the game (see the
// co-op notes at kWiiMaxPlayers). Returns false to drop it.
static bool WiiRoutePlayerMouseEvent(SexyAppBase* theApp, const SDL_Event& theEvent)
{
	uint32_t aNow = SDL_GetTicks();
	switch (theEvent.type)
	{
	case SDL_MOUSEMOTION:
	{
		int aPlayerIndex = WiiMousePlayer(theEvent.motion.which);
		gWiiPlayers[aPlayerIndex].mLastInput = aNow;
		// [wii-debug] pointer diagnosis: motion per player, delivered or dropped
		gWiiDebugMotion[aPlayerIndex][aPlayerIndex == gWiiActivePlayer]++;
		return aPlayerIndex == gWiiActivePlayer;
	}

	case SDL_MOUSEBUTTONDOWN:
	{
		int aPlayerIndex = WiiMousePlayer(theEvent.button.which);
		WiiPlayer& aPlayer = gWiiPlayers[aPlayerIndex];
		aPlayer.mLastInput = aNow;
		Uint32 aButton = SDL_BUTTON(theEvent.button.button);
		if (aPlayerIndex != gWiiActivePlayer)
		{
			const WiiPlayer& anActive = gWiiPlayers[gWiiActivePlayer];
			bool anActiveBusy = aNow - anActive.mLastInput < kWiiActivePlayerIdleMs &&
				(anActive.mButtonsToGame != 0 || !theApp->WiiCoopCanChangePlayer());
			if (anActiveBusy)
			{
				int x = theEvent.button.x;
				int y = theEvent.button.y;
				theApp->mWidgetManager->RemapMouse(x, y);
				theApp->WiiCoopAssistClick(x, y);
				return false;
			}
			// The button handler moves the game's mouse to this click first.
			gWiiPlayers[gWiiActivePlayer].mButtonsToGame = 0;
			printf("[wii-debug] coop: active player %d -> %d (button %d, which 0x%x, at %d,%d)\n", gWiiActivePlayer, aPlayerIndex,
				theEvent.button.button, (unsigned)theEvent.button.which, theEvent.button.x, theEvent.button.y);
			gWiiActivePlayer = aPlayerIndex;
			theApp->WiiRefreshCursor(); // player 1's may no longer be hidden (WiiPlayer1CursorNum)
		}
		aPlayer.mButtonsToGame |= aButton;
		return true;
	}

	case SDL_MOUSEBUTTONUP:
	{
		WiiPlayer& aPlayer = gWiiPlayers[WiiMousePlayer(theEvent.button.which)];
		Uint32 aButton = SDL_BUTTON(theEvent.button.button);
		if (!(aPlayer.mButtonsToGame & aButton))
			return false; // its press was an assist click, or went nowhere
		aPlayer.mButtonsToGame &= ~aButton;
		return true;
	}
	}
	return true;
}

// Everyone's cursor is the hand, like the Wii's own pointer. The one
// exception is the active player's when the game hides it (the Whack a
// Zombie hammer is drawn there instead).
static bool WiiPlayerCursorHidden(SexyAppBase* theApp, int thePlayerIndex)
{
	return thePlayerIndex == gWiiActivePlayer && theApp->mCursorNum == CURSOR_NONE;
}

// Player 1's cursor is SDL's, set from the game's cursor in EnforceCursor.
int WiiPlayer1CursorNum(SexyAppBase* theApp, int theCursorNum)
{
	(void)theCursorNum;
	return WiiPlayerCursorHidden(theApp, 0) ? CURSOR_NONE : CURSOR_HAND;
}

// Players 2-4's cursors: SDL's hand (player 1's cursor), tinted with the
// player's color. Multiplying keeps the black outline and turns the white
// hand into the color.
static MemoryImage* WiiCreatePlayerCursor(const SDLWiiCursor& theCursor, int thePlayerIndex)
{
	static const uint8_t kTints[kWiiMaxPlayers][3] = {
		{ 255, 255, 255 },	// player 1 (SDL draws it untinted)
		{ 255, 110, 110 },	// player 2: red
		{ 110, 220, 120 },	// player 3: green
		{ 255, 215, 80 },	// player 4: yellow
	};
	const uint8_t* aTint = kTints[thePlayerIndex];
	MemoryImage* anImage = new MemoryImage();
	anImage->Create(theCursor.mWidth, theCursor.mHeight);
	uint32_t* aBits = anImage->GetBits();
	for (int i = 0; i < theCursor.mWidth * theCursor.mHeight; i++)
	{
		const uint8_t* aPixel = &theCursor.mPixels[i * 4];
		aBits[i] = ((uint32_t)aPixel[3] << 24) | ((aPixel[0] * aTint[0] / 255) << 16) |
			((aPixel[1] * aTint[1] / 255) << 8) | (aPixel[2] * aTint[2] / 255);
	}
	anImage->BitsChanged();
	return anImage;
}

// Draws players 2-4's cursors over the frame (player 1's is SDL's). Shown
// once the player has moved it, while their controller is connected. Like
// SDL's, they're drawn at the cursor's own size in screen pixels and tilt
// with the Wiimote.
void WiiDrawPlayerCursors(Graphics& g, SexyAppBase* theApp)
{
	static MemoryImage* sImages[kWiiMaxPlayers] = {};
	const SDLWiiCursor& aCursor = kSDLWiiHand;
	WidgetManager* aWidgetManager = theApp->mWidgetManager;
	// Game pixels per screen pixel, to undo the viewport's scaling.
	float aScaleX = aWidgetManager->mMouseDestRect.mWidth / (float)aWidgetManager->mMouseSourceRect.mWidth;
	float aScaleY = aWidgetManager->mMouseDestRect.mHeight / (float)aWidgetManager->mMouseSourceRect.mHeight;
	for (int aPlayerIndex = 1; aPlayerIndex < kWiiMaxPlayers; aPlayerIndex++)
	{
		const WiiPlayer& aPlayer = gWiiPlayers[aPlayerIndex];
		if (!aPlayer.mConnected || aPlayer.mX < 0 || WiiPlayerCursorHidden(theApp, aPlayerIndex))
			continue;
		MemoryImage*& anImage = sImages[aPlayerIndex];
		if (anImage == nullptr)
			anImage = WiiCreatePlayerCursor(aCursor, aPlayerIndex);
		int x = (int)aPlayer.mX;
		int y = (int)aPlayer.mY;
		aWidgetManager->RemapMouse(x, y);

		float anAngle = 0;
		WiiPointerAngle(aPlayerIndex, &anAngle);
		// The matrix blit centers the image on the origin: move the hotspot
		// there, tilt (Wiimote angles are clockwise, Sexy's counterclockwise),
		// then scale to screen pixels.
		SexyTransform2D aTransform;
		aTransform.Translate(aCursor.mWidth / 2.0f - aCursor.mHotX, aCursor.mHeight / 2.0f - aCursor.mHotY);
		aTransform.RotateRad(-anAngle * (float)M_PI / 180.0f);
		aTransform.Scale(aScaleX, aScaleY);
		g.DrawImageMatrix(anImage, aTransform, (float)x, (float)y);
	}
}
#endif

bool SexyAppBase::StartTextInput(std::string& theInput)
{
	(void)theInput;
	SDL_StartTextInput();

#ifdef __EMSCRIPTEN__
	WasmStartSoftKeyboard();
#endif

	return false;
}

void SexyAppBase::StopTextInput()
{
	SDL_StopTextInput();

#ifdef __EMSCRIPTEN__
	WasmStopSoftKeyboard();
#endif
}

bool SexyAppBase::ProcessDeferredMessages(bool singleMessage)
{
#ifdef __EMSCRIPTEN__
	int aPendingKey = WasmPopSoftKeyboardKey();
	if (aPendingKey != 0)
	{
		mLastUserInputTick = mLastTimerTime;
		mWidgetManager->KeyDown(static_cast<KeyCode>(aPendingKey));
		return WasmHasSoftKeyboardEvents() || SDL_HasEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
	}

	int aPendingChar = WasmPopSoftKeyboardChar();
	if (aPendingChar != 0)
	{
		mLastUserInputTick = mLastTimerTime;
		mWidgetManager->KeyChar(static_cast<char>(aPendingChar));
		return WasmHasSoftKeyboardEvents() || SDL_HasEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
	}
#endif

	SDL_Event event;
	if (SDL_PollEvent(&event))
	{
#ifdef __wii__
		if (!WiiRoutePlayerMouseEvent(this, event))
			return SDL_HasEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
#endif
		switch(event.type)
		{
			case SDL_QUIT:
				CloseRequestAsync();
				break;

			case SDL_APP_WILLENTERBACKGROUND:
				mMinimized = true;
				RehupFocus();
				break;

			case SDL_APP_DIDENTERFOREGROUND:
				mMinimized = false;
				RehupFocus();
				mWidgetManager->MarkAllDirty();
				break;

			case SDL_WINDOWEVENT:
				switch(event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						CloseRequestAsync();
						break;

					case SDL_WINDOWEVENT_RESIZED:
						mGLInterface->UpdateViewport();
						mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
						mWidgetManager->MarkAllDirty();
						break;

					case SDL_WINDOWEVENT_MINIMIZED:
						mMinimized = true;
						RehupFocus();
						break;

					case SDL_WINDOWEVENT_RESTORED:
						mMinimized = false;
						RehupFocus();
						mWidgetManager->MarkAllDirty();
						break;

					case SDL_WINDOWEVENT_FOCUS_GAINED:
					case SDL_WINDOWEVENT_FOCUS_LOST:
						mActive = event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED;
						RehupFocus();
						break;
				}
				break;

			case SDL_MOUSEWHEEL:
			{
				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->MouseWheel(event.wheel.y);
				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (!mMouseIn)
					mMouseIn = true;

				int x = event.motion.x;
				int y = event.motion.y;
				mWidgetManager->RemapMouse(x, y);

				mLastUserInputTick = mLastTimerTime;
				
				mWidgetManager->MouseMove(x, y);
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			{
				if (!mMouseIn)
					mMouseIn = true;

				int x = event.button.x;
				int y = event.button.y;
				mWidgetManager->RemapMouse(x, y);

				mLastUserInputTick = mLastTimerTime;
				
				mWidgetManager->MouseMove(x, y);
#ifdef __wii__
				event.button.button = WiiSwapMouseButton(event.button);
#endif
				int btn =
					(event.button.button == SDL_BUTTON_LEFT) ? 1 :
					(event.button.button == SDL_BUTTON_RIGHT) ? -1 :
					3;
				if (event.button.clicks == 2)
					btn = (event.button.button == SDL_BUTTON_LEFT) ? 2 : -2;

				mWidgetManager->MouseDown(x, y, btn);
				break;
			}

			case SDL_MOUSEBUTTONUP:
			{
				if (!mMouseIn)
					mMouseIn = true;

				int x = event.button.x;
				int y = event.button.y;
				mWidgetManager->RemapMouse(x, y);

				mLastUserInputTick = mLastTimerTime;
				
				mWidgetManager->MouseMove(x, y);
#ifdef __wii__
				event.button.button = WiiSwapMouseButton(event.button);
#endif
				int btn =
					(event.button.button == SDL_BUTTON_LEFT) ? 1 :
					(event.button.button == SDL_BUTTON_RIGHT) ? -1 :
					3;

				mWidgetManager->MouseUp(x, y, btn);
				break;
			}

			case SDL_KEYDOWN:
			{
				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->KeyDown(SDLKeyToKeyCode(event.key.keysym.sym));

				char aSynthesizedChar = 0;
				if (SDLSynthesizeAsciiCharFromKeyDown(event.key, aSynthesizedChar))
					mWidgetManager->KeyChar(aSynthesizedChar);

				break;
			}

			case SDL_KEYUP:
				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->KeyUp(SDLKeyToKeyCode(event.key.keysym.sym));
				break;

#ifdef __wii__
			case SDL_JOYDEVICEADDED:
				SDL_JoystickOpen(event.jdevice.which);
				break;

			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			{
				bool isDown = event.type == SDL_JOYBUTTONDOWN;
				switch (WiiButtonToAction(event.jbutton))
				{
				case WiiButtonAction::Menu:
					mLastUserInputTick = mLastTimerTime;
					if (isDown)
						mWidgetManager->KeyDown(KEYCODE_ESCAPE);
					else
						mWidgetManager->KeyUp(KEYCODE_ESCAPE);
					break;
				case WiiButtonAction::LeftClick:
					WiiSendMouseButton(WiiControllerPlayer(event.jbutton.which), isDown, SDL_BUTTON_LEFT);
					break;
				case WiiButtonAction::RightClick:
					WiiSendMouseButton(WiiControllerPlayer(event.jbutton.which), isDown, SDL_BUTTON_RIGHT);
					break;
				case WiiButtonAction::None:
					break;
				}
				break;
			}
#endif

			case SDL_TEXTINPUT:
				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->KeyChar((char)event.text.text[0]);
				break;
		}
	}

	return SDL_HasEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}
