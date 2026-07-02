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

struct Buzzer {
  uint8_t pin;
};

struct BuzzerPattern {
  int pattern[5];
  int patternLength;
  State state;
};

struct LED leds[] = {
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

Buzzer buzzer = {
  26,
};

BuzzerPattern buzzerPatterns[] = {
  {{100, 0, 0, 0, 0}, 1, CODING},
  {{500, 0, 0, 0, 0}, 1, DEEP_WORK},
  {{100, 100, 100, 0, 0}, 3, WRITING},
  {{300, 100, 300, 0, 0}, 3, MEETING},
  {{80, 80, 80, 80, 80}, 5, BREAK},
  {{50, 0, 0, 0, 0}, 1, IDLE}
};

const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);
const int NUM_LED = sizeof(leds) / sizeof(leds[0]);
const int NUM_BUZZER_PATTERNS = sizeof(buzzerPatterns) / sizeof(buzzerPatterns[0]);

const unsigned long DEBOUNCE_TIME = 10; // debounce time in ms

TaskHandle_t timerTaskHandle = NULL;

// Create the size of the buffer
QueueHandle_t xQueue;

void updateState(State buttonState);
void triggerBuzzer(State newState);
void updateLights(State newState);

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
        triggerBuzzer(receivedState);

        xTaskNotify(timerTaskHandle,(uint32_t) receivedState, eSetValueWithOverwrite);

        lastTimePressed = currentTime;

        xQueueReset(xQueue);
      }
    }
  }
}

void timerTask(void * parameters) {
  TickType_t timer = pdMS_TO_TICKS(10000); // 10 seconds
  TickType_t startTime = xTaskGetTickCount(); // snapshot when countdown begins

  uint32_t pulNotificationValue;
  const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );
  
  State localState;

  for(;;) {
    if(xTaskNotifyWait(0, 0, &pulNotificationValue, xTicksToWait) == pdPASS) {
      // Reset start to rearm for the next countdown
      startTime = xTaskGetTickCount();
      localState = (State) pulNotificationValue;
    }
    else {
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
    &timerTaskHandle
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
  updateLights(buttonState);

  Serial.print("State is now ");
  switch(buttonState) {
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

void updateLights(State newState) {
  for(int i = 0; i < NUM_LED; i++) {
    LED &led = leds[i];

    if(newState == led.state || newState == led.state2) {
      digitalWrite(led.pin, HIGH);
    }
    else {
      digitalWrite(led.pin, LOW);
    }
  }
}

void triggerBuzzer(State newState) {   
  for(int i = 0; i < NUM_BUZZER_PATTERNS; i++) {
    if(buzzerPatterns[i].state == newState) {
      for(int p = 0; p < buzzerPatterns[i].patternLength; p++) {
        bool isEven = !(p % 2);
        
        if(isEven) {
          digitalWrite(buzzer.pin, HIGH);
        }
        else {
          digitalWrite(buzzer.pin, LOW);
        }

        vTaskDelay(pdMS_TO_TICKS(buzzerPatterns[i].pattern[p])); // Have buzzer play for ms
      }

      digitalWrite(buzzer.pin, LOW);
    }
  }
}

// put your main code here, to run repeatedly:
void loop() {
}