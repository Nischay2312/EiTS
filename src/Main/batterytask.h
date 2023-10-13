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
// #include <esp_deep_sleep.h>

Adafruit_MAX17048 max17048;

typedef struct
{
  float cellPercentage;
  float cellVoltage;
  float chargeRate;
} batteryData;

typedef struct{
  batteryData Binfo;
  unsigned long upTime;
  bool pluggedIn;
} batteryEvent_t;

static xQueueHandle batteryQueue;
static xQueueHandle mainLoopEventQueue;

#define BATTERY_QUEUE_SIZE 1

//function to put the esp to sleep puts it in hibernate mode, claims to consume sub 10uA
void putEspToSleep(){
  //put the esp to sleep
  digitalWrite(BATEN, LOW);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
}

static void batteryTask(void *arg)
{
    Serial.println("Battery task start!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    if(!max17048.begin()){
      vTaskDelay(1000 / portTICK_PERIOD_MS);
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
        //sleep for 1 seconds
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

int startBatteryTask(BaseType_t assignedCore)
{
    BaseType_t ret = xTaskCreatePinnedToCore(
      (TaskFunction_t)batteryTask,
      (const char *const)"Battery Task",
      (const uint32_t)5000,
      (void *const) NULL,
      (UBaseType_t)5,
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
  unsigned long uptime = 0;
  Serial.println("Battery Display task start!");

  batteryData Bdata;
  batteryEvent_t eventToSend;
  
  //intialize the queue here
  mainLoopEventQueue = xQueueCreate(1, sizeof(batteryEvent_t));
  int ret = startBatteryTask(DECODEASSIGNCORE);
  
  if(ret != 1){
    Serial.println("Battery Task Failed");
    gfx->println("Battery Task Failed");
  }
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  Serial.println("Entering While Loop!");
  while(1){
    //check if there is something in the battery queue
    if(xQueueReceive(batteryQueue, &Bdata, 0)){
      //send the data to the main loop

      //copy the battery data and the uptime and send it to main loop via queue
      eventToSend.Binfo = Bdata;
      eventToSend.upTime = uptime;

      //check if the battery is plugged in
        if(Bdata.chargeRate >= PLUGGED_IN_THRESHOLD){
          eventToSend.pluggedIn = true;
        }
        else{
          eventToSend.pluggedIn = false;
        }

      xQueueOverwrite(mainLoopEventQueue,&eventToSend);

      // Serial.printf("Battery: %.2f%%\n", Bdata.cellPercentage);
      // Serial.printf("Voltage: %.2f V\n", Bdata.cellVoltage);
      // Serial.printf("Battery rate: %.4f\n", Bdata.chargeRate);

    } 
    uint8_t time_wait = 2;
    vTaskDelay(time_wait * 1000 / portTICK_PERIOD_MS);
    uptime += time_wait; //seconds
  }
}

int startBatteryDisplayTask(BaseType_t assignedCore){
  Serial.println("Trying to start Battery Display Task");
  BaseType_t ret = xTaskCreatePinnedToCore(
    (TaskFunction_t)batteryDisplay,
    (const char *const)"Battery Disp",
    (const uint32_t)20000,
    (void *const)NULL,
    (UBaseType_t)5,
    (TaskHandle_t *const)NULL,
    (const BaseType_t)assignedCore);

    if(ret != pdPASS){
      Serial.println("Battery Display Task Creation Failed");
      return 0;
    }

    return 1;
}

