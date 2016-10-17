/*
 * ApHSC (Arduino powered Heated Seat Controller)
 * After-market Heated Seat Switching (powered by Arduino)
 * Copyright (C) 2015 Luis E Alvarado
 * Contact: admin@avnet.ws or alvaradorocks@gmail.com

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 * Software: https://github.com/avluis/ApHSC
 * Hardware: https://github.com/avluis/ApHSC-Hardware
 *
 * Purpose:
 * To control a ComfortHeat "Automotive Carbon Fiber Seat Heaters" by Rostra
 * with the stock control panel of a vehicle, in my case, a 2011 Suzuki Kizashi.
 * [Rostra Kit: 250-1872 (Universal Kit. Double thumb-dial Switch)
 * Install Instructions: http://www.rostra.com/manuals/250-1870_Form5261.pdf]
 *
 * This program handles the logic required to control a 4-Stage Heated Seat Module.
 * The stages are HIGH, MEDIUM, LOW and OFF. OFF is the default-start state.
 * Indication of stages is done via LEDs for HIGH, MEDIUM and LOW.
 * Vehicle wiring is ground based and built-in LEDs turn on when grounded.
 * Rostra Control module is positive (+12v) signal switching.
 *
 * Grounding of LEDs is handled by a ULN2003A/TPL7407L (U3).
 * Heat level switching is handled by a M54564P/A2982/TD62783APG (U4).
 * U3 & U4 input pins are driven by the Arduino's Digital Output Pins.
 *
 * INPUT:
 * Pin 2 & 3 for the Driver and Passenger Heat Buttons.
 *
 * OUTPUT:
 * Pin 4, 5 and 6 for Driver side LEDs/Heat Level.
 * Pin 7, 8 and 9 for Passenger side LEDs/Heat Level.
 * Pin 10 is the ON signal for the Rostra Controller.
 *
 * Guide for the non-programmer:
 *
 * Auto Startup and Timer -
 * If activated, the heat controller will start up when it receives power (think remote start)
 * at a previously saved heat level. Additionally, a timer will run for a preset amount of time
 * (adjustable) from the current heat level and power off once the preset time has passed.
 * If Auto Startup is enabled, the timer can be turned off manually (until next power on)
 * if any (either side) of the heated seat buttons is pressed.
 * The timer will run only once (per startup) - only if Auto Start has been enabled and only
 * for the seat(s) not set to OFF whenever Auto Start is enabled.
 *
 * To enable Auto Start, first set the desired heat level for both seats, then press and hold
 * the driver side button for a few seconds to save the current heat level and enable Auto Start.
 * Note that the timer will only run on the side where a heat level has been selected.
 * So if you want the timer for both sides, set both seats to the desired heat level.
 * If you want the timer on only one side, set that side only (leave the other side OFF).
 * Pressing and holding the driver side button again while both seats are OFF will clear the saved
 * heat levels and disable Auto Start.
 *
 * To set how long the timer will run (for both seats), press and hold the passenger side button for
 * a few seconds with the following in mind:
 * High Heat = 15 minutes, Medium Heat = 10 minutes and Low Heat = 5 minutes.
 * Please note that disabling Auto Start will reset the timer to 1 minute (as a means prevent accidents).
 * If you want to set the timer to 1 minute (manually), set the passenger heat level to OFF and then
 * press and hold the passenger button (just like when saving) to reset the timer back to 1 minute.
 *
 * Additional features:
 *
 * While enabling/disabling Auto Start or saving/clearing heat levels - the current heat level will blink
 * to indicate ON (long blink on, short blink off), OFF (short blink on, long blink off), and toggle
 * (medium blinks of equal duration). An error blink is available, but not yet implemented.
 *
 * If populated, Arduino pin 13 (pin 17 in wiring) is used as a HeartBeat indicator per loop cycle.
 * This is helpful if you wish to measure (in Hz) how long the program is taking to complete a loop.
 * The LED is set to blink at a rate of 1000ms (1 second) by default.
 *
 * There are two ways (previous mentioned above) to monitor program activity via Serial -
 * simply set monitorEnabled to 1 and open a Serial Monitor at a Baud Rate of 9600.
 * If you wish, debugEnabled can be set as well, but be warned that it will flood your Serial Monitor.
 */

// Using:
#include "Const.h"
#include "Helper.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <stdint.h>
#include <WString.h>

void setup(void);
void loop(void);

// Setup
void setup() {
	// Initializing On-Board LED as an output
	pinMode(onBoardLedPin, OUTPUT);
	// Initializing On Signal Pin as output
	pinMode(onSignalPin, OUTPUT);
	// Initializing Buttons as inputs
	for (byte x = 0; x < ArrayLength(btnPins); x++) {
		pinMode(btnPins[x], INPUT);
	}
	// Initializing Status Pins as outputs
	for (byte x = 0; x < ArrayLength(statusPins); x++) {
		pinMode(statusPins[x], OUTPUT);
	}
	// Serial || Debug
	if (monitorEnabled || debugEnabled) {
		Serial.begin(SERIALBAUD);
		Serial.println(F("Heat Controller Up & Running."));
	}
	// Serial AND Debug!
	if (monitorEnabled && debugEnabled) {
		Serial.println(F("Don't blame me if you have to scroll~"));
	} else if (debugEnabled) { // Debug Time!
		Serial.println(F("Debug is enabled - don't get flooded!"));
	} else { // Just Serial please.
		Serial.println(F("Welcome, enjoy your stay."));
	}

	// Helper class
	Helper::init();
}

// Program loop
void loop() {
	// Read the state of each button
	Helper::queryBtnState();
	// Reset counters when btnPushCount > maxBtnPushCount
	Helper::resetBtnPushCount();
	// Toggle power when btnPushCount != 0
	Helper::togglePower();
	// Still alive?
	Helper::heartBeat();
}
