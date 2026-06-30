#include <Arduino.h>

struct Button {
  uint8_t pin;
  bool lastState;
  bool currentState;
};

// GPIO pins connected to button
Button buttons[] = {
  {18},
  {19},
  {17},
  {16},
  {21},
  {22}
}; 

const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

// int lastState[num_buttons]; // the previous i state from the input pin
// int currentState[num_buttons]; // the current i reading from the input pin

// put your setup code here, to run once:
void setup() {
  Serial.begin(115200);

  // initialize the pushbuttons pin as an pull-up input
  for(int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);

    buttons[i].currentState = digitalRead(buttons[i].pin);
    buttons[i].lastState = buttons[i].currentState;
  }
}

// put your main code here, to run repeatedly:
void loop() {
  // read the state of the switch/button:
  for(int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].currentState = digitalRead(buttons[i].pin);
  }

  for(int i = 0; i < NUM_BUTTONS; i++) {
    if(buttons[i].lastState == LOW && buttons[i].currentState == HIGH)
      Serial.printf("Button %d was pressed\n", i + 1);
  }  

  // save the last state
  for(int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].lastState = buttons[i].currentState;
  } 
}