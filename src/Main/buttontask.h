// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   batterytask.h
//   Header file that contains all the button related functions.
// */
// /////////////////////////////////////////////////////////////////

#define DEBOUNCE_COUNT 20   //number of stable counts for the button input
#define SUPER_LONG_PRESS_DURATION 1000 // Duration for super long press in ms
#define LONG_PRESS_DURATION 400 // Duration for long press in ms
#define DOUBLE_PRESS_TIMEOUT 250 // Timeout for waiting for second press in ms
#define SINGLE_PRESS_DURATION 20 // Duration for single press in ms
#define BUTTON_QUEUE_SIZE 1

TaskHandle_t _buttonTask;
static xQueueHandle buttonQueue;

typedef enum{
    BUTTON_PRESSED,
    BUTTON_LONG_PRESSED,
    BUTTON_DOUBLE_PRESSED,
    BUTTON_SUPER_LONG_PRESSED,
    RESTARTME
} buttonPressType;

typedef struct{
    buttonPressType Type;
} buttonData;



void dealButtonBouncing() {
    uint8_t subcounts = 0;
    bool lastState = digitalRead(BUTTON);

    while(subcounts < DEBOUNCE_COUNT) {
        if(lastState == digitalRead(BUTTON)) {
            lastState = digitalRead(BUTTON);
            subcounts++;
        } else {
            lastState = digitalRead(BUTTON);
            subcounts = 0;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
        //delay(1);
    }
}

static void IRAM_ATTR buttonISR(){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    configASSERT( _buttonTask != NULL );
    vTaskNotifyGiveFromISR(_buttonTask, 
                         &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void buttonTask(void *arg){
    Serial.println("Button task start!");
    //vTaskDelay(1000 / portTICK_PERIOD_MS);

    //create the queue
    buttonQueue = xQueueCreate(1, sizeof(buttonData));
    if (buttonQueue == NULL)
    {
        Serial.println("Button Queue Creation Failed");
        vTaskDelete(NULL);
    }
    Serial.println("Button Task queue created successfully!");
    buttonData data;
    bool validPress = false;
    while(1){
        //wait for the interrupt
        //Serial.println("Button Task Waiting for interrupt");
        uint32_t ulNotificationValue = ulTaskNotifyTakeIndexed( 0, pdFALSE, portMAX_DELAY );
        if(ulNotificationValue){
            //stop interrupts
            detachInterrupt(BUTTON);
            //Serial.println("notification received");

            validPress = false;

            //deal with button bouncing
            uint32_t presstime = millis();
            dealButtonBouncing();
            presstime = millis() - presstime;
            //Serial.println("Button debouncing dealt");
            //Serial.printf("Debounce time: %d\n", presstime);

            //check if the button is still pressed
            uint32_t pressDuration = 0;
            while(digitalRead(BUTTON) == LOW){
                vTaskDelay(1 / portTICK_PERIOD_MS);
                //delay(1);
                pressDuration++;
            }

            if(pressDuration >= SUPER_LONG_PRESS_DURATION){
                //Serial.printf("Long Press detected\n");
                data.Type = BUTTON_SUPER_LONG_PRESSED;
                validPress = true;
            }

            else if(pressDuration >= LONG_PRESS_DURATION){
                //Serial.printf("Long Press detected\n");
                data.Type = BUTTON_LONG_PRESSED;
                validPress = true;
            }

            else if(pressDuration >= SINGLE_PRESS_DURATION){
                //wait for the second press
                uint32_t timeWaited = 0;
                bool secondPress = false;
                while(timeWaited < DOUBLE_PRESS_TIMEOUT){
                    vTaskDelay(1 / portTICK_PERIOD_MS);
                    timeWaited++;
                    if(digitalRead(BUTTON) == LOW){
                        //deal with button bouncing
                        dealButtonBouncing();
                        //check if the button is still pressed
                        uint32_t press2Duration = 0;
                        while(digitalRead(BUTTON) == LOW){
                            vTaskDelay(1 / portTICK_PERIOD_MS);
                            press2Duration++;
                        }
                        if(press2Duration >= SINGLE_PRESS_DURATION){
                            secondPress = true;
                            break;
                        }
                    }
                }
                if(secondPress){
                    //Serial.printf("Double Press detected\n");
                    data.Type = BUTTON_DOUBLE_PRESSED;
                    validPress = true;
                }
                else{
                    //Serial.printf("Short Press detected\n");
                    data.Type = BUTTON_PRESSED;
                    validPress = true;
                }
                
            }
            //Serial.printf("Button Presstime: %d\n", pressDuration);
            //send it over through the queue
            if(validPress){
                if(BUTTON_QUEUE_SIZE == 1){
                    xQueueOverwrite(buttonQueue, &data);
                }
                else{
                    xQueueSend(buttonQueue, &data, 0);
                }
            }
            //return interrupts
            attachInterrupt(BUTTON, buttonISR, FALLING);
        }
    }
}

int SetupButton(BaseType_t assignedCore){
    pinMode(BUTTON, INPUT_PULLUP);
    attachInterrupt(BUTTON, buttonISR, FALLING);

    BaseType_t ret = xTaskCreatePinnedToCore(
        buttonTask, /* Function to implement the task */
        "buttonTask", /* Name of the task */
        40000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        configMAX_PRIORITIES - 1,  /* Priority of the task */
        &_buttonTask,  /* Task handle. */
        assignedCore); /* Core where the task should run */

    if(ret != pdPASS){
        Serial.println("Button Task Creation Failed");
        return -1;
    }
    return 1;
}

