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
//#include <Preferences.h>


#include "version.h"


/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
Arduino_DataBus *bus = new Arduino_ESP32SPI(DC, CS, SCK, MOSI, MISO, VSPI);
Arduino_GFX *gfx = new Arduino_ST7735(bus, RST /* RST */, 1 /* rotation */, false /* IPS */, ST7735_TFTWIDTH /* w*/, ST7735_TFTHEIGHT /* h*/,
                                      2 /* col_offset1 */, 1 /* row_offset1 */, 2 /* col_offset2 */, 1 /* row_offset2 */, false /* bgr */);

/* Audio */
#include "audiotask.h"

/* MJPEG Video */
#include "decodetask.h"

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

  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);

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

  // preferences.begin(APP_NAME, false);
  video_idx = 1;//preferences.getUInt(K_VIDEO_INDEX, 1);
  Serial.printf("videoIndex: %d\n", video_idx);

  gfx->setCursor(20, 20);
  gfx->setTextColor(GREEN);
  gfx->setTextSize(6, 6, 0);
  gfx->printf("CH %d", video_idx);
  delay(1000);

  while(1){
    bool ret = playVideoWithAudio(video_idx);
    if(ret){
      video_idx = video_idx + 1;
    }
    else{
      break;
    }
  }

  Serial.println("Restarting");
  esp_restart();

}

void loop() {
}

bool playVideoWithAudio(int channel) {

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

  // while(taskState == 0){

  // }
  // while(taskState != 0){

  // }
  // taskState = 0;
  Serial.println("Start play video");

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

  Serial.println("AV end");
  vFile.close();
  aFile.close();

  return true;
  //videoController(1);
}

void videoController(int next) {

  video_idx += next;
  if (video_idx <= 0) {
    video_idx = VIDEO_COUNT;
  } else if (video_idx > VIDEO_COUNT) {
    video_idx = 1;
  }
  Serial.printf("video_idx : %d\n", video_idx);
  //preferences.putUInt(K_VIDEO_INDEX, video_idx);
  //preferences.end();
  esp_restart();
}
