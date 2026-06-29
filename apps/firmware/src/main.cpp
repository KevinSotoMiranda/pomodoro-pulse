#include <Arduino.h>

// GPIO pins connected to button
const uint8_t button_pins[] = {
  18,
  19,
  17,
  16,
  21,
  22
}; 

const int num_buttons = 6;

int lastState[num_buttons]; // the previous i state from the input pin
int currentState[num_buttons]; // the current i reading from the input pin

// put your setup code here, to run once:
void setup() {
  Serial.begin(115200);

  // initialize the pushbuttons pin as an pull-up input
  for(int i = 0; i < num_buttons; i++) {
    pinMode(button_pins[i], INPUT_PULLUP);
    lastState[i] = digitalRead(button_pins[i]);
  }
}

// put your main code here, to run repeatedly:
void loop() {
  // read the state of the switch/button:
  for(int i = 0; i < num_buttons; i++) {
    currentState[i] = digitalRead(button_pins[i]);
  }

  for(int i = 0; i < num_buttons; i++) {
    if(lastState[i] == LOW && currentState[i] == HIGH)
      Serial.printf("Button %d was pressed\n", i + 1);
  }  

  // save the last state
  for(int i = 0; i < num_buttons; i++) {
    lastState[i] = currentState[i];
  } 
}