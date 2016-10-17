// Helper functions for ApHSC

#include "Const.h"
#include "Helper.h"
#include "EEPROM\src\EEPROM.h"

// Count of button presses for each particular side
static byte btnPushCount[] = { 0, 0 };

// Enable Auto Startup
static byte autoStartup;
// Auto Start Saved Heat Level (per side)
static byte startupHeat[] = { 0, 0 };

// Enable Timer
static byte timerEnabled;

// Selected timer duration (from timerIntervals)
static byte timerOption;
// Only run timer once
static byte timerExpired;

void Helper::init()
{
	autoStartup = 0;
	timerEnabled = 0;
	timerOption = 0;
	timerExpired = 0;

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
		for (byte x = 0; x < ArrayLength(startupHeat); x++) {
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
		for (byte x = 0; x < ArrayLength(startupHeat); x++) {
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
			}
			else {
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
				}
				else {
					Serial.println(F(" Minutes."));
				}
			}
		}
		else {
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

// Listen for single and press+hold button pressed events, while de-bouncing!
void Helper::queryBtnState() {
	// Tracks current button state for each particular side
	static byte btnState[] = { 0, 0 };
	// Tracks previous button state for each particular side
	static byte lastBtnState[] = { 0, 0 };

	// Single and Hold Button States
	static byte btnPressSingle = 0;
	static byte btnPressHold = 0;

	// De-bounce period to filter out button bounce
	const byte debounceDelay = 25;

	// Time to trigger button press+hold event
	const int btnHoldTime = 500;

	// Time button was triggered
	static unsigned long btnTrigger = 0;
	// Time button was last triggered
	static unsigned long lastBtnTrigger[] = { 0, 0 };

	for (byte x = 0; x < ArrayLength(btnState); x++) {
		btnState[x] = digitalRead(btnPins[x]);
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
				if (btnPins[x] == 2) {
					if (monitorEnabled) {
						Serial.println(F("- Driver Side."));
					}
					saveState(x);
				}
				if (btnPins[x] == 3) {
					if (monitorEnabled) {
						Serial.println(F("- Passenger Side."));
					}
					saveState(x);
				}
				btnPressHold = 0;
			}
		}
		if (btnPressSingle == 1
			&& (millis() - lastBtnTrigger[x]) < btnHoldTime) {
			if (monitorEnabled) {
				Serial.print(F("Single Press Event "));
			}
			if (btnPins[x] == 2) {
				btnPushCount[0]++;
				if (monitorEnabled) {
					Serial.println(F("- Driver Side."));
				}
			}
			if (btnPins[x] == 3) {
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

// If the number of button presses reaches maxBtnPushCount, reset btnPushCount to 0
void Helper::resetBtnPushCount() {
	// Maximum allowed number of button presses/per side
	static const byte maxBtnPushCount = ArrayLength(statusPins) / 2;
	for (byte x = 0; x < ArrayLength(btnPushCount); x++) {
		if (btnPushCount[x] > maxBtnPushCount) {
			if (monitorEnabled) {
				Serial.println(F("Button Press Counter Reset."));
			}
			btnPushCount[x] = 0;
		}
	}
}

// Toggles power ON/OFF in accordance to btnPushCount - Updates heatTimer() if enabled (and not expired).
void Helper::togglePower() {
	if (btnPushCount[0] != 0 || btnPushCount[1] != 0) {
		power(1);
		if (debugEnabled) {
			Serial.println(F("Power is ON."));
		}
	}
	else {
		power(0);
		if (debugEnabled) {
			Serial.println(F("Power is OFF."));
		}
	}
	if (timerEnabled && !timerExpired) {
		heatTimer();
		if (debugEnabled) {
			Serial.println(F("Checking on Timer."));
		}
	}
}

// Blinks onBoardLed to indicate HeatBeat per every 1 second - Use this to measure loop() speed.
void Helper::heartBeat() {
	static unsigned int ledStatus = LOW;
	static unsigned long ledBlinkTime = 0;
	// Possibly the most straightforward blink of all time (besides blink())
	if ((long)(millis() - ledBlinkTime) >= 0) {
		ledStatus = (ledStatus == HIGH ? LOW : HIGH);
		digitalWrite(onBoardLedPin, ledStatus);
		ledBlinkTime = millis() + ledBlinkRate;
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
void Helper::saveState(byte btn) {
	if (btn == 0) { // Driver button press+hold
		if ((btnPushCount[0] == 0) && (btnPushCount[1] == 0)) {
			if (monitorEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Disabled."));
				Serial.println(F("Timer Interval Reset."));
			}
			EEPROM.write(AUTOSTARTUP, 0);
			EEPROM.write(TIMEROPTION, timerIntvReset);
			for (byte x = 0; x < ArrayLength(btnPushCount); x++) {
				EEPROM.write(x + HEATLVLOFFSET, 0);
			}
			if (monitorEnabled) {
				Serial.println(F("Auto Startup Heat Level Cleared."));
			}
			blink(btn, 1); // blink OFF Pattern
		}
		if ((btnPushCount[0] >= 1 && btnPushCount[0] <= 3)
			|| (btnPushCount[1] >= 1 && btnPushCount[1] <= 3)) {
			if (monitorEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Enabled."));
			}
			EEPROM.write(AUTOSTARTUP, 1);
			for (byte x = 0; x < ArrayLength(btnPushCount); x++) {
				EEPROM.write(x + HEATLVLOFFSET, btnPushCount[x]);
			}
			if (monitorEnabled) {
				Serial.println(F("Auto Startup Heat Levels Saved."));
			}
			blink(btn, 0); // blink ON Pattern
		}
	}
	if (btn == 1) { // Passenger button press+hold
		if (btnPushCount[btn] >= 1 && btnPushCount[btn] <= 3) {
			EEPROM.write(TIMEROPTION, btnPushCount[btn] - 1);
		}
		else {
			if (monitorEnabled) {
				Serial.println(F("Heat Level is OFF, Timer Interval Reset."));
			}
			EEPROM.write(TIMEROPTION, timerIntvReset);
		}
		blink(btn, 2); // blink TOGGLE Pattern
	}
}

// Blinks the LED for the current heat level (if not OFF) for the desired pattern.
// Designed to not allow background activity for a small blinkDelay while blinking.
void Helper::blink(byte btn, byte pattern) {
	// Save btnPushCount[btn] to save our current state.
	byte prevBtnPushCount = 0;
	prevBtnPushCount = btnPushCount[btn];

	// blink Patterns:
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
	* Setting btnPushCount[btn] to 0 and calling togglePower()
	* prior to running the blink patterns ensures we don't
	* toggle heat on and off as we blink.
	*/
	btnPushCount[btn] = 0;
	togglePower();

	byte blinkPin = 0;
	int pinOffset = 0;

	for (byte x = 0; x < ArrayLength(btnPins); x++) {
		if (prevBtnPushCount >= 1) {
			blinkPin = prevBtnPushCount;
		}
		else {
			blinkPin = 2;
		}
		if (btn == 0) {
			pinOffset = -1;
		}
		else {
			pinOffset = 2;
		}
		// Every time blink() gets called, we need to wait for blinkDelay before we run the blink patterns.
		while (millis() - blinkTimer < blinkDelay) {
			if (debugEnabled) {
				Serial.println(millis() - blinkTimer);
			}
		}
		if (debugEnabled) {
			Serial.println(F("HIGH"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2)])) {
			digitalWrite(statusPins[blinkPin] + pinOffset, HIGH);
			if (debugEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
		if (debugEnabled) {
			Serial.println(F("LOW"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2) + 1])) {
			digitalWrite(statusPins[blinkPin] + pinOffset, LOW);
			if (debugEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
	}
	// Restore btnPushCount[btn] to restore program state before blink().
	btnPushCount[btn] = prevBtnPushCount;
	togglePower();
}

// Toggles the onSignalPin (our ON/OFF signal) and calls toggleHeat() according to power(state).
void Helper::power(boolean state) {
	if (state) {
		digitalWrite(onSignalPin, HIGH);
		toggleHeat(1);
	}
	else {
		toggleHeat(0);
		digitalWrite(onSignalPin, LOW);
	}
}

// Toggles heat ON/OFF - Sends heat level and side to heatLevel().
void Helper::toggleHeat(boolean state) {
	if (state) {
		for (byte x = 0; x < ArrayLength(btnPushCount); x++) {
			heatLevel(btnPushCount[x], x);
		}
	}
	else {
		for (byte x = 0; x < ArrayLength(statusPins); x++) {
			digitalWrite(statusPins[x], LOW);
		}
	}
}

// Handles all possible statusPins states according to the received heat level and side from toggleHeat().
void Helper::heatLevel(byte level, byte side) {
	for (byte n = 0; n < ArrayLength(statusPins) / 2; n++) {
		if (side == 0 && level > 0) {
			digitalWrite(statusPins[n], LOW);
			digitalWrite(statusPins[level] - 1, HIGH);
		}
		if (side == 0 && level == 0) {
			digitalWrite(statusPins[n], LOW);
		}
		if (side == 1 && level > 0) {
			digitalWrite(statusPins[n] + 3, LOW);
			digitalWrite(statusPins[level] + 2, HIGH);
		}
		if (side == 1 && level == 0) {
			digitalWrite(statusPins[n] + 3, LOW);
		}
	}
}

/*
* Start a timer if Auto Start is enabled and timerEnabled is true.
* Switch from saved Auto Start heatLevel to OFF after millis() - runTime >= timer.
* Stop all timers if btnPushCount changes while a timer is active for either side.
*
* Must be kept updated accordingly -- togglePower() handles this.
*/
void Helper::heatTimer() {
	// Track timer state
	static byte timerState[] = { 0, 0 };
	// Timer trigger (in mills)
	static unsigned long timer[] = { 0, 0 };
	// Convert from minutes to mills
	static unsigned long runTime = (unsigned long)timerIntervals[timerOption] * 60 * 1000;

	// Save btnPushCount[btn]
	static byte prevBtnPushCount[ArrayLength(btnPushCount)];
	for (byte x = 0; x < ArrayLength(btnPushCount); x++) {
		prevBtnPushCount[x] = startupHeat[x];
	}
	// Our Timer (is complicated)...
	for (byte x = 0; x < ArrayLength(timerState); x++) {
		if (timerState[x]) { // Timer is running
			if (debugEnabled) {
				Serial.print(F("Timer is running for: "));
				Serial.println(x);
			}
			if ((long)millis() - runTime >= timer[x]) {
				timer[x] = millis();
			}
			else {
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
				for (byte x = 0; x < ArrayLength(btnPushCount); x++) {
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
		}
		else {
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
