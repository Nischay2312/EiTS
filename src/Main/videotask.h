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
uint32_t pauseStartTime;
uint32_t pauseDuration;

//Queue to get info from the main task
static xQueueHandle eventQueueMain;
//Queue to send info to the main task
static xQueueHandle eventQueueMainTx;
//Queue to send info to the video Player task
static xQueueHandle eventQueueVideo;
//Queue to notify the mp3 task.
static xQueueHandle mp3QueueEvent;
//Queue to notify the player task from mp3.
static xQueueHandle mp3QueueTransmitt;
//handle for video player task
static xTaskHandle _videoPlayerTask;

//video event enum
typedef enum{
    VIDEO_FINISHED,
    VIDEO_PLAYING,
    VIDEO_PAUSED
}videoEvent_enum;

//notification enum for the mp3 task
typedef enum{
    MP3_STOP_C,
    MP3_STOP,
    MP3_PAUSE_C,
    MP3_PAUSE,
    MP3_RESUME_C,
    MP3_RESUME,
    MP3_NORMAL
}mp3Event_enum;

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
    eventQueueMainTx = xQueueCreate(1, sizeof(uint8_t));
    if (eventQueueMainTx == NULL)
    {
        Serial.println("Video Event Transfer to main Queue Creation Failed");
        while(1){
          Serial.println("Video event Transfer to main Queue Creation Failed");
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
    mp3QueueTransmitt = xQueueCreate(1, sizeof(uint8_t));
    if (mp3QueueTransmitt == NULL)
    {
        Serial.println("MP3 Transmitt Queue Creation Failed");
        while(1){
          Serial.println("MP3 Transmitt Queue Creation Failed");
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
  pauseStartTime = 0;
  pauseDuration = 0;
  
  Serial.println("Closing the video file");
  //close the file
    vFile.close();
}

// function to close the audio player
void closeAudioPlayer(){
  //vTaskDelay(50/ portTICK_PERIOD_MS);
  uint8_t notif = MP3_NORMAL;
  //read the queue and see if the mp3 task killed itself.
  if((xQueueReceive(mp3QueueTransmitt, &notif, pdMS_TO_TICKS(5)))){
    if(notif == MP3_STOP){
      Serial.println("Mp3 Task killed itself");
      aFile.close();
      return;
    }
    else{
        //put the notification back in the queue
        xQueueOverwrite(mp3QueueTransmitt, &notif);
    }
  }
  
  //send queue notification to close the mp3 task
  Serial.println("Sending notification to close the mp3 task");
  notif = MP3_STOP_C;
  xQueueOverwrite(mp3QueueEvent, &notif);
  //wait for the notification from the audio player task
  while(1){
    if((xQueueReceive(mp3QueueTransmitt, &notif, portMAX_DELAY))){
      Serial.println("mp3queue from mp3 task Received: ");
      Serial.println(notif);
      if(notif == MP3_STOP){
        Serial.println("Audio player force stopped playback");
        break;
      }
      else{
        //put the notification back in the queue
        xQueueOverwrite(mp3QueueEvent, &notif);
      }
    }
    Serial.println("Waiting to hear back from the AudioTask if PAUSED");
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  //close the file
  Serial.println("closing audio file");
  aFile.close();
}

// function to stop the playback
void stopPlayback(){
  //stop the video playack task
  if(_videoPlayerTask){
    vTaskSuspend(_videoPlayerTask);
  }
  //stop the playback
  closeVideoPlayer();
  closeAudioPlayer();
  if(_videoPlayerTask){
    vTaskDelete(_videoPlayerTask);
  }
}

//function to pause the playback
void pausePlayback(){
  //pause the playback
    Serial.println("Pausing the playback");

    pauseStartTime = millis();

    //suspend the video player task and the draw and decode tasks
    if(_videoPlayerTask){
      vTaskSuspend(_videoPlayerTask);
    }
    if(_draw_task && _decodeTask){
      vTaskSuspend(_draw_task);
      vTaskSuspend(_decodeTask);
    }

    //for the audio player task, send a notification to pause the playback
    uint8_t notif = MP3_PAUSE_C;
    xQueueOverwrite(mp3QueueEvent, &notif);
    //give a delay to make sure the audio player task has paused the playback
    vTaskDelay(20 / portTICK_PERIOD_MS);
    //wait for the notification from the audio player task
    while(1){
      if((xQueueReceive(mp3QueueTransmitt, &notif, portMAX_DELAY))){
        Serial.println("Pause Task Received: mp3queue from mp3 task Received: ");
        Serial.println(notif);
        if(notif == MP3_PAUSE){
          Serial.println("Audio player task paused the playback");
          break;
        }
        else{
          //put the notification back in the queue
          xQueueOverwrite(mp3QueueEvent, &notif);
        }
      }
      Serial.println("Waiting to hear back from the AudioTask if PAUSED");
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

//function to resume the playback
void resumePlayback(){
    //resume the playback
        Serial.println("Resuming the playback");

        pauseDuration = millis() - pauseStartTime + pauseDuration;
        // xQueueSend(pauseDurationQueue, &pauseDuration, portMAX_DELAY);
        // xSemaphoreGive(videoSemaphore);  // Signal to resume the video
        //resume the video player task and the draw and decode tasks
        if(_videoPlayerTask){
        vTaskResume(_videoPlayerTask);
        }
        if(_draw_task && _decodeTask){
        vTaskResume(_draw_task);
        vTaskResume(_decodeTask);
        }
    
        //for the audio player task, send a notification to resume the playback
        uint8_t notif = MP3_RESUME_C;
        xQueueOverwrite(mp3QueueEvent, &notif);
        //give a delay to make sure the audio player task has resumed the playback
        vTaskDelay(20 / portTICK_PERIOD_MS);
        //wait for the notification from the audio player task
        while(1){
        if((xQueueReceive(mp3QueueTransmitt, &notif, portMAX_DELAY))){
            Serial.println("Resume Task: mp3queue from mp3 task Received: ");
            Serial.println(notif);
            if(notif == MP3_RESUME){
            Serial.println("Audio player task resumed the playback");
            break;
            }
            else{
            //put the notification back in the queue
            xQueueOverwrite(mp3QueueEvent, &notif);
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        }
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

    if (millis() - pauseDuration < next_frame_ms)  // check show frame or skip frame
    {
      // Play video
      mjpeg_draw_frame();
      total_decode_video_ms += millis() - curr_ms;
      curr_ms = millis();
    } else {
      ++skipped_frames;
      Serial.println("Skip frame");
    }

    while (millis() - pauseDuration < next_frame_ms) {
      vTaskDelay(pdMS_TO_TICKS(5));
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
      (const uint32_t)10000,
      (void *const)index,
      (UBaseType_t)configMAX_PRIORITIES - 3,
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

    uint8_t videoEventTx = 0;

    bool isPlaying = false;
    bool videoPaused = false;

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
                //check if the video is paused
                if(videoPaused && isPlaying){
                    Serial.println("Resuming playback");
                    videoPaused = false;
                    isPlaying = true;
                    videoEventTx = VIDEO_PLAYING;
                    xQueueOverwrite(eventQueueMainTx, &videoEventTx);
                    resumePlayback();
                }
                else if(isPlaying){
                    Serial.println("Pausing playback");
                    videoPaused = true;
                    videoEventTx = VIDEO_PAUSED;
                    xQueueOverwrite(eventQueueMainTx, &videoEventTx);
                    pausePlayback();
                }
                // if(isPlaying){
                //     Serial.println("Playback Stopped");
                //     isPlaying = false;
                //     stopPlayback();
                // }
                else{
                    Serial.println("Starting playback");
                    isPlaying = true;
                    videoPaused = false;

                    //start the task that plays the video
                    int ret = startVideoPlayerTask(AUDIOASSIGNCORE, video_idx);
                    if(ret == 2){
                        //video file not found, reset the video index
                        video_idx = 1; //reset it
                        int ret2 =  startVideoPlayerTask(AUDIOASSIGNCORE, video_idx);
                        if(ret2 == 2){ //file not found again
                            Serial.println("Video Player File Not found.");
                            Serial.println("No video File found. Make sure you have the correct video files in the SD card");
                            //break;
                        }
                        if(ret2 == 0){
                            Serial.println("Video Player Task Failed");
                            //break;
                            esp_restart();
                        }
                    }
                    if(ret == 0){
                        Serial.println("Video Player Task Failed");
                        //break;
                        esp_restart();
                    }
                    videoEventTx = VIDEO_PLAYING;
                    xQueueOverwrite(eventQueueMainTx, &videoEventTx);
                    Serial.println("Playback Start Sucessful");
                }
                //vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            else if(event.Type == BUTTON_DOUBLE_PRESSED){
                Serial.println("Video control task received a double press");
                if(videoPaused || !isPlaying){ //dont do anything+ unless video is paused. or stopped
                  if(!videoPaused && isPlaying){
                    //the video
                    videoPaused = true;
                    isPlaying = true;
                    pausePlayback();
                  }
                
                  //stop the video if playing
                  if(isPlaying && videoPaused){
                      Serial.println("Stopping playback to restart");
                      isPlaying = false;
                      videoPaused = false;
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
                        //break;
                      }
                      if(ret2 == 0){
                          Serial.println("Video Player Task Failed");
                          //break;
                          esp_restart();
                      }
                  }
                  if(ret == 0){
                      Serial.println("Video Player Task Failed");
                      //break;
                      esp_restart();
                  }
                  Serial.println("switch successful");
                  videoEventTx = VIDEO_PLAYING;
                  xQueueOverwrite(eventQueueMainTx, &videoEventTx);
                  Serial.println("Playback Start Sucessful");
                  //vTaskDelay(50 / portTICK_PERIOD_MS);
                }
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
                    videoPaused = false;
                    videoEventTx = VIDEO_FINISHED;
                    xQueueOverwrite(eventQueueMainTx, &videoEventTx);
                    break;
                default:
                    Serial.println("Unrecognized Video Event");
                    break;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.println("Video Control Task has ended. This should not happen! Check the serial monitor for more info");
    
    //delete the queues and tasks
    // vQueueDelete(eventQueueVideo);
    // vQueueDelete(mp3QueueEvent);
    // vQueueDelete(mp3QueueTransmitt);
    // closeVideoPlayer();
    // vTaskDelete(_mp3_player_task);

    // //send a notification to main loop for task restart.
    // event.Type = RESTARTME;
    // xQueueOverwrite(eventQueueMain, &event);
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
      (UBaseType_t)configMAX_PRIORITIES - 1,
      (TaskHandle_t *const)NULL,
      (const BaseType_t)assignedCore);

    if(ret != pdPASS)
    {
      Serial.println("Video Control Task Creation Failed");
      return 0;
    }
    return 1;
}
