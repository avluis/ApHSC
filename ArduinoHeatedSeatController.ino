/*
  Heated Seat Switching
  
  This sketch handles the logic required to control a 4-stage heated seat module.
  The stages are HIGH, MEDIUM, LOW and OFF. OFF is the default-start state.
  Indication of stages is done via LEDs for HIGH, MEDIUM and LOW.
  
  INPUT is handled by pushbutton(s).
  INPUT is done on pins 2 and 3 (interrupt capable pins).
  OUTPUT is done on pins 4, 5, 6 and 7.

*/
const int inputPin0 = 2; // Driver Side Button
const int inputPin1 = 3; // Passenger Side Button

const int ledPin = 13; // OnBoard LED

// Driver Side
const int outputPin0 = 4; // HIGH Relay
const int outputPin1 = 5; // MEDIUM Relay
const int outputPin2 = 6; // LOW Relay

// Passenger Side
const int outputPin3 = 7; // HIGH Relay
const int outputPin4 = 8; // MEDIUM Relay
const int outputPin5 = 9; // LOW Relay

const int outputPin6 = 10; // ON Status Pin - HIGH on all states but OFF

const int interruptTimeMills = 300; // Time allowed for Button Debounce
const int powerOnTimeMills = 1050; // Time allowed for vehicle power stabilization
const int powerOffTimeMills = 320; // Power off time - delay

volatile int pressedCountDrv = 0; // Driver Side Button Pressed Count
volatile int pressedCountPas = 0; // Passenger Side Button Pressed Count

void setup() {
  pinMode(ledPin, OUTPUT); // System On
  digitalWrite(ledPin, LOW);
  
  pinMode(inputPin0, INPUT); // Driver Side Button
  digitalWrite(inputPin0, HIGH); //Pull-Up
   
  pinMode(inputPin1, INPUT); // Passenger Side Button  
  digitalWrite(inputPin1, HIGH); //Pull-Up
   
  pinMode(outputPin0, OUTPUT); // HIGH Relay - Driver Side
  digitalWrite(outputPin0, LOW);
   
  pinMode(outputPin1, OUTPUT); // MEDIUM Relay - Driver Side
  digitalWrite(outputPin1, LOW);
   
  pinMode(outputPin2, OUTPUT); // LOW Relay - Driver Side
  digitalWrite(outputPin2, LOW);
   
  pinMode(outputPin3, OUTPUT); // HIGH Relay - Passenger Side
  digitalWrite(outputPin3, LOW);
   
  pinMode(outputPin4, OUTPUT); // Medium Relay - Passenger Side
  digitalWrite(outputPin4, LOW);
   
  pinMode(outputPin5, OUTPUT); // Low Relay - Passenger Side
  digitalWrite(outputPin5, LOW);
   
  pinMode(outputPin6, OUTPUT); // ON Status Pin - HIGH on all states but OFF
  digitalWrite(outputPin6, LOW);
   
  attachInterrupt(0, buttonPressedDrv, FALLING); // Driver Side Button Interrupt
  attachInterrupt(1, buttonPressedPas, FALLING); // Passenger Side Button Interrupt
   
  Serial.begin(9600);
}

void loop() {
  if (pressedCountDrv != 0 || pressedCountPas != 0){
    digitalWrite(ledPin, HIGH);
    
    delay(powerOnTimeMills);
    
    digitalWrite(outputPin6, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
    
    delay(powerOffTimeMills);
    
    digitalWrite(outputPin6, LOW);
  }
}

void buttonPressedDrv(){
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > interruptTimeMills){
    Serial.println("Driver Side Button Pressed!");
    
    noInterrupts();
    pressedCountDrv++;
    interrupts();
    
    if (pressedCountDrv == 4 || pressedCountDrv > 4){
    pressedCountDrv = 0;
  }
    enableHeat(pressedCountDrv, 0); // enable heat, passing heat level and seat (driver).
  }
  lastInterruptTime = interruptTime;
}

void buttonPressedPas(){
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > interruptTimeMills){
    Serial.println("Passenger Side Button Pressed!");
    
    noInterrupts();
    pressedCountPas++;
    interrupts();
    
    if (pressedCountPas == 4 || pressedCountPas > 4){
    pressedCountPas = 0;
  }
    enableHeat(pressedCountPas, 1); // enable heat, passing heat level and seat (passenger).
  }
  lastInterruptTime = interruptTime;
}
// turns on heat, sets the level of heat, from the side requested
void enableHeat(int level, int side){
  if (side == 0){
    Serial.println("Driver Side Heat:");
    switch(level){
    case 0:
      Serial.println("OFF");
      disableHeat(side);
      break;
    case 1:
      Serial.println("HIGH");
      digitalWrite(outputPin1, LOW);
      digitalWrite(outputPin2, LOW);
      digitalWrite(outputPin0, HIGH);
      break;
    case 2:
      Serial.println("MEDIUM");
      digitalWrite(outputPin0, LOW);
      digitalWrite(outputPin2, LOW);
      digitalWrite(outputPin1, HIGH);
      break;
    case 3:
      Serial.println("LOW");
      digitalWrite(outputPin0, LOW);
      digitalWrite(outputPin1, LOW);
      digitalWrite(outputPin2, HIGH);
      break;
    }
  }
  if (side == 1){
    Serial.println("Passenger Side Heat:");
    switch(level){
    case 0:
      Serial.println("OFF");
      disableHeat(side);
      break;
    case 1:
      Serial.println("HIGH");
      digitalWrite(outputPin4, LOW);
      digitalWrite(outputPin5, LOW);
      digitalWrite(outputPin3, HIGH);
      break;
    case 2:
      Serial.println("MEDIUM");
      digitalWrite(outputPin3, LOW);
      digitalWrite(outputPin5, LOW);
      digitalWrite(outputPin4, HIGH);
      break;
    case 3:
      Serial.println("LOW");
      digitalWrite(outputPin3, LOW);
      digitalWrite(outputPin4, LOW);
      digitalWrite(outputPin5, HIGH);
      break;
    }
  }
}
// turns off heat only on the side requested
void disableHeat(int side){
  switch(side){
    case 0:
      Serial.println("Driver Side Off");
      delay(powerOffTimeMills);
      digitalWrite(outputPin0, LOW);
      digitalWrite(outputPin1, LOW);
      digitalWrite(outputPin2, LOW);
      break;
    case 1:
      Serial.println("Passenger Side Off");
      delay(powerOffTimeMills);
      digitalWrite(outputPin3, LOW);
      digitalWrite(outputPin4, LOW);
      digitalWrite(outputPin5, LOW);
      break;
  }
}
