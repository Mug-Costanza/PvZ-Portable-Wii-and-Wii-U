#include <cstdio>
#include <ogc/lwp_queue.h>
#include <wiiuse/wpad.h>

#include "WiiPointer.h"

// libogc 3.1.0 gives each Wiimote slot an 18 KB command buffer (256
// commands of 72 bytes) from its kernel workspace heap, which is only 64 KB.
// Five slots don't fit, so from the fourth Wiimote on the allocation fails,
// its command queue is left zeroed, and libogc dereferences null as soon as
// it sends that Wiimote a command (a fourth player could never connect).
// The build links with --wrap=wiiuse_init_cmd_queue (CMakeLists.txt), so
// wiiuse_init's calls land here instead; give every slot a buffer of its own.
extern "C" void __wrap_wiiuse_init_cmd_queue(struct wiimote_t* wm)
{
	constexpr int kCommands = 256, kCommandSize = 72;
	static u8 sBuffers[WPAD_MAX_WIIMOTES][kCommands * kCommandSize] ATTRIBUTE_ALIGN(32);
	if (wm->unid < 0 || wm->unid >= WPAD_MAX_WIIMOTES)
		return;
	// cmdq is WCONST (read-only) outside libogc.
	__lwp_queue_initialize(const_cast<lwp_queue*>(&wm->cmdq), sBuffers[wm->unid], kCommands, kCommandSize);
}

// The build links with --wrap=WPAD_Data (CMakeLists.txt), so every call to
// WPAD_Data, SDL's included, lands in __wrap_WPAD_Data; the real one is
// reached as __real_WPAD_Data.
extern "C" WPADData* __real_WPAD_Data(int chan);

// SDL's Wii backend feeds the IR pointer of every Wiimote into its single
// mouse, so with two Wiimotes pointing at the screen its cursor would jump
// between them every frame. Show SDL only Wiimote 1's pointer; players 2-4
// get cursors of their own, read through __real_WPAD_Data below.
extern "C" WPADData* __wrap_WPAD_Data(int chan)
{
	WPADData* aData = __real_WPAD_Data(chan);
	if (chan <= 0 || chan >= WPAD_MAX_WIIMOTES || aData == nullptr)
		return aData;

	static WPADData sHidden[WPAD_MAX_WIIMOTES];
	sHidden[chan] = *aData;
	sHidden[chan].ir.valid = 0;
	return &sHidden[chan];
}

void WiiEnablePointers()
{
	// Do it the moment a Wiimote connects: libogc keeps the format and
	// applies it as it finishes setting the Wiimote up. Setting it once the
	// Wiimote was fully up didn't take (it kept sending buttons only).
	static bool sConnected[WPAD_MAX_WIIMOTES] = {};
	for (int aChannel = 0; aChannel < WPAD_MAX_WIIMOTES; aChannel++)
	{
		u32 aType;
		bool aConnected = WPAD_Probe(aChannel, &aType) == WPAD_ERR_NONE;
		if (aConnected && !sConnected[aChannel])
		{
			// Same settings SDL uses (and scales the pointer by).
			WPAD_SetDataFormat(aChannel, WPAD_FMT_BTNS_ACC_IR);
			WPAD_SetVRes(aChannel, 640, 480);
			printf("[wii-debug] coop: Wiimote %d connected, pointer enabled\n", aChannel + 1);
		}
		sConnected[aChannel] = aConnected;
	}
}

void WiiDebugPointerState(int theChannel, char* theBuffer, int theSize)
{
	u32 aType = 0;
	s32 aProbe = WPAD_Probe(theChannel, &aType);
	WPADData* aData = __real_WPAD_Data(theChannel);
	snprintf(theBuffer, theSize, "probe=%d type=%u data=%x dots=%u raw=%d smooth=%d valid=%d xy=%.0f,%.0f vres=%u,%u",
		(int)aProbe, (unsigned)aType, (unsigned)aData->data_present, (unsigned)aData->ir.num_dots, aData->ir.raw_valid,
		aData->ir.smooth_valid, aData->ir.valid, aData->ir.x, aData->ir.y, aData->ir.vres[0], aData->ir.vres[1]);
}

bool WiiPointerOnScreen(int theChannel)
{
	if (theChannel < 0 || theChannel >= WPAD_MAX_WIIMOTES)
		return false;
	return __real_WPAD_Data(theChannel)->ir.valid;
}

bool WiiPointerPosition(int theChannel, float* theX, float* theY)
{
	if (!WiiPointerOnScreen(theChannel))
		return false;
	// SDL maps ir.x/ir.y to the window as x * width / 640, y * height / 480.
	WPADData* aData = __real_WPAD_Data(theChannel);
	*theX = aData->ir.x / 640.0f;
	*theY = aData->ir.y / 480.0f;
	return true;
}

bool WiiPointerAngle(int theChannel, float* theDegrees)
{
	if (!WiiPointerOnScreen(theChannel))
		return false;
	*theDegrees = __real_WPAD_Data(theChannel)->ir.angle;
	return true;
}
