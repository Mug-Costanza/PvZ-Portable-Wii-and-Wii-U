#ifndef __WIIPOINTER_H__
#define __WIIPOINTER_H__

// Wiimote IR pointer queries. Kept in its own translation unit: the
// libogc/wiiuse headers clash with engine names like Sexy::Color.
//
// Only Wiimote 1's pointer reaches SDL (and moves its built-in cursor); the
// other Wiimotes' pointers are hidden from it so they can drive their own
// cursors (see __wrap_WPAD_Data and WiiUpdatePlayerCursors).

// Turns the IR pointer on for Wiimotes as they connect. SDL's Wii startup
// code does it once, before main(), which misses Wiimotes that connect
// later (Dolphin's emulated ones connect a moment after boot). Called once
// per main-loop iteration.
void WiiEnablePointers();

// [wii-debug] One line of raw pointer state for Wiimote theChannel.
void WiiDebugPointerState(int theChannel, char* theBuffer, int theSize);

// True while Wiimote theChannel (0-3) points at the screen.
bool WiiPointerOnScreen(int theChannel);

// Where Wiimote theChannel points, as a fraction (0-1) of the screen, scaled
// the same way SDL scales Wiimote 1's. False while it isn't pointing.
bool WiiPointerPosition(int theChannel, float* theX, float* theY);

// How far Wiimote theChannel is tilted, in degrees clockwise (SDL tilts
// Wiimote 1's cursor by the same amount). False while it isn't pointing.
bool WiiPointerAngle(int theChannel, float* theDegrees);

#endif // __WIIPOINTER_H__
