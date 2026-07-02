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
};

struct LED {
  uint8_t pin;
  State state;
  State state2;
};

struct BUZZER {
  uint8_t pin;
  unsigned long buzzerTime; 
};

LED leds[] = {
  // {26, BREAK, IDLE}, COMMENTED OUT SINCE USING SAME PIN AS BUZZER // Green LED
  {25, CODING, DEEP_WORK}, // Red LED
  {23, WRITING, MEETING} // Yellow LED
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

// Create the size of the buffer
QueueHandle_t xQueue;

void updateState(State buttonState);
void triggerBuzzer();
void updateLights();

void stateMachineTask(void * parameters) {
  State receivedState;
  const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );

  TickType_t lastTimePressed = 0;

  for(;;) {
    if(xQueueReceive(xQueue, &receivedState, xTicksToWait) == pdPASS) {
      TickType_t currentTime = xTaskGetTickCount();
      TickType_t elapsedDebounceTime = currentTime - lastTimePressed;

      if(elapsedDebounceTime > pdMS_TO_TICKS( DEBOUNCE_TIME )) {
        updateState(receivedState);
        triggerBuzzer();

        lastTimePressed = currentTime;

        xQueueReset(xQueue);
      }
    }
  }
}

void timerTask(void * parameters) {
  TickType_t timer = pdMS_TO_TICKS(10000); // 10 seconds
  TickType_t startTime = xTaskGetTickCount(); // snapshot when countdown begins

  for(;;) {
    TickType_t elapsedTime = xTaskGetTickCount() - startTime;

    if(elapsedTime < timer) {
      TickType_t remainingTime = timer - elapsedTime;
      Serial.print("TIME REMAINING: ");
      Serial.println(pdTICKS_TO_MS(remainingTime));
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else {
      // Timer expired
      Serial.println("TIMER DONE");

      // Reset start to rearm for the next countdown
      startTime = xTaskGetTickCount();
    }
  }
}

void commsSyncTask(void * parameters) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void IRAM_ATTR buttonPressISR( void ) {
  // Create the size of items that will be in the buffer
  State buttonState;

  // A flag for the buffer to know if task has been finished or not
  BaseType_t xHigherPriorityTaskWoken;
  xHigherPriorityTaskWoken = pdFALSE;

  for(int i = 0; i < NUM_BUTTONS; i++) {
    int read = digitalRead(buttons[i].pin);

    if(read == LOW) {
      buttonState = buttons[i].buttonState;

      // Do actual work
      xQueueSendFromISR(xQueue, &buttonState, &xHigherPriorityTaskWoken);
    }
  }

  // If a higher priority task was woken, yield to it immediately
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

// put your setup code here, to run once:
void setup() {
  Serial.begin(115200);
  xQueue = xQueueCreate( 10, sizeof( State ) );

  // initialize the pushbuttons pin as an pull-up input
  for(int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(buttons[i].pin), buttonPressISR, FALLING);
  }

  for(int i = 0; i < NUM_LED; i++) {
    pinMode(leds[i].pin, OUTPUT);
  }

  pinMode(buzzer.pin, OUTPUT);
  

  xTaskCreate(
    stateMachineTask,
    "STATE_MACHINE_TASK",
    2048,
    NULL,
    2,
    NULL
  );

  xTaskCreate(
    timerTask,
    "TIMER_TASK",
    2048,
    NULL,
    2,
    NULL
  );

  xTaskCreate(
    commsSyncTask,
    "COMMS_SYNC_TASK",
    2048,
    NULL,
    1,
    NULL
  );
}

void updateState(State buttonState) {
  currentState = buttonState;
  updateLights();

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

    if(currentState == led.state || currentState == led.state2) {
      digitalWrite(led.pin, HIGH);
    }
    else {
      digitalWrite(led.pin, LOW);
    }
  }
}

void triggerBuzzer() {    
    digitalWrite(buzzer.pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(buzzer.buzzerTime)); // Have buzzer play for 200 ms
    digitalWrite(buzzer.pin, LOW);
}

// put your main code here, to run repeatedly:
void loop() {
}