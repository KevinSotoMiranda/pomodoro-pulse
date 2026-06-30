#include <Arduino.h>

enum State {
  CODING,
  WRITING,
  BREAK,
  MEETING,
  DEEP_WORK,
  IDLE
};

struct Button {
  uint8_t pin;
  State buttonState;

  bool lastState;
  bool currentState;
  unsigned long lastDebounceTime;
};

struct LED {
  uint8_t pin;
  unsigned int brightness;
  unsigned int fadeAmount;
};

struct BUZZER {
  uint8_t pin;
  unsigned long buzzerTime; 
};

LED leds[] = {
  // {26, 0, 5}, COMMENTED OUT SINCE USING SAME PIN AS BUZZER
  {25, 0, 5},
  {23, 0, 5}
};

// GPIO pins connected to button
Button buttons[] = {
  {18, CODING},
  {19, WRITING},
  {17, BREAK},
  {16, MEETING},
  {21, DEEP_WORK},
  {22, IDLE}
}; 

BUZZER buzzer = {
  26,
  200
};

const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);
const int NUM_LED = sizeof(leds) / sizeof(leds[0]);

const unsigned long DEBOUNCE_TIME = 10; // debounce time in ms

enum State currentState = IDLE;

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

  for(int i = 0; i < NUM_LED; i++) {
    pinMode(leds[i].pin, OUTPUT);
  }

  pinMode(buzzer.pin, OUTPUT);
}

void updateState(State buttonState) {
  currentState = buttonState;

  Serial.print("State is now ");
  switch(currentState) {
    case CODING:
      Serial.print("CODING\n");
      break;
    case WRITING:
      Serial.print("WRITING\n");
      break;
    case BREAK:
      Serial.print("BREAK\n");
      break;
    case MEETING:
      Serial.print("MEETING\n");
      break;
    case DEEP_WORK:
      Serial.print("DEEP_WORK\n");
      break;
    case IDLE:
      Serial.print("IDLE\n");
      break;
  }
}

void updateLights() {

  for(int i = 0; i < NUM_LED; i++) {
    LED &led = leds[i];

    // Set brightness
    analogWrite(led.pin, led.brightness);
    led.brightness+= led.fadeAmount; // Increase the brightness
    
    // When it has reached the maximum brightness (255) start decrementing it, or vice versa
    if(led.brightness >= 255 || led.brightness <= 0) {
      led.fadeAmount = -led.fadeAmount; // Invert the amount
    }

    delay(30); // Wait 30 ms
  }
}

void triggerBuzzer() {    
    digitalWrite(buzzer.pin, HIGH);
    delay(buzzer.buzzerTime); // Have buzzer play for 200 ms
    digitalWrite(buzzer.pin, LOW);
}

// put your main code here, to run repeatedly:
void loop() {
  updateLights();

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
          triggerBuzzer();
          updateState(button.buttonState);
        }
      }
    }

    // save the last state
    button.lastState = reading;
  }
}