/*
  Heated Seat Switching
  
  This sketch handles the logic required to control a 4-stage heated seat module.
  The stages are HIGH, MEDIUM, LOW and OFF. OFF is the default-start state.
  Indication of stages is done via LEDs for HIGH, MEDIUM and LOW.
  
  INPUT is handled by pushbutton(s).
  INPUT is done on pins 2 and 3 (interrupt capable pins).
  OUTPUT is done on pins 4, 5, 6 and 7.

*/

int inputPin0 = 2; // Driver Side Button
int inputPin1 = 3; // Passenger Side Button

int ledPin = 13; // OnBoard LED

int outputPin0 = 4; // ON Relay - HIGH on all states but OFF
                    // Driver Side
int outputPin1 = 5; // HIGH Relay
int outputPin2 = 6; // MEDIUM Relay
int outputPin3 = 7; // LOW Relay

int interruptTimeMills = 200; // Time allowed for Button Debounce

volatile int pressedCountDrv = 0; // Driver Side Button Pressed Count
volatile int pressedCountPas = 0; // Passenger Side Button Pressed Count

void setup() {
   pinMode(inputPin0, INPUT); // Driver Side Button
   digitalWrite(inputPin0, HIGH); //Pull-Up
   
   pinMode(inputPin1, INPUT); // Passenger Side Button
   digitalWrite(inputPin1, HIGH); //Pull-Up
   
   pinMode(ledPin, OUTPUT);
   digitalWrite(ledPin, LOW);
   
   pinMode(outputPin0, OUTPUT);
   digitalWrite(outputPin0, LOW);
   
   pinMode(outputPin1, OUTPUT);
   digitalWrite(outputPin1, LOW);
   
   pinMode(outputPin2, OUTPUT);
   digitalWrite(outputPin2, LOW);
   
   pinMode(outputPin3, OUTPUT);
   digitalWrite(outputPin3, LOW);
   
   attachInterrupt(0, buttonPressedDrv, FALLING); // Driver Side Button Interrupt
   attachInterrupt(1, buttonPressedPas, FALLING); // Passenger Side Button Interrupt
   
   Serial.begin(9600);
}

void loop() {
  if (pressedCountDrv == 4 || pressedCountDrv > 4){
    pressedCountDrv = 0;
  }
  
  if (pressedCountPas == 4 || pressedCountPas > 4){
    pressedCountPas = 0;
  }
  
  if (pressedCountDrv != 0 || pressedCountPas != 0){
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
}

void buttonPressedDrv(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > interruptTimeMills){
    Serial.println("Driver Side Button Pressed!");
    
    noInterrupts();
    pressedCountDrv++;
    interrupts();
  }
  last_interrupt_time = interrupt_time;
}

void buttonPressedPas(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > interruptTimeMills){
    Serial.println("Passenger Side Button Pressed!");
    
    noInterrupts();
    pressedCountPas++;
    interrupts();
  }
  last_interrupt_time = interrupt_time;
}
