// Helper functions for ApHSC

#include "Const.h"
#include "Helper.h"
#include "EEPROM.h"

// Count of button presses for each particular side
static byte btn_push_count[] = { 0, 0 };

// Enable Auto Startup
static byte auto_startup;
// Auto Start Saved Heat Level (per side)
static byte startup_heat[] = { 0, 0 };

// Enable Timer
static byte timer_enabled;

// Selected timer duration (from kTimerIntervals)
static byte timer_option;
// Only run timer once
static byte timer_expired;

void Helper::init() {
	auto_startup = 0;
	timer_enabled = 0;
	timer_option = 0;
	timer_expired = 0;

	// Prepare EEPROM
	byte stored_ver = EEPROM.read(EEPROMVER);
	byte current_ver = (CURRENTVER);
	if (current_ver != stored_ver) {
		if (kMonitorEnabled) {
			Serial.println(F("EEPROM version mismatch!"));
		}
		for (uint16_t x = 0; x < EEPROM.length(); x++) {
			EEPROM.write(x, 0);
		}
		EEPROM.write(EEPROMVER, current_ver);
		EEPROM.write(AUTOSTARTUP, 0);
		EEPROM.write(TIMEROPTION, kTimerIntvReset);
		for (byte x = 0; x < ArrayLength(startup_heat); x++) {
			EEPROM.write(x + HEATLVLOFFSET, 0);
		}
		if (kMonitorEnabled) {
			Serial.println(F("EEPROM Cleared!"));
		}
	}
	// Auto Startup & Timer Feature
	auto_startup = EEPROM.read(AUTOSTARTUP);
	if (auto_startup) {
		if (kMonitorEnabled) {
			Serial.println(F("Auto Startup & Timer Feature Enabled."));
		}
		// Retrieve Saved Heat Level
		for (byte x = 0; x < ArrayLength(startup_heat); x++) {
			startup_heat[x] = EEPROM.read(x + HEATLVLOFFSET);
			if (startup_heat[x] >= 0 && startup_heat[x] <= 3) {
				btn_push_count[x] = EEPROM.read(x + HEATLVLOFFSET);
				if (kMonitorEnabled) {
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
					switch (btn_push_count[x]) {
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
						Serial.print(btn_push_count[x]);
						Serial.println(F("."));
						break;
					}
				}
				timer_enabled = 1;
			} else {
				// Clear Saved Heat Level
				EEPROM.write(x + HEATLVLOFFSET, 0);
				if (kMonitorEnabled) {
					Serial.println(F("Auto Startup Heat Level Cleared."));
				}
			}
		}
		// Retrieve timer option
		byte stored_tmr = EEPROM.read(TIMEROPTION);
		if (stored_tmr >= 0 && stored_tmr <= 3) {
			timer_option = stored_tmr;
			if (kMonitorEnabled) {
				Serial.print(F("Timer Option Set, Current Value: "));
				Serial.print(kTimerIntervals[timer_option]);
				if (kTimerIntervals[timer_option] == 1) {
					Serial.println(F(" Minute."));
				} else {
					Serial.println(F(" Minutes."));
				}
			}
		} else {
			if (kMonitorEnabled) {
				Serial.print(F("Timer Interval Out Of Range: "));
				Serial.println(timer_option);
			}
			// Reset timer option
			EEPROM.write(TIMEROPTION, kTimerIntvReset);
			timer_option = kTimerIntvReset;
			if (kMonitorEnabled) {
				Serial.println(F("Timer Option Reset."));
			}
		}
	}
}

// Listen for single and press+hold button pressed events, while de-bouncing!
void Helper::queryBtnState() {
	// Tracks current button state for each particular side
	static byte btn_state[] = { 0, 0 };
	// Tracks previous button state for each particular side
	static byte last_btn_state[] = { 0, 0 };

	// Single and Hold Button States
	static byte btn_press_single = 0;
	static byte btn_press_hold = 0;

	// De-bounce period to filter out button bounce
	const byte kDebounceDelay = 25;

	// Time to trigger button press+hold event
	const int kBtnHoldTime = 500;

	// Time button was triggered
	static unsigned long btn_trigger = 0;
	// Time button was last triggered
	static unsigned long last_btn_trigger[] = { 0, 0 };

	for (byte x = 0; x < ArrayLength(btn_state); x++) {
		btn_state[x] = digitalRead(kBtnPins[x]);
		// first button press
		if (btn_state[x] == HIGH && last_btn_state[x] == LOW) {
			if (kMonitorEnabled) {
				Serial.println(F("Button Triggered:"));
			}
			btn_trigger = millis();
		}
		// button held
		if (btn_state[x] == HIGH && last_btn_state[x] == HIGH) {
			if ((millis() - btn_trigger) > kBtnHoldTime) {
				btn_press_hold = 1;
			}
		}
		// button released
		if (btn_state[x] == LOW && last_btn_state[x] == HIGH) {
			if (((millis() - btn_trigger) > kDebounceDelay)
				&& btn_press_hold != 1) {
				if ((millis() - last_btn_trigger[x]) >= kDebounceDelay * 2) {
					btn_press_single = 1;
					last_btn_trigger[x] = millis();
				}
			}
			if (btn_press_hold == 1) {
				if (kMonitorEnabled) {
					Serial.print(F("Press+Hold Event "));
				}
				if (kBtnPins[x] == 2) {
					if (kMonitorEnabled) {
						Serial.println(F("- Driver Side."));
					}
					saveState(x);
				}
				if (kBtnPins[x] == 3) {
					if (kMonitorEnabled) {
						Serial.println(F("- Passenger Side."));
					}
					saveState(x);
				}
				btn_press_hold = 0;
			}
		}
		if (btn_press_single == 1
			&& (millis() - last_btn_trigger[x]) < kBtnHoldTime) {
			if (kMonitorEnabled) {
				Serial.print(F("Single Press Event "));
			}
			if (kBtnPins[x] == 2) {
				btn_push_count[0]++;
				if (kMonitorEnabled) {
					Serial.println(F("- Driver Side."));
				}
			}
			if (kBtnPins[x] == 3) {
				btn_push_count[1]++;
				if (kMonitorEnabled) {
					Serial.println(F("- Passenger Side."));
				}
			}
			btn_press_single = 0;
		}
		last_btn_state[x] = btn_state[x];
	}
}

// Reset Button Push Count if/when out of range.
void Helper::resetBtnPushCount() {
	// Maximum allowed number of button presses/per side
	static const byte kMaxBtnPushCount = ArrayLength(kStatusPins) / 2;
	for (byte x = 0; x < ArrayLength(btn_push_count); x++) {
		if (btn_push_count[x] > kMaxBtnPushCount) {
			if (kMonitorEnabled) {
				Serial.println(F("Button Press Counter Reset."));
			}
			btn_push_count[x] = 0;
		}
	}
}

// Toggles power ON/OFF in accordance to btn_push_count.
void Helper::togglePower() {
	if (btn_push_count[0] != 0 || btn_push_count[1] != 0) {
		power(1);
		if (kDebugEnabled) {
			Serial.println(F("Power is ON."));
		}
	} else {
		power(0);
		if (kDebugEnabled) {
			Serial.println(F("Power is OFF."));
		}
	}
	// Update heatTimer() if enabled (and not expired).
	if (timer_enabled && !timer_expired) {
		heatTimer();
		if (kDebugEnabled) {
			Serial.println(F("Checking on Timer."));
		}
	}
}

// Blinks onBoardLed to indicate HeartBeat per every 1 second.
// Use this to measure loop() speed.
void Helper::heartBeat() {
	static unsigned int led_status = LOW;
	static unsigned long led_blink_time = 0;
	// Possibly the most straightforward blink of all time (besides blink())
	if ((long)(millis() - led_blink_time) >= 0) {
		led_status = (led_status == HIGH ? LOW : HIGH);
		digitalWrite(kOnBoardLedPin, led_status);
		led_blink_time = millis() + kLedBlinkRate;
	}
}

/*
* Saves Auto Start option (along with timer) to EEPROM when (driver button)
* press+hold is triggered along with the currently selected heat level.
* Saves timer_option to EEPROM when (passenger button) press+hold is triggered.
* Disables Auto Start, resets timer_option and Heat Level if both heat levels
* are OFF when (driver button) press+hold is triggered.
* Resets timer_option to kTimerIntvReset when (passenger button) press+hold
* is triggered and (passenger) heat level is OFF.
*
* EEPROM is saved as such:
* [0:EEPROMver, 1:auto_startup, 2:timer_option, 3:drvHeatLevel, 4:pasHeatLevel]
*/
void Helper::saveState(byte btn) {
	if (btn == 0) { // Driver button press+hold
		if ((btn_push_count[0] == 0) && (btn_push_count[1] == 0)) {
			if (kMonitorEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Disabled."));
				Serial.println(F("Timer Interval Reset."));
			}
			EEPROM.write(AUTOSTARTUP, 0);
			EEPROM.write(TIMEROPTION, kTimerIntvReset);
			for (byte x = 0; x < ArrayLength(btn_push_count); x++) {
				EEPROM.write(x + HEATLVLOFFSET, 0);
			}
			if (kMonitorEnabled) {
				Serial.println(F("Auto Startup Heat Level Cleared."));
			}
			blink(btn, 1); // blink OFF Pattern
		}
		if ((btn_push_count[0] >= 1 && btn_push_count[0] <= 3)
			|| (btn_push_count[1] >= 1 && btn_push_count[1] <= 3)) {
			if (kMonitorEnabled) {
				Serial.println(F("Auto Startup & Timer Feature Enabled."));
			}
			EEPROM.write(AUTOSTARTUP, 1);
			for (byte x = 0; x < ArrayLength(btn_push_count); x++) {
				EEPROM.write(x + HEATLVLOFFSET, btn_push_count[x]);
			}
			if (kMonitorEnabled) {
				Serial.println(F("Auto Startup Heat Levels Saved."));
			}
			blink(btn, 0); // blink ON Pattern
		}
	}
	if (btn == 1) { // Passenger button press+hold
		if (btn_push_count[btn] >= 1 && btn_push_count[btn] <= 3) {
			EEPROM.write(TIMEROPTION, btn_push_count[btn] - 1);
		} else {
			if (kMonitorEnabled) {
				Serial.println(F("Heat Level is OFF, Timer Interval Reset."));
			}
			EEPROM.write(TIMEROPTION, kTimerIntvReset);
		}
		blink(btn, 2); // blink TOGGLE Pattern
	}
}

/*
* Blinks LED for the current heat level in the desired pattern.
* Powers off unit before blinking status LEDs.
*/
void Helper::blink(byte btn, byte pattern) {
	// Save btn_push_count[btn] to save our current state.
	byte prev_btn_push_count = 0;
	prev_btn_push_count = btn_push_count[btn];

	// blink Patterns:
	//        {onTime, offTime}:               ON    -   OFF    -  TOGGLE   -  ERROR
	const unsigned long blinkPatterns[] = { 1500, 500, 350, 1500, 1000, 1000, 350, 350 };

	const int blinkDelay = 500;
	unsigned long blinkTimer = millis();
	unsigned long patternTimer = millis() + blinkDelay;

	if (pattern >= 3) {
		pattern = 3;
	}
	if (kMonitorEnabled) {
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
	* Setting btn_push_count[btn] to 0 and calling togglePower()
	* prior to running the blink patterns ensures we don't
	* toggle heat on and off as we blink.
	*/
	btn_push_count[btn] = 0;
	togglePower();

	byte blink_pin = 0;
	int pin_offset = 0;

	for (byte x = 0; x < ArrayLength(kBtnPins); x++) {
		if (prev_btn_push_count >= 1) {
			blink_pin = prev_btn_push_count;
		} else {
			blink_pin = 2;
		}
		if (btn == 0) {
			pin_offset = -1;
		} else {
			pin_offset = 2;
		}
		// Every time blink() gets called;
		// Wait for blinkDelay before we run the blink patterns.
		while (millis() - blinkTimer < blinkDelay) {
			if (kDebugEnabled) {
				Serial.println(millis() - blinkTimer);
			}
		}
		if (kDebugEnabled) {
			Serial.println(F("HIGH"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2)])) {
			digitalWrite(kStatusPins[blink_pin] + pin_offset, HIGH);
			if (kDebugEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
		if (kDebugEnabled) {
			Serial.println(F("LOW"));
		}
		while (millis() - patternTimer < (blinkPatterns[(pattern * 2) + 1])) {
			digitalWrite(kStatusPins[blink_pin] + pin_offset, LOW);
			if (kDebugEnabled) {
				Serial.println(millis() - patternTimer);
			}
		}
		patternTimer = millis();
	}
	// Restore btn_push_count[btn] to restore program state before blink().
	btn_push_count[btn] = prev_btn_push_count;
	togglePower();
}

// Toggles our ON/OFF signal.
void Helper::power(boolean state) {
	if (state) {
		digitalWrite(kOnSignalPin, HIGH);
		toggleHeat(1);
	} else {
		toggleHeat(0);
		digitalWrite(kOnSignalPin, LOW);
	}
}

// Toggles heat ON/OFF.
void Helper::toggleHeat(boolean state) {
	if (state) {
		for (byte x = 0; x < ArrayLength(btn_push_count); x++) {
			heatLevel(btn_push_count[x], x);
		}
	} else {
		for (byte x = 0; x < ArrayLength(kStatusPins); x++) {
			digitalWrite(kStatusPins[x], LOW);
		}
	}
}

// Toggles heat level per side.
void Helper::heatLevel(byte level, byte side) {
	for (byte n = 0; n < ArrayLength(kStatusPins) / 2; n++) {
		if (side == 0 && level > 0) {
			digitalWrite(kStatusPins[n], LOW);
			digitalWrite(kStatusPins[level] - 1, HIGH);
		}
		if (side == 0 && level == 0) {
			digitalWrite(kStatusPins[n], LOW);
		}
		if (side == 1 && level > 0) {
			digitalWrite(kStatusPins[n] + 3, LOW);
			digitalWrite(kStatusPins[level] + 2, HIGH);
		}
		if (side == 1 && level == 0) {
			digitalWrite(kStatusPins[n] + 3, LOW);
		}
	}
}

/*
* Start a timer if Auto Start is enabled and timer_enabled is true.
* Switch from saved Auto Start heatLevel to OFF after millis() - kRunTime >= timer.
* Stop all timers if btn_push_count changes while a timer is active for either side.
*/
void Helper::heatTimer() {
	// Track timer state
	static byte timer_state[] = { 0, 0 };
	// Timer trigger (in mills)
	static unsigned long timer[] = { 0, 0 };
	// Convert from minutes to mills
	static unsigned long kRunTime =
		(unsigned long)kTimerIntervals[timer_option] * 60 * 1000;

	// Save btn_push_count[btn]
	static byte kPrevBtnPushCount[ArrayLength(btn_push_count)];
	for (byte x = 0; x < ArrayLength(btn_push_count); x++) {
		kPrevBtnPushCount[x] = startup_heat[x];
	}
	// Our Timer (is complicated)...
	for (byte x = 0; x < ArrayLength(timer_state); x++) {
		if (timer_state[x]) { // Timer is running
			if (kDebugEnabled) {
				Serial.print(F("Timer is running for: "));
				Serial.println(x);
			}
			if ((long)millis() - kRunTime >= timer[x]) {
				timer[x] = millis();
			} else {
				timer_state[x] = 0; // Reset timer state
				btn_push_count[x] = 0; // Reset btn_push_count
				if (kMonitorEnabled) {
					Serial.print(F("Timer has been reset for: "));
					Serial.println(x);
				}
			}
		}
		// btn_push_count changed
		if (kPrevBtnPushCount[x] != btn_push_count[x]) {
			timer_state[x] = 0;
			if (!timer_state[x]) {
				for (byte x = 0; x < ArrayLength(btn_push_count); x++) {
					btn_push_count[x] = 0;
					timer_state[x] = 0;
				}
			}
			if (kMonitorEnabled) {
				Serial.print(F("btnPushCount[btn] has changed for: "));
				Serial.println(x);
			}
		}
		// Don't run timer if already OFF
		if (btn_push_count[x] == 0) {
			timer_state[x] = 0;
			if (kDebugEnabled) {
				Serial.print(F("This side's timer is disabled: "));
				Serial.println(x);
			}
		} else {
			timer_state[x] = 1;
		}
		// Both timers have been reset
		if (!timer_state[0] && !timer_state[1]) {
			// Both buttons have been reset
			if (!btn_push_count[0] && !btn_push_count[1]) {
				timer_expired = 1;
				if (kMonitorEnabled) {
					Serial.println(F("Timer has expired."));
				}
			}
		}
	}
}
