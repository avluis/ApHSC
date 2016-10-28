// Const.h

#ifndef _CONST_h
#define _CONST_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// Dashboard Buttons to monitor
const byte kBtnPins[] = { 2, 3 };

// Output to ULN2003A & M54564P
const byte kStatusPins[] = { 4, 5, 6, 7, 8, 9 };
// Pin 10 is only for ON Signal so we keep it out of the array
const byte kOnSignalPin = 10;
// On-Board LED (on Arduino)
const byte kOnBoardLedPin = 13;

// How far do we count before switching heat level (in minutes)
const byte kTimerIntervals[] = { 15, 10, 5, 1 };
// Reset timer_option to (from kTimerIntervals)
const byte kTimerIntvReset = 3;

// HeartBeat (LED) blink rate (in milliseconds)
const unsigned int kLedBlinkRate = 1000;

// Enable Serial "Monitor" - For simple monitoring
const byte kMonitorEnabled = 0;
// Enable an additional "Monitor" - to debug timing related issues
const byte kDebugEnabled = 0;

// Macro(s)
#define ArrayLength(x) (sizeof(x) / sizeof(x[0]))
#define SERIALBAUD 9600

// Mapped EEPROM Data
#define EEPROMVER 0
#define AUTOSTARTUP 1
#define TIMEROPTION 2
#define HEATLVLOFFSET 3
#define CURRENTVER 1

#endif
