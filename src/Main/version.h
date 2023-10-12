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
#define FIRMWARE_VER "v0.12_pre_release"

#define APP_NAME "video_player"

#define AUDIO_GAIN_FILE_PATH "/Config/AudioGain.txt"
#define BASE_PATH "/Videos/"
#define AAC_FILENAME "/test.aac"
#define MP3_FILENAME "/test.mp3"
#define MJPEG_FILENAME "/test.mjpeg"


//Battery Metric
#define BATTERY_LOW 4.1
#define LOW_BATTERY_COUNTDOWN 60 //seconds
#define PLUGGED_IN_THRESHOLD -2.0