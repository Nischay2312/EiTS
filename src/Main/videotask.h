// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   videotask.h
//   Header file that contains the code for task that controls the video playback.
// */
// /////////////////////////////////////////////////////////////////

/* Variables */
static int next_frame = 0;
static int skipped_frames = 0;
static unsigned long start_ms, curr_ms, next_frame_ms;
static unsigned int video_idx = 1;

//Queue to get info from the main task
static xQueueHandle eventQueueMain;
//Queue to send info to the video Player task
static xQueueHandle eventQueueVideo;
//Queue to notify the mp3 task to stop playing.
static xQueueHandle mp3QueueEvent;
//handle for video player task
static xTaskHandle _videoPlayerTask;

/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
Arduino_DataBus *bus = new Arduino_ESP32SPI(DC, CS, SCK, MOSI, MISO, VSPI);
Arduino_GFX *gfx = new Arduino_ST7735(bus, RST /* RST */, 1 /* rotation */, false /* IPS */, ST7735_TFTWIDTH /* w*/, ST7735_TFTHEIGHT /* h*/,
                                      2 /* col_offset1 */, 1 /* row_offset1 */, 2 /* col_offset2 */, 1 /* row_offset2 */, false /* bgr */);

/* Audio */
#include "audiotask.h"
File aFile;

/* MJPEG Video */
#include "decodetask.h"
void playVideo();
File vFile;

//video event enum
typedef enum{
    VIDEO_FINISHED
}videoEvent_enum;

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw) {
  unsigned long s = millis();
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  total_show_video_ms += millis() - s;
  return 1;
} /* drawMCU() */

// function to initialize the button event queue
void intializeButtonEventQueue(){
    eventQueueMain = xQueueCreate(1, sizeof(buttonData));
    if (eventQueueMain == NULL)
    {
        Serial.println("Button Event Transfer Queue Creation Failed");
        while(1){
          Serial.println("Button Event Transfer Queue Creation Failed");
          vTaskDelay(1000 / portTICK_PERIOD_MS);
        };
    }
}

// function to initialize the button event queue
void intializeMP3EventQueue(){
    mp3QueueEvent = xQueueCreate(1, sizeof(uint8_t));
    if (mp3QueueEvent == NULL)
    {
        Serial.println("MP3 Queue Creation Failed");
        while(1){
          Serial.println("MP3 Queue Creation Failed");
          vTaskDelay(1000 / portTICK_PERIOD_MS);
        };
    }
}

// function to initialize the video event queue
void intializeVideoEventQueue(){
    eventQueueVideo = xQueueCreate(1, sizeof(uint8_t));
    if (eventQueueVideo == NULL)
    {
        Serial.println("Video Event Transfer Queue Creation Failed");
        while(1){
          Serial.println("Video Event Transfer Queue Creation Failed");
          vTaskDelay(1000 / portTICK_PERIOD_MS);
        };
    }
}

// function to close the video player
void closeVideoPlayer(){
  Serial.println("Trying to kill the video playback tasks");
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

  Serial.println("Freeing up the buffers");
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

  Serial.println("Closing the video file");
  //close the file
    vFile.close();

}

// function to close the audio player
void closeAudioPlayer(){
  vTaskDelay(50/ portTICK_PERIOD_MS);
  // if (_mp3_player_task) {
  //     vTaskDelete(_mp3_player_task);
  //     _mp3_player_task = NULL;
  //     Serial.println("Force Kill Audio Task");
  // }

  uint8_t notif = 0;

  //read the queue and see if the mp3 task killed itself.
  if((xQueueReceive(mp3QueueEvent, &notif, 0) == pdPASS)){
    if(notif == 2){
      Serial.println("Mp3 Task killed itself");
      aFile.close();
      return;
    }
  }

  //other wise tell it to close itself.
  //send queue notification
  Serial.println("Sending notification to close the mp3 task");
  notif = 1;
  xQueueOverwrite(mp3QueueEvent, &notif);
  // while(_mp3_player_task != NULL){
  //   Serial.println("Waiting for mp3_player_Task to be dead");
  //   vTaskDelay(10/ portTICK_PERIOD_MS);
  // }
  // notif = 0;
  // xQueueOverwrite(mp3QueueEvent, &notif);
  //_mp3.end();
  //close the file
  Serial.println("closing audio file");
  aFile.close();
}

// function to stop the playback
void stopPlayback(){
  //stop the playback
  closeVideoPlayer();
  closeAudioPlayer();
}

// function to control the video playback
void playVideo(){
  
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

  //close the players
  Serial.println("Video Playing Finished now closing the players");
  closeVideoPlayer();
  closeAudioPlayer();
}


// Function to play a video file
bool playVideoWithAudio(int channel) {
  
  Serial.printf("videoIndex: %d\n", channel);
  char aFilePath[40];
  sprintf(aFilePath, "%s%d%s", BASE_PATH, channel, MP3_FILENAME);

  aFile = SD.open(aFilePath);
  if (!aFile || aFile.isDirectory()) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    gfx->printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    return false;
  }

  char vFilePath[40];
  sprintf(vFilePath, "%s%d%s", BASE_PATH, channel, MJPEG_FILENAME);

  vFile = SD.open(vFilePath);
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
  playVideo();

  Serial.println("AV end");

//   vFile.close();
//   aFile.close();

  return true;
}

// The task that plays the video
static void videoPlayerTask(void *arg){
    Serial.println("Video Player task Starting the video");
    //get the index to play from the argument.
    unsigned int index = (unsigned int)arg;
    int ret = playVideoWithAudio(index);

    //notify the video control task that the video has finished
    Serial.println("Video Player task finished the video");
    uint8_t videoEvent = VIDEO_FINISHED;
    xQueueOverwrite(eventQueueVideo, &videoEvent);
    
    vTaskDelete(NULL);
}

// function to start the video player task
int startVideoPlayerTask(BaseType_t assignedCore, unsigned int index){

    Serial.println("Starting Video Player Task");

    //first check if the SD card has the video files for the index
    Serial.println("Checking if video and audio files exist");
    bool videoExists = true;
    bool audioExists = true;

    char aFilePath[40];
    sprintf(aFilePath, "%s%d%s", BASE_PATH, index, MP3_FILENAME);

    File _aFile = SD.open(aFilePath);
    if (!_aFile || _aFile.isDirectory()) {
        audioExists = false;
    }

    char vFilePath[40];
    sprintf(vFilePath, "%s%d%s", BASE_PATH, index, MJPEG_FILENAME);

    File _vFile = SD.open(vFilePath);
    if (!_vFile || vFile.isDirectory()) {
        videoExists = false;
    }

    //close the files
    _aFile.close();
    _vFile.close();

    if(!videoExists || !audioExists){
        Serial.println("Video or Audio File does not exist");
        return 2;
    }
    BaseType_t ret = xTaskCreatePinnedToCore(
      (TaskFunction_t)videoPlayerTask,
      (const char *const)"Video Player Task",
      (const uint32_t)40000,
      (void *const)index,
      (UBaseType_t)configMAX_PRIORITIES - 4,
      (TaskHandle_t *const)_videoPlayerTask,
      (const BaseType_t)assignedCore);

    if(ret != pdPASS)
    {
      Serial.println("Video Player Task Creation Failed");
      return 0;
    }
    return 1;
}

// The task that controls the video playback
static void videoControlTask( void *arg){

    Serial.println("Video Control Task Start!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    //create a button event object
    buttonData event;
    video_idx = 1;

    bool isPlaying = false;

    //initalize the video event queue
    uint8_t videoEvent = 0;
    intializeVideoEventQueue();

    //initialize the mp3 event queue
    intializeMP3EventQueue();

    //wait for the event from the main task
    while(1){
        // wait for the button event
        if(xQueueReceive(eventQueueMain, &event, 0)){
           Serial.println("VIDEO controller received a button event. now decoding");
            //decode the event
            if(event.Type == BUTTON_PRESSED){
                Serial.println("Video Control Task received a single press");
                if(isPlaying){
                    Serial.println("Playback Stopped");
                    isPlaying = false;
                    if(_videoPlayerTask){
                      vTaskDelete(_videoPlayerTask);
                    }
                    stopPlayback();
                }
                else{
                    Serial.println("Starting playback");
                    isPlaying = true;
                    //start the task that plays the video
                    int ret = startVideoPlayerTask(AUDIOASSIGNCORE, video_idx);
                    if(ret == 2){
                        //video file not found, reset the video index
                        video_idx = 1;
                        int ret2 =  startVideoPlayerTask(AUDIOASSIGNCORE, video_idx);
                        if(ret2 == 2){
                            Serial.println("Video Player File Not found.");
                            Serial.println("No video File found. Make sure you have the correct video files in the SD card");
                            break;
                        }
                        if(ret2 == 0){
                            Serial.println("Video Player Task Failed");
                            break;
                        }
                    }
                    if(ret == 0){
                        Serial.println("Video Player Task Failed");
                        break;
                    }
                    Serial.println("Playback Start Sucessful");
                }
            }
            else if(event.Type == BUTTON_DOUBLE_PRESSED){
                Serial.println("Video control task received a double press");
                //stop the video if playing
                if(isPlaying){
                    Serial.println("Stopping playback to restart");
                    isPlaying = false;
                    if(_videoPlayerTask){
                      vTaskDelete(_videoPlayerTask);
                    }
                    stopPlayback();
                }
                isPlaying = true;
                //play the next video
                video_idx++;
                Serial.println("Trying to switch to next video");
                int ret = startVideoPlayerTask(AUDIOASSIGNCORE, video_idx);
                if(ret == 2){
                    //video file not found, reset the video index
                    video_idx = 1;
                    int ret2 =  startVideoPlayerTask(AUDIOASSIGNCORE, video_idx);
                    if(ret2 == 2){
                        Serial.println("Video Player File Not found.");
                        Serial.println("No video File found. Make sure you have the correct video files in the SD card");
                        break;
                    }
                    if(ret2 == 0){
                        Serial.println("Video Player Task Failed");
                        break;
                    }
                }
                if(ret == 0){
                    Serial.println("Video Player Task Failed");
                    break;
                }
                Serial.println("switch successful");
            }
        }
        
        // wait for the video event
        if(xQueueReceive(eventQueueVideo, &videoEvent, 0)){
            Serial.println("Received an event from video player task");
            //decode the event
            switch(videoEvent){
                case VIDEO_FINISHED:
                    Serial.println("Video playing finished");
                    isPlaying = false;
                    break;
                default:
                    Serial.println("Unrecognized Video Event");
                    break;
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    Serial.println("Video Control Task has ended. This should not happen! Check the serial monitor for more info");
    vTaskDelete(NULL);
}

// function to start the video control task
int startVideoControlTask(BaseType_t assignedCore){

    Serial.println("Starting Video Control Task");
    
    BaseType_t ret = xTaskCreatePinnedToCore(
      (TaskFunction_t)videoControlTask,
      (const char *const)"Video Control Task",
      (const uint32_t)40000,
      (void *const) NULL,
      (UBaseType_t)configMAX_PRIORITIES - 4,
      (TaskHandle_t *const)NULL,
      (const BaseType_t)assignedCore);

    if(ret != pdPASS)
    {
      Serial.println("Video Control Task Creation Failed");
      return 0;
    }
    return 1;
}
