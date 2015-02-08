/*
  Heated Seat Switching
  by Luis E Alvarado
  2014/11/27
  
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
long debounceDelay = 5; // number of millisecs between button readings

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
  // Reset counters if above a set limit
  resetPushCounter();
  // Lets read the state of each button
  queryButtonState();
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
    switch(level){
      case 0:
        digitalWrite(statusPin[0], LOW);
        digitalWrite(statusPin[1], LOW);
        digitalWrite(statusPin[2], LOW);
        break;
      case 1:
        digitalWrite(statusPin[1], LOW);
        digitalWrite(statusPin[2], LOW);
        digitalWrite(statusPin[0], HIGH);
        break;
      case 2:
        digitalWrite(statusPin[0], LOW);
        digitalWrite(statusPin[2], LOW);
        digitalWrite(statusPin[1], HIGH);
        break;
      case 3:
        digitalWrite(statusPin[0], LOW);
        digitalWrite(statusPin[1], LOW);
        digitalWrite(statusPin[2], HIGH);
        break;
      }
  }
  if (side == 1){
  switch(level){
    case 0:
        digitalWrite(statusPin[3], LOW);
        digitalWrite(statusPin[4], LOW);
        digitalWrite(statusPin[5], LOW);
      break;
    case 1:
      digitalWrite(statusPin[4], LOW);
      digitalWrite(statusPin[5], LOW);
      digitalWrite(statusPin[3], HIGH);
      break;
    case 2:
      digitalWrite(statusPin[3], LOW);
      digitalWrite(statusPin[5], LOW);
      digitalWrite(statusPin[4], HIGH);
      break;
    case 3:
      digitalWrite(statusPin[3], LOW);
      digitalWrite(statusPin[4], LOW);
      digitalWrite(statusPin[5], HIGH);
      break;
    }
  }
}
