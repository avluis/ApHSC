/*
  Heated Seat Switching (powered by Arduino Nano)
  By: Luis E Alvarado
  Contact: admin@avnet.ws or alvaradorocks@gmail.com
  License: GNU LGPL 2.1+
  
  Software: https://github.com/avluis/ArduinoHeatedSeatController
  Hardware: https://github.com/avluis/ArduinoHeatedSeatController-Hardware
  
  Purpose: To drive a ComfortHeat "Automotive Carbon Fiber Seat Heaters"
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
*/

//  5 Seconds =   5000 Milliseconds
// 15 Seconds =  15000 Milliseconds
// 30 Seconds =  30000 Milliseconds
//  1 Minute  =  60000 Milliseconds
// 10 Minutes = 600000 Milliseconds
// 15 minutes = 900000 Milliseconds

// Using EEPROM library
#include <EEPROM.h>

// Dashboard Buttons to monitor
const byte buttonPin[] = {2, 3};

// Output to ULN2003A & M54564P
const byte statusPin[] = {4, 5, 6, 7, 8, 9};
// Pin 10 is only for ON Signal so we keep it out of the array
const byte onSignalPin = 10;

// On-Board LED (on Arduino)
const byte onBoardLedPin = 13;

// Count of button presses for each particular side
byte buttonPushCounter[] = {0, 0};
// Tracks current button state for each particular side
byte buttonState[] = {0, 0};
// Tracks previous button state for each particular side
byte lastButtonState[] = {0, 0};

// Debounce period to filter out button bounce
const byte debounceDelay = 25;
// Last debounce time
unsigned long lastDebounceTime = 0;

// Single and Hold Button States
byte buttonPressSingle = 0;
byte buttonPressHold = 0;
// Time to trigger button press+hold event
const int buttonHoldTime = 500;

// Time button was triggered
unsigned long buttonTrigger = 0;
// Time button was last triggered
unsigned long lastButtonTrigger[] = {0, 0};

// Enable timer
byte timerEnabled = 0;

// Tracks if the timer has already been called
byte timerState[] = {0, 0};

// How long do we wait before starting the timer
const int timerDelay = 15000;

// How far do we count before switching heat level (from HI to MID)
const long timerInterval = 900000;

// When was the timer triggered (in mills)
unsigned long timerTrigger[] = {0, 0};
// When was the timer last triggered
unsigned long lastTimerTrigger[] = {0, 0};

// Enable Auto Startup
byte autoStartup = 0;

// Enable Serial - Handy for debugging
byte serialEnabled = 1;

void setup() {
  // Initializing On-Board LED as an output
  pinMode(onBoardLedPin, OUTPUT);
  // Initializing On Signal Pin as output
  pinMode(onSignalPin, OUTPUT);
  // Initializing Buttons as inputs
  for (byte x = 0; x < 2; x++){
  pinMode(buttonPin[x], INPUT);
  }
  // Initializing Status Pins as outputs
  for (byte x = 0; x < 6; x++){
  pinMode(statusPin[x], OUTPUT);
  }
  // Set up serial
  if (serialEnabled){
    Serial.begin(9600);
    Serial.println("Heat Controller up and running.");
  }
  // Enable Timer
  timerEnabled = EEPROM.read(0);
  if (timerEnabled){
    if (serialEnabled){
      Serial.println("Timer Feature Enabled.");
    }
    timerEnabled = 1;
  }
  // Auto Startup
  autoStartup = EEPROM.read(1);
  if (autoStartup){
    if (serialEnabled){
      Serial.println("Auto Startup Feature Enabled.");
    }
    for (byte x = 0; x < 2; x++){
    buttonPushCounter[x] = 1;
    }
  }
}

void loop() {
  // Read the state of each button
  QueryButtonState();
  // Reset counters when buttonPushCounter == 4
  ResetPushCounter();
  // Toggle power when buttonPushCounter > 1
  TogglePower();
  // Start a timer if heat level is set to HIGH for more than timerDelay
  HeatTimer();
}

// If the number of button presses reaches
// a certain number, reset buttonPushCounter to 0
void ResetPushCounter(){
  for (byte x = 0; x < 2; x++){
    if (buttonPushCounter[x] == 4){
      if (serialEnabled){
        Serial.println("Button Press Counter Reset.");
      }
      buttonPushCounter[x] = 0;
    }
  }
}

// Listens for button presses, while debouncing the button input
// Tracks the number of presses respective to each button (side)
void QueryButtonState(){
  for (byte x = 0; x < 2; x++){
    buttonState[x] = digitalRead(buttonPin[x]);
    if (buttonState[x] == HIGH && lastButtonState[x] == LOW){ // first button press
      if (serialEnabled){
        Serial.println("Button Triggered: ");
      }
      buttonTrigger = millis();
    }
    if (buttonState[x] == HIGH && lastButtonState[x] == HIGH){ // button held
      if ((millis() - buttonTrigger) > buttonHoldTime){
      buttonPressHold = 1;
      }
    }
    if (buttonState[x] == LOW && lastButtonState[x] == HIGH){ // button released
      if (((millis() - buttonTrigger) > debounceDelay) && buttonPressHold != 1){
        if ((millis() - lastButtonTrigger[x]) >= debounceDelay * 2){
          buttonPressSingle = 1;
          lastButtonTrigger[x] = millis();
        }
      }
      if (buttonPressHold == 1){
        if (serialEnabled){
          Serial.print("Press+Hold Event ");
        }
        if (buttonPin[x] == 2){
          SaveState(x);
          if (serialEnabled){
            Serial.println("- Driver Side.");
          }
        }
        if (buttonPin[x] == 3){
          SaveState(x);
          if (serialEnabled){
            Serial.println("- Passenger Side.");
          }
        }
        buttonPressHold = 0;
      }
    }
    if (buttonPressSingle == 1 && (millis() - lastButtonTrigger[x]) < buttonHoldTime){
      if (serialEnabled){
        Serial.print("Single Press Event ");
      }
      if (buttonPin[x] == 2){
        buttonPushCounter[0]++;
        if (serialEnabled){
          Serial.println("- Driver Side.");
        }
      }
      if (buttonPin[x] == 3){
        buttonPushCounter[1]++;
        if (serialEnabled){
          Serial.println("- Passenger Side.");
        }
      }
      buttonPressSingle = 0;
    }
    lastButtonState[x] = buttonState[x];
  }
}

// Toggles heat ON in accordance to the number of button presses
// on each respective side/button.
// Toggles heat off if the number of presses loops to 0.
void TogglePower(){
  if (buttonPushCounter[0] != 0 || buttonPushCounter[1] != 0){
    Power(1);
  } else {
    Power(0);
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
      HeatLevel(buttonPushCounter[x], x);
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

// Start a timer if heat level is set to HIGH for more than timerDelay
// Switch heatLevel from HIGH to MID after timerTrigger > timerInterval
void HeatTimer(){
  if (timerEnabled){
    for (byte x = 0; x < 2; x++){
      if (timerState[x] == 0){
        if (buttonPushCounter[x] != 1){
          timerTrigger[x] = millis();
        }
        if (buttonPushCounter[x] == 1){
          timerTrigger[x] = millis();
          if (serialEnabled){
            Serial.println("Timer Trigger Updated.");
          }
        }
      }
      if (buttonPushCounter[x] == 1){
        timerState[x] = 1;
        if (((millis() - timerTrigger[x]) - lastTimerTrigger[x]) >= timerDelay){
          if ((millis() - timerTrigger[x]) - lastTimerTrigger[x] >= timerInterval){
            buttonPushCounter[x] = 0;
            timerState[x] = 0;
            TogglePower();
            buttonPushCounter[x] = 2;
            TogglePower();
          }
        }
      }
      if (timerState[x] == 1 && buttonPushCounter[x] > 1){
        timerState[x] = 0;
        lastTimerTrigger[x] = timerTrigger[x];
        TogglePower();
        if (serialEnabled){
          Serial.println("Timer State Reset.");
        }
      }
      if (timerState[x] == 0 && buttonPushCounter[x] > 1) {
        timerTrigger[x] = millis();
      }
    }
  }
}

// Saves current buttonPushCounter to EEPROM when press+hold is triggered
void SaveState(byte button){
  if (button == 0){
    int savedTimer = EEPROM.read(0);
    if (savedTimer == 0){
      if (serialEnabled){
        Serial.println("Timer Enabled.");
      }
      EEPROM.write(0, 1);
      timerEnabled = 1;
      Blink(button, 0);
      HeatTimer();
    }
    if (savedTimer == 1){
      if (serialEnabled){
        Serial.println("Timer Disabled.");
      }
      EEPROM.write(0, 0);
      timerEnabled = 0;
      Blink(button, 1);
      HeatTimer();
    }
  }
  if (button == 1){
    int savedAutoStartup = EEPROM.read(1);
    if (savedAutoStartup == 0) {
      if (serialEnabled){
        Serial.println("Auto Startup Enabled.");
      }
      EEPROM.write(1, 1);
      Blink(button, 0);
    }
    if (savedAutoStartup == 1) {
      if (serialEnabled){
        Serial.println("Auto Startup Disabled.");
      }
      EEPROM.write(1, 0);
      Blink(button, 1);
    } 
  }
}

void Blink(byte button, byte pattern){
  byte previousButtonPushCounter = 0;
  previousButtonPushCounter = buttonPushCounter[button];
  if (serialEnabled){
    Serial.println("Blink Pattern: ");
  }
  if (pattern == 0){
    if (serialEnabled){
      Serial.println("ON.");
    }
    buttonPushCounter[button] = 0;
    TogglePower();
    delay(500);
    buttonPushCounter[button] = 1;
    TogglePower();
    delay(1500);
    buttonPushCounter[button] = 0;
    TogglePower();
    delay(500);
    buttonPushCounter[button] = 1;
    TogglePower();
    delay(1500);
  }
  if (pattern == 1){
    if (serialEnabled){
      Serial.println("OFF.");
    }
    buttonPushCounter[button] = 0;
    TogglePower();
    delay(1500);
    buttonPushCounter[button] = 1;
    TogglePower();
    delay(500);
    buttonPushCounter[button] = 0;
    TogglePower();
    delay(1500);
    buttonPushCounter[button] = 1;
    TogglePower();
    delay(500);
  }
  if (pattern == 2){
    if (serialEnabled){
      Serial.println("TOGGLE.");
    }
    buttonPushCounter[button] = 0;
    TogglePower();
    delay(1000);
    buttonPushCounter[button] = 1;
    TogglePower();
    delay(1000);
    buttonPushCounter[button] = 0;
    TogglePower();
    delay(1000);
    buttonPushCounter[button] = 1;
    TogglePower();
    delay(1000);
  }
  buttonPushCounter[button] = previousButtonPushCounter;
  TogglePower();
}
