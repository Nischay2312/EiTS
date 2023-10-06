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

/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
Arduino_DataBus *bus = new Arduino_ESP32SPI(DC, CS, SCK, MOSI, MISO, VSPI);
Arduino_GFX *gfx = new Arduino_ST7735(bus, RST /* RST */, 1 /* rotation */, false /* IPS */, ST7735_TFTWIDTH /* w*/, ST7735_TFTHEIGHT /* h*/,
                                      2 /* col_offset1 */, 1 /* row_offset1 */, 2 /* col_offset2 */, 1 /* row_offset2 */, false /* bgr */);

/* Audio */
#include "audiotask.h"

/* MJPEG Video */
#include "decodetask.h"
void playVideo(File vFile, File afile);

/* Batter Management */
#include "batterytask.h"

/* Variables */
static int next_frame = 0;
static int skipped_frames = 0;
static unsigned long start_ms, curr_ms, next_frame_ms;
static unsigned int video_idx = 1;

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw) {
  unsigned long s = millis();
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  total_show_video_ms += millis() - s;
  return 1;
} /* drawMCU() */

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
    return;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println("Init FS");

  SPIClass spi = SPIClass(HSPI);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, spi, 80000000)) {
    Serial.println("ERROR: File system mount failed!");
    gfx->println("ERROR: File system mount failed!");
    return;
  }
  delay(2000);
  Serial.println("System Start");
}

void loop() {
  //start the battery task
  batteryData Bdata;
  int ret = startBatteryTask(AUDIOASSIGNCORE);

  if(ret != 1){
    Serial.println("Battery Task Failed");
    gfx->println("Battery Task Failed");
  }

  video_idx = 1;
  Serial.printf("videoIndex: %d\n", video_idx);

  gfx->setCursor(20, 20);
  gfx->setTextColor(GREEN);
  gfx->setTextSize(6, 6, 0);
  gfx->printf("CH %d", video_idx);
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  while(1){
    // bool ret = false;//playVideoWithAudio(video_idx);
    // if(ret){
    //   video_idx = video_idx + 1;
    // }
    // else{
    //   video_idx = 1;
    // }
    
    //check if there is something in the battery queue
    if(xQueueReceive(batteryQueue, &Bdata, 0)){
      //print the battery data
      //first make the background black
      gfx->fillRect(0, 0, 160, 128, BLACK);
      gfx->setCursor(0, 0);
      gfx->setTextColor(WHITE);
      //smaller text size
      gfx->setTextSize(1, 1, 0);
      gfx->printf("Battery: %.2f%%\n", Bdata.cellPercentage);
      gfx->printf("Voltage: %.2f V\n", Bdata.cellVoltage);
      gfx->printf("Battery rate: %.4f\n", Bdata.chargeRate);
      Serial.printf("Battery: %.2f%%\n", Bdata.cellPercentage);
      Serial.printf("Voltage: %.2f V\n", Bdata.cellVoltage);
      Serial.printf("Battery rate: %.4f\n", Bdata.chargeRate);

      vTaskDelay(1000 / portTICK_PERIOD_MS);

    } 
  }

  Serial.println("Restarting");
  esp_restart();
}

bool playVideoWithAudio(int channel) {
  
  Serial.printf("videoIndex: %d\n", channel);
  char aFilePath[40];
  sprintf(aFilePath, "%s%d%s", BASE_PATH, channel, MP3_FILENAME);

  File aFile = SD.open(aFilePath);
  if (!aFile || aFile.isDirectory()) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    gfx->printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    return false;
  }

  char vFilePath[40];
  sprintf(vFilePath, "%s%d%s", BASE_PATH, channel, MJPEG_FILENAME);

  File vFile = SD.open(vFilePath);
  if (!vFile || vFile.isDirectory()) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", vFilePath);
    gfx->printf("ERROR: Failed to open %s file for reading\n", vFilePath);
    return false;
  }

  Serial.println("Init video");

  mjpeg_setup(&vFile, MJPEG_BUFFER_SIZE, drawMCU, false, DECODEASSIGNCORE, DRAWASSIGNCORE);

  Serial.println("Start play audio task");

  BaseType_t ret = mp3_player_task_start(&aFile, AUDIOASSIGNCORE);//aac_player_task_start(&aFile, AUDIOASSIGNCORE);

  if (ret != pdPASS) {
    Serial.printf("Audio player task start failed: %d\n", ret);
    gfx->printf("Audio player task start failed: %d\n", ret);
  }

  Serial.println("Start play video");
  playVideo(vFile, aFile);

  Serial.println("AV end");

  vFile.close();
  aFile.close();

  return true;
}

void playVideo(File vFile, File afile){
  
  start_ms = millis();
  curr_ms = millis();
  next_frame_ms = start_ms + (++next_frame * 1000 / FPS / 2);
  while (vFile.available() && mjpeg_read_frame())  // Read video
  {
    total_read_video_ms += millis() - curr_ms;
    curr_ms = millis();

    if (millis() < next_frame_ms)  // check show frame or skip frame
    {
      // Play video
      mjpeg_draw_frame();
      total_decode_video_ms += millis() - curr_ms;
      curr_ms = millis();
    } else {
      ++skipped_frames;
      Serial.println("Skip frame");
    }

    while (millis() < next_frame_ms) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    curr_ms = millis();
    next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
  }
  int time_used = millis() - start_ms;
  int total_frames = next_frame - 1;
  
  if (_decodeTask) {
      vTaskDelete(_decodeTask);
      _decodeTask = NULL;
      Serial.println("Force Kill Decode Task");
  }

  if (_draw_task) {
      vTaskDelete(_draw_task);
      _draw_task = NULL;
      Serial.println("Force Kill Draw Task");
  }

  for (int i = 0; i < NUMBER_OF_DECODE_BUFFER; ++i) {
    if (_mjpegBufs[i].buf) {
        free(_mjpegBufs[i].buf);
        _mjpegBufs[i].buf = NULL;
    }
  }

  if (_read_buf) {
    free(_read_buf);
    _read_buf = NULL;
  }

  for (int i = 0; i < NUMBER_OF_DRAW_BUFFER; i++) {
    if (jpegdraws[i].pPixels) {
        heap_caps_free(jpegdraws[i].pPixels);
        jpegdraws[i].pPixels = NULL;
    }
  }

  _inputindex = 0;
  _mjpeg_buf_offset = 0;
  _mBufIdx = 0;
  next_frame = 0;
  skipped_frames = 0;
}

