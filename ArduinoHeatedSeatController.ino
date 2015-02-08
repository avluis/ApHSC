/*
  Heated Seat Switching
  
  This sketch handles the logic required to control a 4-stage heated seat module.
  The stages are HIGH, MEDIUM, LOW and OFF. OFF is the default-start state.
  Indication of stages is done via LEDs for HIGH, MEDIUM and LOW.
  Vehicle wiring is ground based and built-in LEDs turn on when grounded.
  
  INPUT:
  Pin 2 & 3
  
  OUTPUT:
  Pin 4, 5 and 6 for Driver side.
  Pin 7, 8 and 9 for Passenger side.
*/

// Dash Buttons to monitor
const byte buttonPin[] = {2, 3};

// Output to ULN2003A
const byte statusPin[] = {4, 5, 6, 7, 8, 9};

// On-Board LED (on Arduino)
const byte onBoardLedPin = 13;

// Push Counters/State Change
byte buttonPushCounter[] = {0, 0};
byte buttonState[] = {0, 0};
byte lastButtonState[] = {0, 0};

// Timers
unsigned long lastDebounceTime = 0;
// Adjust this if getting button bounce
unsigned long debounceDelay = 6;

// Called when sketch starts
void setup() {
  // Initializing On-Board LED as an output
  pinMode(onBoardLedPin, OUTPUT);
  // Initializing Buttons as inputs
  for (byte x = 0; x < 2; x++){
  pinMode(buttonPin[x], INPUT);
  }
  // Initializing Status Pins as outputs
  for (byte x = 0; x < 6; x++){
  pinMode(statusPin[x], OUTPUT);
  }
}

// Loops consecutively
void loop() {
  // Lets read the state of each button
  queryButtonState();
  // Reset counters if above a set limit
  resetPushCounter();
  // Now we can get some heat going
  toggleHeat();
}

// If the number of button presses reaches
// a certain number, reset press count to 0
void resetPushCounter(){
  for (byte x = 0; x < 2; x++){
    if (buttonPushCounter[x] == 4){
      buttonPushCounter[x] = 0;
    }
  }
}

// Listens for button presses, while debouncing the button input
// and tracks the number of presses respective to each button (side)
void queryButtonState(){
  for (byte x = 0; x < 2; x++){
    buttonState[x] = digitalRead(buttonPin[x]);
    if ((millis() - lastDebounceTime) > debounceDelay){
      if (buttonState[x] != lastButtonState[x]){
          if (buttonState[x] == HIGH && buttonPin[x] == 2){
            buttonPushCounter[0]++;
          }
          if (buttonState[x] == HIGH && buttonPin[x] == 3){
            buttonPushCounter[1]++;
          }
        }
      lastButtonState[x] = buttonState[x];
      lastDebounceTime = millis();
    }
  }
}

// Toggles heat on in accordance to the number of button presses
// on each respective side/button.
// It also toggles heat off if the number of presses loops to 0
void toggleHeat(){
  if (buttonPushCounter[0] != 0 || buttonPushCounter[1] != 0){
    powerOn();
  }
  else{
    powerOff();
  }
}

// Toggles on the On Board LED
// and calls enableHeat()
void powerOn(){
  digitalWrite(onBoardLedPin, HIGH);
  enableHeat();
}

// Toggles off the On Board LED
// and calls disableHeat()
void powerOff(){
  disableHeat();
  digitalWrite(onBoardLedPin, LOW);
}

// Disables heat when called
void disableHeat(){
  for (byte x = 0; x < 6; x++){
  digitalWrite(statusPin[x], LOW);
  }
}

// Enables heat when called,
// then sends heat level and side to heatLevel()
void enableHeat(){
  for (byte x = 0; x < 2; x++){
    heatLevel(buttonPushCounter[x], x);
  }
}

// This functions receives heat level and heat side arguments,
// it then uses this data to turn off/on the respective pin(s)
void heatLevel(byte level, byte side){
  if (side == 0){
    if (level != 0){
      for (byte n = 0; n < 3; n++){
        digitalWrite(statusPin[n], LOW);
        if (level > 0){
          digitalWrite(statusPin[level] - 1, HIGH);
        }
      }
    } else {
      for (byte n = 0; n < 3; n++){
      digitalWrite(statusPin[n], LOW);
      }
    }
  } else {
    if (level != 0){
      for (byte n = 0; n < 3; n++){
        digitalWrite(statusPin[n] + 3, LOW);
        if (level > 0){
          digitalWrite(statusPin[level] + 2, HIGH);
        }
      }
    } else {
      for (byte n = 0; n < 3; n++){
      digitalWrite(statusPin[n] + 3, LOW);
      }
    }
  }
}
