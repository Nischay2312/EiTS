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
#define LONG_PRESS_DURATION 1000 // Duration for long press in ms
#define DOUBLE_PRESS_TIMEOUT 500 // Timeout for waiting for second press in ms
#define SINGLE_PRESS_DURATION 250 // Duration for single press in ms
#define BUTTON_QUEUE_SIZE 1

TaskHandle_t _buttonTask;
static xQueueHandle buttonQueue;


typedef enum{
    BUTTON_PRESSED,
    BUTTON_LONG_PRESSED,
    BUTTON_DOUBLE_PRESSED
} buttonPressType;

typedef struct{
    buttonPressType Type;
} buttonData;



void dealButtonBouncing() {
    uint8_t subcounts = 0;
    bool state = digitalRead(BUTTON);

    while(subcounts < DEBOUNCE_COUNT) {
        if(state == digitalRead(BUTTON)) {
            subcounts++;
        } else {
            subcounts = 0;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
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
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    //create the queue
    buttonQueue = xQueueCreate(1, sizeof(buttonData));
    if (buttonQueue == NULL)
    {
        Serial.println("Button Queue Creation Failed");
        vTaskDelete(NULL);
    }
    buttonData data;
    bool validPress = false;
    while(1){
        //wait for the interrupt
        uint32_t ulNotificationValue = ulTaskNotifyTakeIndexed( 0, pdFALSE, portMAX_DELAY );
        if(ulNotificationValue){
            //stop interrupts
            detachInterrupt(BUTTON);

            validPress = false;

            //deal with button bouncing
            dealButtonBouncing();

            //check if the button is still pressed
            uint32_t pressDuration = 0;
            while(digitalRead(BUTTON) == LOW){
                vTaskDelay(1 / portTICK_PERIOD_MS);
                pressDuration++;
            }

            if(pressDuration >= LONG_PRESS_DURATION){
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
                    data.Type = BUTTON_DOUBLE_PRESSED;
                    validPress = true;
                }
                else{
                    data.Type = BUTTON_PRESSED;
                    validPress = true;
                }
            }

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
            attachInterrupt(BUTTON, buttonISR, RISING);
        }
    }
}

int SetupButton(BaseType_t assignedCore){
    pinMode(BUTTON, INPUT_PULLUP);
    attachInterrupt(BUTTON, buttonISR, RISING);

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

