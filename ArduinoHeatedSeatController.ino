/*
  Heated Seat Switching (powered by Arduino Nano)
  By: Luis E Alvarado
  Contact: admin@avnet.ws or alvaradorocks@gmail.com
  License: GNU LGPL 2.1+

  Software: https://github.com/avluis/ArduinoHeatedSeatController
  Hardware: https://github.com/avluis/ArduinoHeatedSeatController-Hardware

  Purpose:
  To drive a ComfortHeat "Automotive Carbon Fiber Seat Heaters"
  by Rostra with the stock control panel of a vehicle, in my case, a 2011 Suzuki Kizashi
  [Rostra Kit: 250-1872 (Universal Kit. Double thumb-dial Switch)
  Install Instructions: http://www.rostra.com/manuals/250-1870_Form5261.pdf]

  This sketch handles the logic required to control a 4-stage heated seat module.
  The stages are HIGH, MEDIUM, LOW and OFF. OFF is the default-start state.
  Indication of stages is done via LEDs for HIGH, MEDIUM and LOW.
  Vehicle wiring is ground based and built-in LEDs turn on when grounded.
  Control module is positive (+12v) signal switching.

  Grounding of LEDs is handled by a ULN2003A.
  Heat level switching is handled by a M54564P.
  ULN2003A & M54564P input pins are driven by the Arduino's Digital Output Pins.

  INPUT:
  Pin 2 & 3 for the Driver and Passenger Heat Buttons.

  OUTPUT:
  Pin 4, 5 and 6 for Driver side LED/Heat.
  Pin 7, 8 and 9 for Passenger side LED/Heat.
  Pin 10 is the ON signal for the Controller.

  Guide for the nonprogrammer:
  Auto Startup and Timer - If activated, Heat Controller will start up when it is powered
  ON at a previously saved heat level. Additionally, a timer will run for a preset time
  from HIGH heat to MEDIUM heat. If Auto Startup is enabled, the timer can be turned off
  temporarily (until next power on) if any of the heated seats is turned off via any of
  the two buttons.
  The timer will run only once (once timer has expired, it will not repeat), only if autostart
  is enabled and only for the seat set to HIGH heat whenever autostart is enabled.
  To enable Auto Start, set heat level where desired then press and hold the driver side button
  for a few seconds to set the heat level and enable auto start.
  Note that the timer will only run for the side set to HIGH heat.
  So if you want the timer for both sides, set both heat levels to HIGH.
  Pressing and holding the driver side button again will clear the currently set heat level and
  disable auto start.
  To set how long the timer will run for, press and hold the passenger side button for a few seconds
  with the following in mind: HIGH = 15 minutes, MEDIUM = 10 minutes and LOW = 5 minutes.
  Please note that disabling auto start resets the timer back to 15 minutes.
*/


// Using EEPROM library
#include <EEPROM.h>

// Dashboard Buttons to monitor
const byte btnPin[] = {2, 3};

// Output to ULN2003A & M54564P
const byte statusPin[] = {4, 5, 6, 7, 8, 9};
// Pin 10 is only for ON Signal so we keep it out of the array
const byte onSignalPin = 10;

// On-Board LED (on Arduino)
const byte onBoardLedPin = 13;

// Count of button presses for each particular side
byte btnPushCount[] = {0, 0};
// Tracks current button state for each particular side
byte btnState[] = {0, 0};
// Tracks previous button state for each particular side
byte lastBtnState[] = {0, 0};

// Debounce period to filter out button bounce
const byte debounceDelay = 25;

// Single and Hold Button States
byte btnPressSingle = 0;
byte btnPressHold = 0;
// Time to trigger button press+hold event
const int btnHoldTime = 500;

// Time button was triggered
unsigned long btnTrigger = 0;
// Time button was last triggered
unsigned long lastBtnTrigger[] = {0, 0};

// Enable Timer
byte timerEnabled = 0;

// Only run timer once
byte timerExpired = 0;

// Tracks if the timer has already been called
byte timerState[] = {0, 0};

// When was the timer triggered (in mills)
unsigned long timerTrigger[] = {0, 0};

// How long do we wait before starting the timer
const int timerDelay = 15000;

// Track duration of timer (from timerIntervals)
byte timerOption = 0;

// How far do we count before switching heat level (from HI to MID)
const long timerIntervals[] = {900000, 600000, 300000, 15000};

// Enable Auto Startup
byte autoStartup = 0;
// Auto Start Saved Heat Level
byte startupHeat[] = {0, 0};

// Blink Patterns
const long blinkPatterns[] = {1500, 500, 500, 1500, 1000, 500};

// Enable Serial - Handy for debugging
byte serialEnabled = 0;

void setup() {
  // Initializing On-Board LED as an output
  pinMode(onBoardLedPin, OUTPUT);
  // Initializing On Signal Pin as output
  pinMode(onSignalPin, OUTPUT);
  // Initializing Buttons as inputs
  for (byte x = 0; x < 2; x++){
    pinMode(btnPin[x], INPUT);
  }
  // Initializing Status Pins as outputs
  for (byte x = 0; x < 6; x++){
    pinMode(statusPin[x], OUTPUT);
  }
  // Set up serial
  if (serialEnabled){
    Serial.begin(9600);
    Serial.println("Heat Controller Up & Running.");
  }
  // Retrieve timer interval
  timerOption = EEPROM.read(0);
  if (timerOption >= 0 && timerOption <= 3){
    if (serialEnabled){
      Serial.print("Timer Interval Set, Current Value: ");
      Serial.print(timerIntervals[timerOption]);
      Serial.println(" Milliseconds.");
    }
  } else {
    if (serialEnabled){
      Serial.print("Timer Interval Out Of Range: ");
      Serial.println(timerOption);
    }
    // Reset timer interval
    EEPROM.write(0, 0);
    timerOption = 3;
    if (serialEnabled){
      Serial.println("Timer Interval Reset.");
    }
  }
  // Auto Startup & Timer Feature
  autoStartup = EEPROM.read(1);
  if (autoStartup){
    if (serialEnabled){
      Serial.println("Auto Startup & Timer Feature Enabled.");
    }
    timerEnabled = 1;
    // Retrieve Saved Heat Level
    for (byte x = 0; x < 2; x++){
      startupHeat[x] = EEPROM.read(x + 2);
      if (startupHeat[x] >= 0 && startupHeat[x] <= 3){
        btnPushCount[x] = EEPROM.read(x + 2);
        if (serialEnabled){
          Serial.print("Heat Level - ");
          if (x == 0){
            Serial.print("Driver Side: ");
          }
          if (x == 1){
            Serial.print("Passenger Side: ");
          }
          if (btnPushCount[x] == 0){
            Serial.println("OFF");
          }
          if (btnPushCount[x] == 1){
            Serial.println("HIGH");
          }
          if (btnPushCount[x] == 2){
            Serial.println("MEDIUM");
          }
          if (btnPushCount[x] == 3){
            Serial.println("LOW");
          }
        }
      } else {
      // Clear Saved Heat Level
        EEPROM.write(x + 2, 0);
        if (serialEnabled){
          Serial.println("Auto Startup Heat Level Cleared.");
        }
      }
    }
  }
}

void loop() {
  // Read the state of each button
  QueryButtonState();
  // Reset counters when btnPushCount == 4
  ResetPushCounter();
  // Toggle power when btnPushCount > 1
  TogglePower();
}

// If the number of button presses reaches
// a certain number, reset btnPushCount to 0
void ResetPushCounter(){
  for (byte x = 0; x < 2; x++){
    if (btnPushCount[x] == 4){
      if (serialEnabled){
        Serial.println("Button Press Counter Reset.");
      }
      btnPushCount[x] = 0;
    }
  }
}

// Listens for button presses, while debouncing the button input
// Tracks the number of presses respective to each button (side)
void QueryButtonState(){
  for (byte x = 0; x < 2; x++){
    btnState[x] = digitalRead(btnPin[x]);
    if (btnState[x] == HIGH && lastBtnState[x] == LOW){ // first button press
      if (serialEnabled){
        Serial.println("Button Triggered:");
      }
      btnTrigger = millis();
    }
    if (btnState[x] == HIGH && lastBtnState[x] == HIGH){ // button held
      if ((millis() - btnTrigger) > btnHoldTime){
        btnPressHold = 1;
      }
    }
    if (btnState[x] == LOW && lastBtnState[x] == HIGH){ // button released
      if (((millis() - btnTrigger) > debounceDelay) && btnPressHold != 1){
        if ((millis() - lastBtnTrigger[x]) >= debounceDelay * 2){
          btnPressSingle = 1;
          lastBtnTrigger[x] = millis();
        }
      }
      if (btnPressHold == 1){
        if (serialEnabled){
          Serial.print("Press+Hold Event ");
        }
        if (btnPin[x] == 2){
          if (serialEnabled){
            Serial.println("- Driver Side.");
          }
          SaveState(x);
        }
        if (btnPin[x] == 3){
          if (serialEnabled){
            Serial.println("- Passenger Side.");
          }
          SaveState(x);
        }
        btnPressHold = 0;
      }
    }
    if (btnPressSingle == 1 && (millis() - lastBtnTrigger[x]) < btnHoldTime){
      if (serialEnabled){
        Serial.print("Single Press Event ");
      }
      if (btnPin[x] == 2){
        btnPushCount[0]++;
        if (serialEnabled){
          Serial.println("- Driver Side.");
        }
      }
      if (btnPin[x] == 3){
        btnPushCount[1]++;
        if (serialEnabled){
          Serial.println("- Passenger Side.");
        }
      }
      btnPressSingle = 0;
    }
    lastBtnState[x] = btnState[x];
  }
}

// Toggles heat ON in accordance to the number of button presses
// on each respective side/button.
// Updates heat timer if enabled (and not expired).
void TogglePower(){
  if (btnPushCount[0] != 0 || btnPushCount[1] != 0){
    Power(1);
  } else {
    Power(0);
  }
  if (timerEnabled == 1 && timerExpired == 0){
    HeatTimer();
  }
}

// Toggles the On-Board LED and calls enableHeat()/disableHeat()
// according to power state - sends our ON/OFF signal as well.
void Power(boolean state){
  if (state){
    digitalWrite(onBoardLedPin, HIGH);
    digitalWrite(onSignalPin, HIGH);
    ToggleHeat(1);
  } else {
    ToggleHeat(0);
    digitalWrite(onSignalPin, LOW);
    digitalWrite(onBoardLedPin, LOW);
  }
}

// Enables/Disables heat when called.
// Sends heat level and side to heatLevel() if heat is enabled.
void ToggleHeat(boolean state){
  if (state){
    for (byte x = 0; x < 2; x++){
      HeatLevel(btnPushCount[x], x);
    }
  } else {
    for (byte x = 0; x < 6; x++){
      digitalWrite(statusPin[x], LOW);
    }
  }
}

// Receives heat level and heat side arguments.
// Uses this data to turn off/on the respective pin(s)
void HeatLevel(byte level, byte side){
  for (byte n = 0; n < 3; n++){
    if (side == 0 && level > 0){
      digitalWrite(statusPin[n], LOW);
      digitalWrite(statusPin[level] - 1, HIGH);
    }
    if (side == 0 && level == 0) {
      digitalWrite(statusPin[n], LOW);
    }
    if (side == 1 && level > 0){
      digitalWrite(statusPin[n] + 3, LOW);
      digitalWrite(statusPin[level] + 2, HIGH);
    }
    if (side == 1 && level == 0) {
      digitalWrite(statusPin[n] + 3, LOW);
    }
  }
}

// Start a timer if heat level is set to HIGH for more than timerDelay.
// Switch heatLevel from HIGH to MID after timerTrigger > timerInterval.
// Stop timer if btnPushCount == 0 while timer is active for either side.
void HeatTimer(){
  for (byte x = 0; x < 2; x++){
    if (timerState[x] == 0){
      if (btnPushCount[x] == 1){
        timerState[x] = 1;
        timerTrigger[x] = millis();
        if (serialEnabled){
          Serial.print("Timer Triggered: ");
          Serial.print(timerTrigger[x]);
          Serial.println(" Milliseconds.");
        }
      }
      if (btnPushCount[x] > 1){
        timerState[x] = 0;
        timerTrigger[x] = 0;
      }
    }
    if (timerState[x] == 1){
      if (btnPushCount[x] == 1){
        if ((millis() - timerTrigger[x]) >= timerDelay){
          if ((millis() - timerTrigger[x]) >= timerIntervals[timerOption]){
            timerState[x] = 0;
            timerTrigger[x] = 0;
            btnPushCount[x] = 2;
          }
        }
      }
      if ((timerState[0] == 1 && timerState[1] == 0) || (timerState[0] == 0 && timerState[1] == 1)){
        if ((btnPushCount[0] >= 1 && btnPushCount[1] == 0) || (btnPushCount[0] == 0 && btnPushCount[1] >= 1)){
          timerState[x] = 0;
          timerTrigger[x] = 0;
          timerExpired = 1;
          if (serialEnabled){
            Serial.println("Timer Manually Reset.");
          }
        }
      }
      if (btnPushCount[x] > 1){
        timerState[x] = 0;
        TogglePower();
        if (btnPushCount[0] >= 2 && btnPushCount[1] >= 2){
          timerExpired = 1;
          if (serialEnabled){
            Serial.println("Timer Expired.");
          }
        }
      }
    }
  }
}

// Saves auto start option (along with timer) to EEPROM when (driver)
// press+hold is triggered along with the currently selected heat level.
// Saves timer duration to EEPROM when (passenger) press+hold is triggered.
void SaveState(byte btn){
  if (btn == 0){
    if ((btnPushCount[0] >= 1 && btnPushCount[0] <= 3) || (btnPushCount[1] >= 1 && btnPushCount[1] <= 3)){
      if (serialEnabled){
        Serial.println("Auto Startup & Timer Feature Enabled.");
      }
      EEPROM.write(1, 1);
      for (byte x = 0; x < 2; x++){
        EEPROM.write(x + 2, btnPushCount[x]);
      }
      if (serialEnabled){
        Serial.println("Auto Startup Heat Level Saved.");
      }
      Blink(btn, 0);
    } else {
      if (serialEnabled){
        Serial.println("Auto Startup & Timer Feature Disabled.");
        Serial.println("Timer Interval Reset.");
      }
      EEPROM.write(0, 0);
      EEPROM.write(1, 0);
      for (byte x = 0; x < 2; x++){
        EEPROM.write(x + 2, 0);
      }
      if (serialEnabled){
        Serial.println("Auto Startup Heat Level Cleared.");
      }
      timerExpired = 1;
      Blink(btn, 1);
    }
  }
  if (btn == 1){
    int savedTimerOption = EEPROM.read(0);
    if (btnPushCount[btn] >= 1 && btnPushCount[btn] <= 3){
      EEPROM.write(0, btnPushCount[btn] - 1);
      savedTimerOption = EEPROM.read(0);
      if (serialEnabled){
        Serial.print("New Timer Interval: ");
        Serial.print(timerIntervals[savedTimerOption]);
        Serial.println(" Milliseconds.");
      }
    } else {
      if (serialEnabled){
        Serial.println("Heat Level is OFF, Timer Interval Reset.");
      }
      EEPROM.write(0, 3);
    }
    Blink(btn, 2);
  }
}

void Blink(byte btn, byte pattern){
  byte prevBtnPushCount = 0;
  prevBtnPushCount = btnPushCount[btn];
  if (serialEnabled){
    Serial.print("Blink Pattern: ");
  }
  if (pattern == 0){
    if (serialEnabled){
      Serial.println("ON.");
    }
  }
  if (pattern == 1){
    if (serialEnabled){
      Serial.println("OFF.");
    }
  }
  if (pattern == 2){
    if (serialEnabled){
      Serial.println("TOGGLE.");
    }
  }
  btnPushCount[btn] = 0;
  TogglePower();
  delay(500);
  for (byte x = 0; x < 2; x++){
    if (btn == 0){
      if (prevBtnPushCount >= 1){
        digitalWrite(statusPin[prevBtnPushCount] - 1, HIGH);
        delay(blinkPatterns[(pattern * 2)]);
        digitalWrite(statusPin[prevBtnPushCount] - 1, LOW);
        delay(blinkPatterns[(pattern * 2) + 1]);
      } else {
        digitalWrite(statusPin[2] - 1, HIGH);
        delay(blinkPatterns[(pattern * 2)]);
        digitalWrite(statusPin[2] - 1, LOW);
        delay(blinkPatterns[(pattern * 2) + 1]);
      }
    }
    if (btn == 1){
      if (prevBtnPushCount >= 1){
        digitalWrite(statusPin[prevBtnPushCount] + 2, HIGH);
        delay(blinkPatterns[(pattern * 2)]);
        digitalWrite(statusPin[prevBtnPushCount] + 2, LOW);
        delay(blinkPatterns[(pattern * 2) + 1]);
      } else {
        digitalWrite(statusPin[2] + 2, HIGH);
        delay(blinkPatterns[(pattern * 2)]);
        digitalWrite(statusPin[2] + 2, LOW);
        delay(blinkPatterns[(pattern * 2) + 1]);
      }
    }
  }
  btnPushCount[btn] = prevBtnPushCount;
  TogglePower();
}
