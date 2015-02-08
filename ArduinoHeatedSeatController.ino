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
const int buttonPin[] = {2, 3};

// Output to ULN2003A
const int statusPin[] = {4, 5, 6, 7, 8, 9};

// On-Board LED (on Arduino)
const int onBoardLedPin = 13;

// Push Counters/State Change
int buttonPushCounter[] = {0, 0};
int buttonState[] = {0, 0};
int lastButtonState[] = {0, 0};

// Timers
long lastDebounceTime = 0;
// adjust this if getting button bounce
long debounceDelay = 6;

void setup() {
  
  // Initializing On-Board LED as an output
  pinMode(onBoardLedPin, OUTPUT);
  
  // Initializing Buttons as inputs
  for (int x = 0; x < 2; x++){
  pinMode(buttonPin[x], INPUT);
  }  
  
  // Initializing Status Pins as outputs
  for (int x = 0; x < 6; x++){
  pinMode(statusPin[x], OUTPUT);
  }
}

void loop() {
  // Lets read the state of each button
  queryButtonState();
  // Reset counters if above a set limit
  resetPushCounter();
  // Now we can get some heat going
  toggleHeat();
}

void resetPushCounter(){
  for (int x = 0; x < 2; x++){
    if (buttonPushCounter[x] == 4){
      buttonPushCounter[x] = 0;
    }
  }
}

void queryButtonState(){
  for (int x = 0; x < 2; x++){
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

void toggleHeat(){
  if (buttonPushCounter[0] != 0 || buttonPushCounter[1] != 0){
    powerOn();
  }
  else{
    powerOff();
  }
}

void powerOn(){
  digitalWrite(onBoardLedPin, HIGH);
  enableHeat();
}

void powerOff(){
  disableHeat();
  digitalWrite(onBoardLedPin, LOW);
}

void disableHeat(){
  for (int x = 0; x < 6; x++){
  digitalWrite(statusPin[x], LOW);
  }
}

void enableHeat(){
  for (int x = 0; x < 2; x++){
    heatLevel(buttonPushCounter[x], x);
  }
}

void heatLevel(int level, int side){
  if (side == 0){
    if (level != 0){
      for (int n = 0; n < 3; n++){
        digitalWrite(statusPin[n], LOW);
        if (level > 0){
          digitalWrite(statusPin[level] - 1, HIGH);
        }
      }
    } else {
      for (int n = 0; n < 3; n++){
      digitalWrite(statusPin[n], LOW);
      }
    }
  } else {
    if (level != 0){
      for (int n = 0; n < 3; n++){
        digitalWrite(statusPin[n] + 3, LOW);
        if (level > 0){
          digitalWrite(statusPin[level] + 2, HIGH);
        }
      }
    } else {
      for (int n = 0; n < 3; n++){
      digitalWrite(statusPin[n] + 3, LOW);
      }
    }
  }
}
