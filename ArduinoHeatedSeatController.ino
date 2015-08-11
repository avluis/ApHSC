/*
 * Heated Seat Switching (powered by Arduino)
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

 * Software: https://github.com/avluis/ArduinoHeatedSeatController
 * Hardware: https://github.com/avluis/ArduinoHeatedSeatController-Hardware
 *
 * Purpose:
 * To drive a ComfortHeat "Automotive Carbon Fiber Seat Heaters" by Rostra
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
 * Guide for the nonprogrammer:
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
#include <Arduino.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
#include <stdint.h>
#include <WString.h>

// Dashboard Buttons to monitor
const byte btnPin[] = { 2, 3 };

// Output to ULN2003A & M54564P
const byte statusPin[] = { 4, 5, 6, 7, 8, 9 };
// Pin 10 is only for ON Signal so we keep it out of the array
const byte onSignalPin = 10;
// On-Board LED (on Arduino)
const byte onBoardLedPin = 13;

// Count of button presses for each particular side
byte btnPushCount[] = { 0, 0 };

// Enable Auto Startup
byte autoStartup = 0;
// Auto Start Saved Heat Level (per side)
byte startupHeat[] = { 0, 0 };

// Enable Timer
byte timerEnabled = 0;
// Only run timer once
byte timerExpired = 0;
// Selected timer duration (from timerIntervals)
byte timerOption = 0;
// How far do we count before switching heat level (in minutes)
const byte timerIntervals[] = { 15, 10, 5, 1 };
// Reset timerOption to (from timerIntervals)
const byte timerIntvReset = 3;

// HeatBeat (LED) blink rate (in milliseconds)
const unsigned int ledBlinkRate = 1000;

// Enable Serial "Monitor" - For simple monitoring
const byte monitorEnabled = 0;
// Enable an additional "Monitor" - to debug timing related issues
const byte debugEnabled = 0;

// Macro(s)
#define ArrayElementSize(x) (sizeof(x) / sizeof(x[0]))
#define SERIALBAUD 9600

// Mapped EEPROM Data
#define EEPROMVER 0
#define AUTOSTARTUP 1
#define TIMEROPTION 2
#define HEATLVLOFFSET 3
#define CURRENTVER 1

/*
 * Function prototypes
 */
void setup(void);
void loop(void);
void ResetPushCounter(void);
void QueryButtonState(void);
void TogglePower(void);
void Power(boolean);
void ToggleHeat(boolean);
void HeatLevel(byte, byte);
void HeatTimer(void);
void SaveState(byte);
void Blink(byte, byte);
void HeartBeat(void);

void setup() {
	// Initializing On-Board LED as an output
	pinMode(onBoardLedPin, OUTPUT);
	// Initializing On Signal Pin as output
	pinMode(onSignalPin, OUTPUT);
	// Initializing Buttons as inputs
	for (byte x = 0; x < ArrayElementSize(btnPin); x++) {
		pinMode(btnPin[x], INPUT);
	}
	// Initializing Status Pins as outputs
	for (byte x = 0; x < ArrayElementSize(statusPin); x++) {
		pinMode(statusPin[x], OUTPUT);
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
	// Prepare EEPROM
	byte stored_ver = EEPROM.read(EEPROMVER);
	byte current_ver = (CURRENTVER);
	if (current_ver != stored_ver) {
		if (monitorEnabled) {
			Serial.println(F("EEPROM version mismatch!"));
		}
		for (uint16_t x = 0; x < EEPROM.length(); x++) {
			EEPROM.write(x, 0);
		}
		EEPROM.write(EEPROMVER, current_ver);
		EEPROM.write(AUTOSTARTUP, 0);
		EEPROM.write(TIMEROPTION, timerIntvReset);
		for (byte x = 0; x < ArrayElementSize(startupHeat); x++) {
			EEPROM.write(x + HEATLVLOFFSET, 0);
		}
		if (monitorEnabled) {
			Serial.println(F("EEPROM Cleared!"));
		}
	}
	// Auto Startup & Timer Feature
	autoStartup = EEPROM.read(AUTOSTARTUP);
	if (autoStartup) {
		if (monitorEnabled) {
			Serial.println(F("Auto Startup & Timer Feature Enabled."));
		}
		// Retrieve Saved Heat Level
		for (byte x = 0; x < ArrayElementSize(startupHeat); x++) {
			startupHeat[x] = EEPROM.read(x + HEATLVLOFFSET);
			if (startupHeat[x] >= 0 && startupHeat[x] <= 3) {
				btnPushCount[x] = EEPROM.read(x + HEATLVLOFFSET);
				if (monitorEnabled) {
					Serial.print(F("Heat Level - "));
					switch (x) {
					case 0:
						Serial.print(F("Driver Side: "));
						break;
					case 1:
						Serial.print(F("Passenger Side: "));
						break;
					default:
						Serial.print(F("ERROR! - Saved value out of range: "));
						break;
					}
					switch (btnPushCount[x]) {
					case 0:
						Serial.println(F("OFF"));
						break;
					case 1:
						Serial.println(F("HIGH"));
						break;
					case 2:
						Serial.println(F("MEDIUM"));
						break;
					case 3:
						Serial.println(F("LOW"));
						break;
					default:
						Serial.print(btnPushCount[x]);
						Serial.println(F("."));
						break;
					}
				}
				timerEnabled = 1;
			} else {
				// Clear Saved Heat Level
				EEPROM.write(x + HEATLVLOFFSET, 0);
				if (monitorEnabled) {
					Serial.println(F("Auto Startup Heat Level Cleared."));
				}
			}
		}
		// Retrieve timer option
		byte stored_tmr = EEPROM.read(TIMEROPTION);
		if (stored_tmr >= 0 && stored_tmr <= 3) {
			timerOption = stored_tmr;
			if (monitorEnabled) {
				Serial.print(F("Timer Option Set, Current Value: "));
				Serial.print(timerIntervals[timerOption]);
				if (timerIntervals[timerOption] == 1) {
					Serial.println(F(" Minute."));
				} else {
					Serial.println(F(" Minutes."));
				}
			}
		} else {
			if (monitorEnabled) {
				Serial.print(F("Timer Interval Out Of Range: "));
				Serial.println(timerOption);
			}
			// Reset timer option
			EEPROM.write(TIMEROPTION, timerIntvReset);
			timerOption = timerIntvReset;
			if (monitorEnabled) {
				Serial.println(F("Timer Option Reset."));
			}
		}
	}
}

void loop() {
	// Read the state of each button
	QueryButtonState();
	// Reset counters when btnPushCount > maxBtnPushCount
	ResetPushCounter();
	// Toggle power when btnPushCount != 0
	TogglePower();
	// Still alive?
	HeartBeat();
}

// If the number of button presses reaches maxBtnPushCount, reset btnPushCount to 0
void ResetPushCounter() {
	// Maximum allowed number of button presses/per side
	static const byte maxBtnPushCount = ArrayElementSize(statusPin) / 2;
	for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
		if (btnPushCount[x] > maxBtnPushCount) {
			if (monitorEnabled) {
				Serial.println(F("Button Press Counter Reset."));
			}
			btnPushCount[x] = 0;
		}
	}
}

// Listen for single and press+hold button pressed events, while debouncing!
void QueryButtonState() {
	// Tracks current button state for each particular side
	static byte btnState[] = { 0, 0 };
	// Tracks previous button state for each particular side
	static byte lastBtnState[] = { 0, 0 };

	// Single and Hold Button States
	static byte btnPressSingle = 0;
	static byte btnPressHold = 0;

	// Debounce period to filter out button bounce
	const byte debounceDelay = 25;

	// Time to trigger button press+hold event
	const int btnHoldTime = 500;

	// Time button was triggered
	static unsigned long btnTrigger = 0;
	// Time button was last triggered
	static unsigned long lastBtnTrigger[] = { 0, 0 };

	for (byte x = 0; x < ArrayElementSize(btnState); x++) {
		btnState[x] = digitalRead(btnPin[x]);
		if (btnState[x] == HIGH && lastBtnState[x] == LOW) { // first button press
			if (monitorEnabled) {
				Serial.println(F("Button Triggered:"));
			}
			btnTrigger = millis();
		}
		if (btnState[x] == HIGH && lastBtnState[x] == HIGH) { // button held
			if ((millis() - btnTrigger) > btnHoldTime) {
				btnPressHold = 1;
			}
		}
		if (btnState[x] == LOW && lastBtnState[x] == HIGH) { // button released
			if (((millis() - btnTrigger) > debounceDelay)
					&& btnPressHold != 1) {
				if ((millis() - lastBtnTrigger[x]) >= debounceDelay * 2) {
					btnPressSingle = 1;
					lastBtnTrigger[x] = millis();
				}
			}
			if (btnPressHold == 1) {
				if (monitorEnabled) {
					Serial.print(F("Press+Hold Event "));
				}
				if (btnPin[x] == 2) {
					if (monitorEnabled) {
						Serial.println(F("- Driver Side."));
					}
					SaveState(x);
				}
				if (btnPin[x] == 3) {
					if (monitorEnabled) {
						Serial.println(F("- Passenger Side."));
					}
					SaveState(x);
				}
				btnPressHold = 0;
			}
		}
		if (btnPressSingle == 1
				&& (millis() - lastBtnTrigger[x]) < btnHoldTime) {
			if (monitorEnabled) {
				Serial.print(F("Single Press Event "));
			}
			if (btnPin[x] == 2) {
				btnPushCount[0]++;
				if (monitorEnabled) {
					Serial.println(F("- Driver Side."));
				}
			}
			if (btnPin[x] == 3) {
				btnPushCount[1]++;
				if (monitorEnabled) {
					Serial.println(F("- Passenger Side."));
				}
			}
			btnPressSingle = 0;
		}
		lastBtnState[x] = btnState[x];
	}
}

// Toggles power ON/OFF in accordance to btnPushCount - Updates HeatTimer() if enabled (and not expired).
void TogglePower() {
	if (btnPushCount[0] != 0 || btnPushCount[1] != 0) {
		Power(1);
		if (debugEnabled) {
			Serial.println(F("Power is ON."));
		}
	} else {
		Power(0);
		if (debugEnabled) {
			Serial.println(F("Power is OFF."));
		}
	}
	if (timerEnabled && !timerExpired) {
		HeatTimer();
		if (debugEnabled) {
			Serial.println(F("Checking on Timer."));
		}
	}
}

// Toggles the onSignalPin (our ON/OFF signal) and calls ToggleHeat() according to Power(state).
void Power(boolean state) {
	if (state) {
		digitalWrite(onSignalPin, HIGH);
		ToggleHeat(1);
	} else {
		ToggleHeat(0);
		digitalWrite(onSignalPin, LOW);
	}
}

// Toggles heat ON/OFF - Sends heat level and side to HeatLevel().
void ToggleHeat(boolean state) {
	if (state) {
		for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
			HeatLevel(btnPushCount[x], x);
		}
	} else {
		for (byte x = 0; x < ArrayElementSize(statusPin); x++) {
			digitalWrite(statusPin[x], LOW);
		}
	}
}

// Handles all possible statusPin states according to the received heat level and side from ToggleHeat().
void HeatLevel(byte level, byte side) {
	for (byte n = 0; n < ArrayElementSize(statusPin) / 2; n++) {
		if (side == 0 && level > 0) {
			digitalWrite(statusPin[n], LOW);
			digitalWrite(statusPin[level] - 1, HIGH);
		}
		if (side == 0 && level == 0) {
			digitalWrite(statusPin[n], LOW);
		}
		if (side == 1 && level > 0) {
			digitalWrite(statusPin[n] + 3, LOW);
			digitalWrite(statusPin[level] + 2, HIGH);
		}
		if (side == 1 && level == 0) {
			digitalWrite(statusPin[n] + 3, LOW);
		}
	}
}

/*
 * Start a timer if Auto Start is enabled and timerEnabled is true.
 * Switch from saved Auto Start HeatLevel to OFF after millis() - runTime >= timer.
 * Stop all timers if btnPushCount changes while a timer is active for either side.
 *
 * Must be kept updated accordingly -- TogglePower() handles this.
 */
void HeatTimer() {
	// Track timer state
	static byte timerState[] = { 0, 0 };
	// Timer trigger (in mills)
	static unsigned long timer[] = { 0, 0 };
	// Convert from minutes to mills
	static unsigned long runTime = (unsigned long) timerIntervals[timerOption] * 60 * 1000;

	// Save btnPushCount[btn]
	static byte prevBtnPushCount[ArrayElementSize(btnPushCount)];
	for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
		prevBtnPushCount[x] = startupHeat[x];
	}
	// Our Timer (is complicated)...
	for (byte x = 0; x < ArrayElementSize(timerState); x++) {
		if (timerState[x]) { // Timer is running
			if (debugEnabled) {
				Serial.print(F("Timer is running for: "));
				Serial.println(x);
			}
			if ((long) millis() - runTime >= timer[x]) {
				timer[x] = millis();
			} else {
				timerState[x] = 0; // Reset timer state
				btnPushCount[x] = 0; // Reset btnPushCount
				if (monitorEnabled) {
					Serial.print(F("Timer has been reset for: "));
					Serial.println(x);
				}
			}
		}
		if (prevBtnPushCount[x] != btnPushCount[x]) { // btnPushCount changed
			timerState[x] = 0;
			if (!timerState[x]) {
				for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
					btnPushCount[x] = 0;
					timerState[x] = 0;
				}
			}
			if (monitorEnabled) {
				Serial.print(F("btnPushCount[btn] has changed for: "));
				Serial.println(x);
			}
		}
		if (btnPushCount[x] == 0) { // Don't run timer if already OFF
			timerState[x] = 0;
			if (debugEnabled) {
				Serial.print(F("This side's timer is disabled: "));
				Serial.println(x);
			}
		} else {
			timerState[x] = 1;
		}
		if (!timerState[0] && !timerState[1]) { // Both timers have been reset
			if (!btnPushCount[0] && !btnPushCount[1]) { // Both buttons have been reset
				timerExpired = 1;
				if (monitorEnabled) {
					Serial.println(F("Timer has expired."));
				}
			}
		}
	}
}

/*
 * Saves Auto Start option (along with timer) to EEPROM when (driver button)
 * press+hold is triggered along with the currently selected heat level.
 * Saves timerOption to EEPROM when (passenger button) press+hold is triggered.
 * Disables Auto Start, resets timerOption and Heat Level if both heat levels
 * are OFF when (driver button) press+hold is triggered.
 * Resets timerOption to timerIntvReset when (passenger button) press+hold
 * is triggered and (passenger) heat level is OFF.
 *
 * EEPROM is saved as such:
 * [0:EEPROMver, 1:autoStartup, 2:timerOption, 3:drvHeatLevel, 4:pasHeatLevel]
 */
void SaveState(byte btn) {
	if (btn == 0) { // Driver button press+hold
		if ((btnPushCount[0] == 0) && (btnPushCount[1] == 0)) {
			if (monitorEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Disabled."));
				Serial.println(F("Timer Interval Reset."));
			}
			EEPROM.write(AUTOSTARTUP, 0);
			EEPROM.write(TIMEROPTION, timerIntvReset);
			for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
				EEPROM.write(x + HEATLVLOFFSET, 0);
			}
			if (monitorEnabled) {
				Serial.println(F("Auto Startup Heat Level Cleared."));
			}
			Blink(btn, 1); // Blink OFF Pattern
		}
		if ((btnPushCount[0] >= 1 && btnPushCount[0] <= 3)
				|| (btnPushCount[1] >= 1 && btnPushCount[1] <= 3)) {
			if (monitorEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Enabled."));
			}
			EEPROM.write(AUTOSTARTUP, 1);
			for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
				EEPROM.write(x + HEATLVLOFFSET, btnPushCount[x]);
			}
			if (monitorEnabled) {
				Serial.println(F("Auto Startup Heat Levels Saved."));
			}
			Blink(btn, 0); // Blink ON Pattern
		}
	}
	if (btn == 1) { // Passenger button press+hold
		if (btnPushCount[btn] >= 1 && btnPushCount[btn] <= 3) {
			EEPROM.write(TIMEROPTION, btnPushCount[btn] - 1);
		} else {
			if (monitorEnabled) {
				Serial.println(F("Heat Level is OFF, Timer Interval Reset."));
			}
			EEPROM.write(TIMEROPTION, timerIntvReset);
		}
		Blink(btn, 2); // Blink TOGGLE Pattern
	}
}

// Blinks the LED for the current heat level (if not OFF) for the desired pattern.
// Designed to not allow background activity for a small blinkDelay while blinking.
void Blink(byte btn, byte pattern) {
	// Save btnPushCount[btn] to save our current state.
	byte prevBtnPushCount = 0;
	prevBtnPushCount = btnPushCount[btn];

	// Blink Patterns:
	//        {onTime, offTime}:               ON    -   OFF    -  TOGGLE   -  ERROR
	const unsigned long blinkPatterns[] = { 1500, 500, 350, 1500, 1000, 1000, 350, 350 };

	const int blinkDelay = 500;
	unsigned long blinkTimer = millis();
	unsigned long patternTimer = millis() + blinkDelay;

	if (pattern >= 3) {
		pattern = 3;
	}
	if (monitorEnabled) {
		Serial.print(F("Blink Pattern: "));

		switch (pattern) {
		case 0:
			Serial.println(F("ON."));
			break;
		case 1:
			Serial.println(F("OFF."));
			break;
		case 2:
			Serial.println(F("TOGGLE."));
			break;
		default:
			Serial.println(F("Pattern out of Range!"));
			break;
		}
	}
	/*
	 * Setting btnPushCount[btn] to 0 and calling TogglePower()
	 * prior to running the blink patterns ensures we don't
	 * toggle heat on and off as we blink.
	 */
	btnPushCount[btn] = 0;
	TogglePower();

	byte blinkPin = 0;
	int pinOffset = 0;

	for (byte x = 0; x < ArrayElementSize(btnPin); x++) {
		if (prevBtnPushCount >= 1) {
			blinkPin = prevBtnPushCount;
		} else {
			blinkPin = 2;
		}
		if (btn == 0) {
			pinOffset = -1;
		} else {
			pinOffset = 2;
		}
		// Every time Blink() gets called, we need to wait for blinkDelay before we run the blink patterns.
		while (millis() - blinkTimer < blinkDelay) {
			if (debugEnabled) {
				Serial.println(millis() - blinkTimer);
			}
		}
		if (debugEnabled) {
			Serial.println(F("HIGH"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2)])) {
			digitalWrite(statusPin[blinkPin] + pinOffset, HIGH);
			if (debugEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
		if (debugEnabled) {
			Serial.println(F("LOW"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2) + 1])) {
			digitalWrite(statusPin[blinkPin] + pinOffset, LOW);
			if (debugEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
	}
	// Restore btnPushCount[btn] to restore program state before Blink().
	btnPushCount[btn] = prevBtnPushCount;
	TogglePower();
}

// Blinks onBoardLed to indicate HeatBeat per every 1 second - Use this to measure loop() speed.
void HeartBeat() {
	static unsigned int ledStatus = LOW;
	static unsigned long ledBlinkTime = 0;
	// Possibly the most straightforward blink of all time (besides Blink())
	if ((long) (millis() - ledBlinkTime) >= 0) {
		ledStatus = (ledStatus == HIGH ? LOW : HIGH);
		digitalWrite(onBoardLedPin, ledStatus);
		ledBlinkTime = millis() + ledBlinkRate;
	}
}
