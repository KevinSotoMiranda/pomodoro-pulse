#include <Arduino.h>

struct Button {
  uint8_t pin;
  bool lastState;
  bool currentState;
  unsigned long lastDebounceTime;
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
const unsigned long DEBOUNCE_TIME = 10; // debounce time in ms

// put your setup code here, to run once:
void setup() {
  Serial.begin(115200);

  // initialize the pushbuttons pin as an pull-up input
  for(int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);

    buttons[i].currentState = digitalRead(buttons[i].pin);
    buttons[i].lastState = buttons[i].currentState;
    buttons[i].lastDebounceTime = 0;
  }
}

// put your main code here, to run repeatedly:
void loop() {
  for(int i = 0; i < NUM_BUTTONS; i++) {
    Button &button = buttons[i];

    // read the state of the switch/button:
    int reading = digitalRead(button.pin);

    unsigned long currentTime = millis();

    // check difference (i.e., the button might have been pressed or released)
    if(reading != button.lastState) {
      button.lastDebounceTime = currentTime;
    }

    unsigned long elapsedDebounceTime = currentTime - button.lastDebounceTime;

    if(elapsedDebounceTime > DEBOUNCE_TIME) {
      // button has been stable long enough
      if(reading != button.currentState) {
        button.currentState = reading; // Update the stable state
        if(button.currentState == LOW) {
          // Button press is confirmed, take appropriate action
          Serial.printf("Button %d was pressed\n", i + 1);
        }
      }
    }

    // save the last state
    button.lastState = reading;
  }
}