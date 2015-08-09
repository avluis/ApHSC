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
 * To drive a ComfortHeat "Automotive Carbon Fiber Seat Heaters"
 * by Rostra with the stock control panel of a vehicle, in my case, a 2011 Suzuki Kizashi
 * [Rostra Kit: 250-1872 (Universal Kit. Double thumb-dial Switch)
 * Install Instructions: http://www.rostra.com/manuals/250-1870_Form5261.pdf]
 *
 * This sketch handles the logic required to control a 4-stage heated seat module.
 * The stages are HIGH, MEDIUM, LOW and OFF. OFF is the default-start state.
 * Indication of stages is done via LEDs for HIGH, MEDIUM and LOW.
 * Vehicle wiring is ground based and built-in LEDs turn on when grounded.
 * Control module is positive (+12v) signal switching.
 *
 * Grounding of LEDs is handled by a ULN2003A/TPL7407L (U3).
 * Heat level switching is handled by a M54564P/A2982/TD62783APG (U4).
 * U3 & U4 input pins are driven by the Arduino's Digital Output Pins.
 *
 * INPUT:
 * Pin 2 & 3 for the Driver and Passenger Heat Buttons.
 *
 * OUTPUT:
 * Pin 4, 5 and 6 for Driver side LED/Heat.
 * Pin 7, 8 and 9 for Passenger side LED/Heat.
 * Pin 10 is the ON signal for the Rostra Controller.
 *
 * Guide for the nonprogrammer:
 * Auto Startup and Timer - If activated, Heat Controller will start up when it is powered
 * ON at a previously saved heat level. Additionally, a timer will run for a preset time
 * from HIGH heat to MEDIUM heat. If Auto Startup is enabled, the timer can be turned off
 * temporarily (until next power on) if any of the heated seats is turned off via any of
 * the two buttons.
 * The timer will run only once (once timer has expired, it will not repeat), only if autostart
 * is enabled and only for the seat set to HIGH heat whenever autostart is enabled.
 * To enable Auto Start, set heat level where desired then press and hold the driver side button
 * for a few seconds to set the heat level and enable auto start.
 * Note that the timer will only run for the side set to HIGH heat.
 * So if you want the timer for both sides, set both heat levels to HIGH.
 * Pressing and holding the driver side button again will clear the currently set heat level and
 * disable auto start.
 * To set how long the timer will run for, press and hold the passenger side button for a few seconds
 * with the following in mind: HIGH = 15 minutes, MEDIUM = 10 minutes and LOW = 5 minutes.
 * Please note that disabling auto start resets the timer back to 15 minutes.
 */

// Using:
#include <Arduino.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
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

// Enable Timer
byte timerEnabled = 0;

// Only run timer once
byte timerExpired = 0;

// Selected timer duration (from timerIntervals)
byte timerOption = 0;

// How far do we count before switching heat level (from HI to MID)
const unsigned long timerIntervals[] = { 900000, 600000, 300000, 15000 };

// If timer interval needs to be reset, then set it to this:
const byte timerIntvReset = 3;

// Enable Serial - Handy for debugging
const byte serialEnabled = 0;

// Macro(s)
#define ArrayElementSize(x) (sizeof(x) / sizeof(x[0]))
#define SERIALBAUD 9600

// Mapped EEPROM Data
#define EEPROMVER 0
#define AUTOSTARTUP 1
#define TIMEROPTION 2
#define HEATLVLOFFSET 3
#define CURRENTVER 001

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
void HeatTimer();
void SaveState(byte);
void Blink(byte, byte);

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
	// Set up serial
	if (serialEnabled) {
		Serial.begin(SERIALBAUD);
		Serial.println(F("Heat Controller Up & Running."));
	}
	// Prepare EEPROM
	if (EEPROM.read(EEPROMVER) != CURRENTVER) {
		if (serialEnabled) {
			Serial.println(F("EEPROM version mismatch!"));
		}
		for (byte x = 0; x < EEPROM.length(); x++) {
			EEPROM.write(x, 0);
		}
		if (serialEnabled) {
			Serial.println(F("EEPROM Cleared!"));
		}
		EEPROM.write(EEPROMVER, CURRENTVER);
	}

	// Auto Startup & Timer Feature
	autoStartup = EEPROM.read(AUTOSTARTUP);
	if (autoStartup) {
		if (serialEnabled) {
			Serial.println(F("Auto Startup & Timer Feature Enabled."));
		}
		// Auto Start Saved Heat Level (per side)
		static byte startupHeat[] = { 0, 0 };
		timerEnabled = 1;
		// Retrieve Saved Heat Level
		for (byte x = 0; x < ArrayElementSize(startupHeat); x++) {
			startupHeat[x] = EEPROM.read(x + HEATLVLOFFSET);
			if (startupHeat[x] >= 0 && startupHeat[x] <= 3) {
				btnPushCount[x] = EEPROM.read(x + HEATLVLOFFSET);
				if (serialEnabled) {
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
			} else {
				// Clear Saved Heat Level
				EEPROM.write(x + HEATLVLOFFSET, 0);
				if (serialEnabled) {
					Serial.println(F("Auto Startup Heat Level Cleared."));
				}
			}
		}
	}
	// Retrieve timer interval
	timerOption = EEPROM.read(TIMEROPTION);
	if (timerOption >= 0 && timerOption <= 3) {
		if (serialEnabled) {
			Serial.print(F("Timer Interval Set, Current Value: "));
			Serial.print(timerIntervals[timerOption]);
			Serial.println(F(" Milliseconds."));
		}
	} else {
		if (serialEnabled) {
			Serial.print(F("Timer Interval Out Of Range: "));
			Serial.println(timerOption);
		}
		// Reset timer interval
		EEPROM.write(TIMEROPTION, 0);
		timerOption = timerIntvReset;
		if (serialEnabled) {
			Serial.println(F("Timer Interval Reset."));
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
}

// If the number of button presses reaches
// a certain number, reset btnPushCount to 0
void ResetPushCounter() {
	// Maximum allowed number of button presses/per side
	static const byte maxBtnPushCount = ArrayElementSize(statusPin) / 2;
	for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
		if (btnPushCount[x] > maxBtnPushCount) {
			if (serialEnabled) {
				Serial.println(F("Button Press Counter Reset."));
			}
			btnPushCount[x] = 0;
		}
	}
}

// Listens for button presses, while debouncing the button input.
// Tracks the number of presses respective to each button (side).
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
			if (serialEnabled) {
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
				if (serialEnabled) {
					Serial.print(F("Press+Hold Event "));
				}
				if (btnPin[x] == 2) {
					if (serialEnabled) {
						Serial.println(F("- Driver Side."));
					}
					SaveState(x);
				}
				if (btnPin[x] == 3) {
					if (serialEnabled) {
						Serial.println(F("- Passenger Side."));
					}
					SaveState(x);
				}
				btnPressHold = 0;
			}
		}
		if (btnPressSingle == 1
				&& (millis() - lastBtnTrigger[x]) < btnHoldTime) {
			if (serialEnabled) {
				Serial.print(F("Single Press Event "));
			}
			if (btnPin[x] == 2) {
				btnPushCount[0]++;
				if (serialEnabled) {
					Serial.println(F("- Driver Side."));
				}
			}
			if (btnPin[x] == 3) {
				btnPushCount[1]++;
				if (serialEnabled) {
					Serial.println(F("- Passenger Side."));
				}
			}
			btnPressSingle = 0;
		}
		lastBtnState[x] = btnState[x];
	}
}

/*
 * Toggles power ON/OFF in accordance to the number of button presses on each respective side/button.
 * Updates HeatTimer() if enabled (and not expired).
 */
void TogglePower() {
	if (btnPushCount[0] != 0 || btnPushCount[1] != 0) {
		Power(1);
		if (serialEnabled) {
			Serial.println(F("ON."));
		}
	} else {
		Power(0);
		if (serialEnabled) {
			Serial.println(F("OFF."));
		}
	}
	if (timerEnabled == 1 && timerExpired == 0) {
		HeatTimer();
	}
}

// Toggles the On-Board LED(s) and calls ToggleHeat() according to power state which sends our ON/OFF signal.
void Power(boolean state) {
	if (state) {
		digitalWrite(onBoardLedPin, HIGH);
		digitalWrite(onSignalPin, HIGH);
		ToggleHeat(1);
	} else {
		ToggleHeat(0);
		digitalWrite(onSignalPin, LOW);
		digitalWrite(onBoardLedPin, LOW);
	}
}

// Enables/Disables heat when called - Sends heat level and side to HeatLevel().
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
 * Start a timer if heat level is set to HIGH for more than timerDelay.
 * Switch HeatLevel from HIGH to MID after timerTrigger > timerInterval.
 * Stop timer if btnPushCount == 0 while timer is active for either side.
 *
 * Must be kept updated accordingly -- TogglePower() handles this.
 */
void HeatTimer() {
	// Tracks if the timer has already been called
	static byte timerState[] = { 0, 0 };

	// When was the timer triggered (in mills)
	static unsigned long timerTrigger[] = { 0, 0 };

	// How long do we wait before starting the timer
	const int timerDelay = 15000;

	for (byte x = 0; x < ArrayElementSize(timerState); x++) {
		if (timerState[x] == 0) {
			if (btnPushCount[x] == 1) {
				timerState[x] = 1;
				timerTrigger[x] = millis();
				if (serialEnabled) {
					Serial.print(F("Timer Triggered: "));
					Serial.print(timerTrigger[x]);
					Serial.println(F(" Milliseconds."));
				}
			}
			if (btnPushCount[x] > 1) {
				timerState[x] = 0;
				timerTrigger[x] = 0;
			}
		}
		if (timerState[x] == 1) {
			if (btnPushCount[x] == 1) {
				if ((millis() - timerTrigger[x]) >= timerDelay) {
					if ((millis() - timerTrigger[x])
							>= timerIntervals[timerOption]) {
						timerState[x] = 0;
						timerTrigger[x] = 0;
						btnPushCount[x] = 2;
					}
				}
			}
			if ((timerState[0] == 1 && timerState[1] == 0)
					|| (timerState[0] == 0 && timerState[1] == 1)) {
				if ((btnPushCount[0] >= 1 && btnPushCount[1] == 0)
						|| (btnPushCount[0] == 0 && btnPushCount[1] >= 1)) {
					timerState[x] = 0;
					timerTrigger[x] = 0;
					timerExpired = 1;
					if (serialEnabled) {
						Serial.println(F("Timer Manually Reset."));
					}
				}
			}
			if (btnPushCount[x] > 1) {
				timerState[x] = 0;
				TogglePower();
				if (btnPushCount[0] >= 2 && btnPushCount[1] >= 2) {
					timerExpired = 1;
					if (serialEnabled) {
						Serial.println(F("Timer Expired."));
					}
				}
			}
		}
	}
}

/*
 * Saves auto start option (along with timer) to EEPROM when (driver)
 * press+hold is triggered along with the currently selected heat level.
 * Saves timer duration to EEPROM when (passenger) press+hold is triggered.
 *
 * EEPROM is saved as such:
 * [0:EEPROMver, 1:autoStartup, 2:timerOption, 3:drvHeatLevel, 4:pasHeatLevel]
 */
void SaveState(byte btn) {
	if (btn == 0) { // Driver button press+hold
		if ((btnPushCount[0] >= 1 && btnPushCount[0] <= 3)
				|| (btnPushCount[1] >= 1 && btnPushCount[1] <= 3)) {
			if (serialEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Enabled."));
			}
			EEPROM.write(AUTOSTARTUP, 1);
			for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
				EEPROM.write(x + HEATLVLOFFSET, btnPushCount[x]);
			}
			if (serialEnabled) {
				Serial.println(F("Auto Startup Heat Levels Saved."));
			}
			Blink(btn, 0); // Blink ON Pattern
		} else {
			if (serialEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Disabled."));
				Serial.println(F("Timer Interval Reset."));
			}
			EEPROM.write(AUTOSTARTUP, 0);
			EEPROM.write(TIMEROPTION, 0);
			for (byte x = 0; x < ArrayElementSize(btnPushCount); x++) {
				EEPROM.write(x + HEATLVLOFFSET, 0);
			}
			if (serialEnabled) {
				Serial.println(F("Auto Startup Heat Level Cleared."));
			}
			timerExpired = 1;
			Blink(btn, 1); // Blink OFF Pattern
		}
	}
	if (btn == 1) { // Passenger button press+hold
		if (btnPushCount[btn] >= 1 && btnPushCount[btn] <= 3) {
			EEPROM.write(TIMEROPTION, btnPushCount[btn] - 1);
			if (serialEnabled) {
				int savedTimerOption = EEPROM.read(TIMEROPTION);
				Serial.print(F("New Timer Interval: "));
				Serial.print(timerIntervals[savedTimerOption]);
				Serial.println(F(" Milliseconds."));
			}
		} else {
			if (serialEnabled) {
				Serial.println(F("Heat Level is OFF, Timer Interval Reset."));
			}
			EEPROM.write(TIMEROPTION, timerIntvReset);
		}
		Blink(btn, 2); // Blink TOGGLE Pattern
	}
}

// Takes care of all the blinking for us according to a desired pattern.
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

	if (serialEnabled) {
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
		// Every time Blink() gets called, we need to wait for blinkDelay
		// before we run the blink patterns.
		while (millis() - blinkTimer < blinkDelay) {
			if (serialEnabled) {
				Serial.println(millis() - blinkTimer);
			}
		}
		if (serialEnabled) {
			Serial.println(F("HIGH"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2)])) {
			digitalWrite(statusPin[blinkPin] + pinOffset, HIGH);
			if (serialEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
		if (serialEnabled) {
			Serial.println(F("LOW"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2) + 1])) {
			digitalWrite(statusPin[blinkPin] + pinOffset, LOW);
			if (serialEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
	}

	// Restore btnPushCount[btn] to restore program state before Blink().
	btnPushCount[btn] = prevBtnPushCount;
	TogglePower();
}
