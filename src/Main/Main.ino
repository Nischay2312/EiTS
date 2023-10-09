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

void setup() {
  disableCore0WDT();
  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  Serial.println("Wifi is off");

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
  delay(2000);
  Serial.println("System Start");
  myLoop();
}

void loop() {
  //Dont put code here
}


void myLoop(){
  //start the battery reading task
  // if(startBatteryDisplayTask(AUDIOASSIGNCORE) != 1){
  //   Serial.println("Battery Display Task Failed");
  //   gfx->println("Battery Display Task Failed");
  // }

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

  gfx->println("Waiting For Input");
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
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // if(xQueueReceive(eventQueueMain, &receivedData, 0)){
    //   switch(receivedData.Type){
    //     case RESTARTME:
    //        Serial.println("Restarting Video Player Task");
    //        if(startVideoControlTask(AUDIOASSIGNCORE) != 1){
    //           Serial.println("Video Control Task Failed");
    //           gfx->println("Video Control Task Failed");
    //        }
    //        break;
    //     default:
    //       Serial.println("Un recognized input, putting it back in.");
    //       xQueueOverwrite(eventQueueMain, &receivedData);
    //       break;
    //   }
    // }
  }

  Serial.println("Restarting");
  esp_restart();
}
