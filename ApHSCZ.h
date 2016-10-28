// ApHSCZ.h

#ifndef _APHSC_h
#define _APHSC_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class ApHSC {
public:
	static void init();
	static void queryBtnState();
	static void resetBtnPushCount();
	static void togglePower();
	static void heartBeat();

private:
	static void saveState(byte);
	static void blink(byte, byte);
	static void power(boolean);
	static void toggleHeat(boolean);
	static void heatLevel(byte, byte);
	static void heatTimer();
};

#endif
