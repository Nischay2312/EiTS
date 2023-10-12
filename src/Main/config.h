// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   config.h
//   Header file that hosts all the pin definitions for the project.
// */
// /////////////////////////////////////////////////////////////////

#define ESP32S3


#ifndef ESP32S3
  //ST7735 TFT Display Pinouts
  #define CS 5
  #define SCK 18
  #define MOSI 23
  #define MISO -1
  #define DC 27
  #define RST 33
  #define BLK 22

  //I2S Audio Driver Pinouts
  #define I2S_MCLK -1
  #define I2S_SCLK 25
  #define I2S_LRCLK 26
  #define I2S_DOUT 32
  #define I2S_DIN -1

  //SD Card Pinouts
  #define SD_SCK 14
  #define SD_MOSI 15
  #define SD_MISO 4
  #define SD_CS 13

  //Button Pinouts
  #define BUTTON 8
  #define LCDBK 9
  #define BATEN 36
  #else
  //Esp32S3 pinouts
  //ST7735 TFT Display Pinouts
  #define MISO -1
  #define SCK 14
  #define MOSI 15
  #define DC 16
  #define RST 17
  #define CS 18
  #define BLK 22

  //I2S Audio Driver Pinouts
  #define I2S_MCLK -1
  #define I2S_DIN -1
  #define I2S_DOUT 5
  #define I2S_SCLK 6
  #define I2S_LRCLK 9
  
  //SD Card Pinouts
  #define SD_CS 10
  #define SD_MOSI 11
  #define SD_SCK 12
  #define SD_MISO 13

  //Button Pinouts
  #define BUTTON 37
  #define LCDBK 8
  #define BATEN 36
#endif
