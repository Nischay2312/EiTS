// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   version.h
//   Header file that hosts all the version information for the project.
// */
// /////////////////////////////////////////////////////////////////
#define COMPILE_TIME __TIME__
#define COMPILE_DATE __DATE__
#define FIRMWARE_VER "v0.18_pre_release"

#define APP_NAME "video_player"

#define AUDIO_GAIN_FILE_PATH "/Config/AudioGain.txt"
#define LOOP_FILE_PATH "/Config/Loop.txt"
#define BASE_PATH "/Videos/"
#define VIDEO_MAX_MIN_SEARCH_PATH "/Videos"
#define AAC_FILENAME "/test.aac"
#define MP3_FILENAME "/test.mp3"
#define MJPEG_FILENAME "/test.mjpeg"
#define SPLASH_SCREEN_FILE "/Config/SplashScreen.jpeg"

//Battery Metric
#define BATTERY_LOW 3.4
#define LOW_BATTERY_COUNTDOWN 120 //seconds
#define PLUGGED_IN_THRESHOLD -1.0
#define BATTERY_METRIC_INIT_TIME 40
#define SLEEP_TIME 10 * 60 //seconds  


