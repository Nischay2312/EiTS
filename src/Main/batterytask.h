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
    batteryData *data = (batteryData *)arg;
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
        data->cellPercentage = max17048.cellPercent();
        data->cellVoltage = max17048.cellVoltage();
        data->chargeRate = max17048.chargeRate();
        //send it over through the queue
        if(BATTERY_QUEUE_SIZE == 1){
            xQueueOverwrite(batteryQueue, data);
        }
        else{
            xQueueSend(batteryQueue, data, 0);
        }
        //sleep for 5 seconds
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

int startBatteryTask(BaseType_t assignedCore)
{
    if(!max17048.begin()){
        delay(1000);
        Serial.println("MAX17048 not found!");
        return -1;
    }
    Serial.print(F("Found MAX17048"));
    Serial.print(F(" with Chip ID: 0x")); 
    Serial.println(max17048.getChipID(), HEX);
    
    // BaseType_t ret = xTaskCreatePinnedToCore(
    //   (TaskFunction_t)batteryTask,
    //   (const char *const)"Battery Task",
    //   (const uint32_t)2000,
    //   (void *const) NULL,
    //   (UBaseType_t)5,
    //   (TaskHandle_t *const)NULL,
    //   (const BaseType_t)assignedCore);

    // if(ret != pdPASS)
    // {
    //   Serial.println("Battery Task Creation Failed");
    //   return 0;
    // }
    return 1;
}


