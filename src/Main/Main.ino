// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// // This project is based on a Mini Retro TV.
// // https://www.instructables.com/Mini-Retro-TV/ 
// /////////////////////////////////////////////////////////////////

#define FPS 24
#define MJPEG_BUFFER_SIZE (128 * 160 * 2 / 8)
#define AUDIOASSIGNCORE 1
#define DECODEASSIGNCORE 0
#define DRAWASSIGNCORE 1

#include "config.h"
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include "version.h"

#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define VSPI FSPI
#endif

/* Button Input*/
#include "buttontask.h"

/* Video Play Control*/
#include "videotask.h"

/* Battery Management */
#include "batterytask.h"

/* For retrieving last battery info*/
#include "saveFlash.h"

//intialize the batteryEvent_t memeber
batteryEvent_t batteryEventRcvd;
uint8_t videoState = VIDEO_FINISHED;

void setup() {
  disableCore0WDT();
  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  Serial.println("Wifi is off");

  //configure the LCDBKLT pin
  pinMode(LCDBK, OUTPUT);
  digitalWrite(LCDBK, HIGH);
  delay(100);

  // Init Display
  gfx->begin(80000000);
  gfx->fillScreen(BLACK);

  Serial.println("Init I2S");

  esp_err_t ret_val = i2s_init(I2S_NUM_0, 44100, I2S_MCLK, I2S_SCLK, I2S_LRCLK, I2S_DOUT, I2S_DIN);
  if (ret_val != ESP_OK) {
    Serial.printf("i2s_init failed: %d\n", ret_val);
    gfx->println("i2s_init failed");
    Serial.println("Restarting");
    esp_restart();
    return;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println("Init FS");

  SPIClass spi = SPIClass(HSPI);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, spi, 80000000)) {
    Serial.println("ERROR: File system mount failed!");
    gfx->println("ERROR: File system mount failed!");
    Serial.println("Restarting");
    esp_restart();
    return;
  }

  //retrieve the last battery info
  initPreferences(&batteryEventRcvd.Binfo, sizeof(batteryEventRcvd.Binfo));

  delay(2000);
  Serial.println("System Start");
  myLoop();
}

void loop() {
  //Dont put code here
}

void ShowInfo(bool both){
  //print the data for now
  Serial.println("---------------------------");
  Serial.println("SYSTEM INFO");
  Serial.printf("Uptime: %d mins %d secs\n", (int)(batteryEventRcvd.upTime/60), (int)(batteryEventRcvd.upTime%60));
  Serial.printf("Percetage: %f\n", batteryEventRcvd.Binfo.cellPercentage);
  Serial.printf("Voltage: %f\n", batteryEventRcvd.Binfo.cellVoltage);
  Serial.printf("Dischage Rate: %f\n", batteryEventRcvd.Binfo.chargeRate);
  Serial.printf("Compile Time: %s\nCompile Date: %s\n", COMPILE_TIME, COMPILE_DATE);
  Serial.printf("Version: %s\n", FIRMWARE_VER);
  Serial.println("---------------------------");
  if(both){
      gfx->fillRect(0, 0, 160, 128, BLACK);
      gfx->setCursor(0, 0);
      gfx->setTextColor(RED);
      //smaller text size
      gfx->setTextSize(1, 1, 0);
      gfx->println("SYSTEM INFO");
      gfx->printf("Battery: %.2f%%\n", batteryEventRcvd.Binfo.cellPercentage);
      gfx->printf("Voltage: %.2f V\n", batteryEventRcvd.Binfo.cellVoltage);
      gfx->printf("Battery rate: %.4f\n", batteryEventRcvd.Binfo.chargeRate);
      gfx->printf("Uptime: %d mins %d secs\n", (int)(batteryEventRcvd.upTime/60), (int)(batteryEventRcvd.upTime%60));
      gfx->printf("\n\nVersion: %s\n", FIRMWARE_VER);
      gfx->printf("Compile Time: %s\nCompile Date: %s\n", COMPILE_TIME, COMPILE_DATE);
  }

  //update the last battery info into the flash
  savePreferences(&batteryEventRcvd.Binfo, sizeof(batteryEventRcvd.Binfo));
}

void myLoop(){
  //start the battery reading task
  if(startBatteryDisplayTask(AUDIOASSIGNCORE) != 1){
    Serial.println("Battery Display Task Failed");
    gfx->println("Battery Display Task Failed");
  }

  if(SetupButton(AUDIOASSIGNCORE) != 1){
    Serial.println("Button Task Failed");
    gfx->println("Button Task Failed");
  }
  
  //start the video control task
  if(startVideoControlTask(AUDIOASSIGNCORE) != 1){
    Serial.println("Video Control Task Failed");
    gfx->println("Video Control Task Failed");
  }

  //initialize the video control event queue
    buttonData receivedData;
    intializeButtonEventQueue();

  gfx->println("System Ready\nWaiting For User Input\nSingle Press: Start/Pause\nDouble Press: Skip");
  while(1){

    //read data from the button queue
    if(xQueueReceive(buttonQueue, &receivedData, 0)){
      Serial.println("----------------");
      //print the guesture:
      switch(receivedData.Type){
        case BUTTON_PRESSED:
          Serial.println("Single Press received");
          //Send the button event to the video control task
          xQueueOverwrite(eventQueueMain, &receivedData);
          break;
        case BUTTON_LONG_PRESSED:
          Serial.println("Long Press received");
          if(videoState == VIDEO_FINISHED || videoState == VIDEO_PAUSED){
            ShowInfo(1);
          }
          else{
            Serial.println("Video is not paused so not showing info");
          }
          break;
        case BUTTON_DOUBLE_PRESSED:
          Serial.println("Double Press received");
          xQueueOverwrite(eventQueueMain, &receivedData);
          break;
        default:
          Serial.println("Unrecognized Input");
          break;       
      }
      Serial.println("----------------");
    }
    
    if(xQueueReceive(mainLoopEventQueue, &batteryEventRcvd, 0)){
      //battery info read
      //check if video is not being played
      // if(xQueueReceive(eventQueueMainTx, &videoState, 0)){
      //   if(videoState == VIDEO_PAUSED){
      //     // //print the data for now
      //     // Serial.printf("Uptime: %d\n", batteryEventRcvd.upTime);
      //     // Serial.printf("Percetage: %f\n", batteryEventRcvd.Binfo.cellPercentage);
      //     // Serial.printf("Voltage: %f\n", batteryEventRcvd.Binfo.cellVoltage);
      //     // Serial.printf("Dischage Rate: %f\n", batteryEventRcvd.Binfo.chargeRate);
      //     //ShowInfo();
      //   }

      }
    if(xQueueReceive(eventQueueMainTx, &videoState, 0)){
      //Video State Read
      //push back the state into it for reading it next time
      //xQueueOverwrite(eventQueueMainTx, &videoState);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  Serial.println("Restarting");
  esp_restart();
}
