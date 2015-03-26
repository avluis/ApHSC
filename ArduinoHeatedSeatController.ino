/*
  Heated Seat Switching (powered by Arduino Nano)
  By: Luis E Alvarado
  Contact: admin@avnet.ws or alvaradorocks@gmail.com
  License: GNU LGPL 2.1+
  
  GitHub: https://github.com/avluis/ArduinoHeatedSeatController
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
const byte debounceDelay = 50;
// Time button is held to trigger secondary action
const byte buttonHoldTime = 2500;
// Last debounce time
unsigned long lastDebounceTime = 0;

// Time button was pressed/released
unsigned long buttonPressed = 0;
unsigned long buttonDepressed = 0;

// Ignore button release event in case of press+hold
boolean ignoreButton = false;

// How far do we count before switching heat level (from HI to MID)
// 15 Seconds =  15000 Milliseconds
// 30 Seconds =  30000 Milliseconds
//  1 Minute  =  60000 Milliseconds
// 10 Minutes = 600000 Milliseconds
// 15 minutes = 900000 Milliseconds
const long timerIntervals[] = {15000, 60000};

// Tracks if the timer has already been called
byte timerState[] = {0, 0};

// How long do we wait before starting the timer
const int timerDelay = 15000;

// When was the timer triggered (in mills)
unsigned long timerTrigger[] = {0, 0};

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
  Serial.begin(9600);
  Serial.println("Up and running.");
}

void loop() {
  // Read the state of each button
  QueryButtonState();
  // Reset counters when buttonPushCounter > 4
  ResetPushCounter();
  // Toggle power when buttonPushCounter > 1
  TogglePower();
  // Start a timer if heat level is set to HIGH for more than timerDelay
  //Timer(); // Testing
}

// If the number of button presses reaches
// a certain number, reset buttonPushCounter to 0
void ResetPushCounter(){
  for (byte x = 0; x < 2; x++){
    if (buttonPushCounter[x] == 4){
      buttonPushCounter[x] = 0;
    }
  }
}

// Listens for button presses, while debouncing the button input
// Tracks the number of presses respective to each button (side)
void QueryButtonState(){
  for (byte x = 0; x < 2; x++){
    buttonState[x] = digitalRead(buttonPin[x]);
      if (buttonState[x] == HIGH && lastButtonState[x] == LOW && (millis() - buttonDepressed) > debounceDelay){
        buttonPressed = millis();
      }
      if (buttonState[x] == LOW && lastButtonState[x] == HIGH && (millis() - buttonPressed) > debounceDelay){
        if (ignoreButton == false){
          if (buttonPin[x] == 2){
            buttonPushCounter[0]++;
            Serial.println("Driver side button was pressed.");
          }
          if (buttonPin[x] == 3){
            buttonPushCounter[1]++;
            Serial.println("Passenger side button was pressed.");
          }
        } else {
            ignoreButton = false;
            buttonDepressed = millis();
      }
      if (buttonState[x] == HIGH && (millis() - buttonPressed) > buttonHoldTime){
        //SaveState(); // Testing
        Serial.println("Press and hold button event was triggered");
        ignoreButton = true;
        buttonPressed = millis();
      }
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

// If heat is HIGH starts a counter after timerDelay
// And switches heatLevel from HIGH to MID after timerIntervals(side)
void Timer(){
  for (byte x = 0; x < 2; x++){
    if (buttonPushCounter[x] == 1 && timerState[x] == 0){
      timerTrigger[x] = millis();
      timerState[x] = 1;
    }
    if (timerState[x] == 1){
      if ((millis() - timerTrigger[x]) >= timerDelay){
        if (timerTrigger[x] >= timerIntervals[x]){
          HeatLevel(2, x);
          timerState[x] = 0;
          if (timerState[0] == 0){
            Serial.println("Stopping timer for the driver side.");
          }
          if (timerState[1] == 0){
            Serial.println("Stopping timer for the passenger side.");
          }
        }
      }
    }
  }
}

// Saves current buttonPushCounter to EEPROM when press+hold is triggered
void SaveState(){
}
