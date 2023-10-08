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
//   Header file that contains all the battery related functions. Using Max17048 library to read the battery voltage.
// */
// /////////////////////////////////////////////////////////////////

#include "Adafruit_MAX1704X.h"

Adafruit_MAX17048 max17048;

typedef struct
{
  float cellPercentage;
  float cellVoltage;
  float chargeRate;
} batteryData;

static xQueueHandle batteryQueue;
#define BATTERY_QUEUE_SIZE 1

static void batteryTask(void *arg)
{
    Serial.println("Battery task start!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    if(!max17048.begin()){
      delay(1000);
      Serial.println("MAX17048 not found!");
      vTaskDelete(NULL);
    }

    Serial.print(F("Found MAX17048"));
    Serial.print(F(" with Chip ID: 0x")); 
    Serial.println(max17048.getChipID(), HEX);

    batteryData data;
    //max17048.setActivityThreshold(0.15);
    //max17048.setHibernationThreshold(5);

    //create the queue
    batteryQueue = xQueueCreate(BATTERY_QUEUE_SIZE, sizeof(batteryData));
    if (batteryQueue == NULL)
    {
        Serial.println("Battery Queue Creation Failed");
        vTaskDelete(NULL);
    }
    while (1)
    {
        //Record the battery data
        data.cellPercentage = max17048.cellPercent();
        data.cellVoltage = max17048.cellVoltage();
        data.chargeRate = max17048.chargeRate();
        //send it over through the queue
        if(BATTERY_QUEUE_SIZE == 1){
            xQueueOverwrite(batteryQueue, &data);
        }
        else{
            xQueueSend(batteryQueue, &data, 0);
        }
        // Serial.printf("Battery: %.2f%%\n", data.cellPercentage);
        // Serial.printf("Voltage: %.2f V\n", data.cellVoltage);
        // Serial.printf("Battery rate: %.4f\n", data.chargeRate);
        //sleep for 5 seconds
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

int startBatteryTask(BaseType_t assignedCore)
{
    BaseType_t ret = xTaskCreatePinnedToCore(
      (TaskFunction_t)batteryTask,
      (const char *const)"Battery Task",
      (const uint32_t)5000,
      (void *const) NULL,
      (UBaseType_t)1,
      (TaskHandle_t *const)NULL,
      (const BaseType_t)assignedCore);

    if(ret != pdPASS)
    {
      Serial.println("Battery Task Creation Failed");
      return 0;
    }
    return 1;
}

static void batteryDisplay(void *arg){
  Serial.println("Battery Display task start!");

  batteryData Bdata;
  int ret = startBatteryTask(AUDIOASSIGNCORE);
  

  if(ret != 1){
    Serial.println("Battery Task Failed");
    gfx->println("Battery Task Failed");
  }
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  Serial.println("Entering While Loop!");
  while(1){
    //check if there is something in the battery queue
    if(xQueueReceive(batteryQueue, &Bdata, 0)){
      Serial.println("Trying to suspend tasks");
      //Suspend the other tasks
      //vTaskSuspend(_mp3_player_task);
      if(_draw_task && _decodeTask){
        vTaskSuspend(_draw_task);
        vTaskSuspend(_decodeTask);
      }

      //print the battery data
      //first make the background black
      gfx->fillRect(0, 0, 160, 128, BLACK);
      gfx->setCursor(0, 0);
      gfx->setTextColor(RED);
      //smaller text size
      gfx->setTextSize(1, 1, 0);
      gfx->printf("Battery: %.2f%%\n", Bdata.cellPercentage);
      gfx->printf("Voltage: %.2f V\n", Bdata.cellVoltage);
      gfx->printf("Battery rate: %.4f\n", Bdata.chargeRate);
      Serial.printf("Battery: %.2f%%\n", Bdata.cellPercentage);
      Serial.printf("Voltage: %.2f V\n", Bdata.cellVoltage);
      Serial.printf("Battery rate: %.4f\n", Bdata.chargeRate);

      vTaskDelay(2500 / portTICK_PERIOD_MS);   

      //Resume the Tasks
      //xTaskResumeAll();
      //vTaskResume(_mp3_player_task);
      if(_draw_task && _decodeTask){
        vTaskResume(_draw_task);
        vTaskResume(_decodeTask);
      }
    } 
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

int startBatteryDisplayTask(BaseType_t assignedCore){
  Serial.println("Trying to start Battery Display Task");
  BaseType_t ret = xTaskCreatePinnedToCore(
    (TaskFunction_t)batteryDisplay,
    (const char *const)"Battery Disp",
    (const uint32_t)5000,
    (void *const)NULL,
    (UBaseType_t)1,
    (TaskHandle_t *const)NULL,
    (const BaseType_t)assignedCore);

    if(ret != pdPASS){
      Serial.println("Battery Display Task Creation Failed");
      return 0;
    }

    return 1;
}

