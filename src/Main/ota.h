// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   ota.h
//   Header file that contains the code for Over the Air (OTA) firmware loading, using the ElegantOTA library.
// */
// /////////////////////////////////////////////////////////////////

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#include <ElegantOTA.h>
#include "qrcode.h"

#include "ota_page.h"

const char* ssid = "ESP_OTA";
const char* password = "ESP_OTA_123";

WebServer server(80);

unsigned long ota_progress_millis = 0;
int previousClientCount = 0;

#define QRCODE_SIZE 3

QRCode qrcode;

//function to generate the QR code
void displayQRCode(String url) {
  uint8_t qrcodeData[qrcode_getBufferSize(QRCODE_SIZE)];
  qrcode_initText(&qrcode, qrcodeData, QRCODE_SIZE, 0, url.c_str());
  gfx->fillScreen(BLACK);

  // Display the QR code
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        gfx->fillRect(x*3, y*3, 3, 3, WHITE);
      }
    }
  }
  gfx->setCursor(0,100);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->printf("SCAN ME!!!\n");
}

void ShowOTAInfo(){
  //print the network name, password and IP address
    gfx->fillScreen(BLACK);
    gfx->setCursor(0,0);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->printf("Connect to the network for OTA\n");
    gfx->printf("SSID: %s\n", ssid);
    gfx->printf("Password: %s\n", password);
    gfx->printf("Type that on your browser:\n%s\n", WiFi.softAPIP().toString().c_str());
    
    //Serial print the info too
    Serial.printf("Connect to the network for OTA\n");
    Serial.printf("SSID: %s\n", ssid);
    Serial.printf("Password: %s\n", password);
    Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
}

void checkConnectedClients(String url) {
  int currentClientCount = WiFi.softAPgetStationNum();
  if (currentClientCount > previousClientCount) {
    // A client connected
    // Display the QR code
    displayQRCode(url);
  } else if (currentClientCount < previousClientCount) {
    // A client disconnected
    ShowOTAInfo();  // Assuming this function displays the connection info
  }
  previousClientCount = currentClientCount;
}


void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  //clear the display
  gfx->fillScreen(BLACK);
  gfx->setCursor(0,0);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->printf("OTA update started!\n");    
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    gfx->fillScreen(BLACK);
    gfx->setCursor(0,0);
    gfx->printf("OTA Progress:\nCurrent: %u bytes\nFinal: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
    gfx->fillScreen(BLACK);
    gfx->setCursor(0,0);
    gfx->printf("OTA update finished \nsuccessfully!\n");
  } else {
    Serial.println("There was an error during OTA update!");
    gfx->fillScreen(BLACK);
    gfx->setCursor(0,0);
    gfx->printf("There was an error \nduring OTA update!\n");
  }
  delay(1000);
  ShowOTAInfo();
}

void checkOTA(){
    //Set the Button Pin to Input with Pullup
    pinMode(BUTTON, INPUT_PULLUP);
    delay(250);
    bool do_OTA = false;

    //wait if the button is pressed, start OTA
    unsigned long timeStart = millis();
    while(millis() - timeStart < 200){
        if(digitalRead(BUTTON) == LOW){
            Serial.println("Button Pressed, Starting OTA");
            do_OTA = true;
            break;
        }
    }
    if(!do_OTA){
        Serial.println("Button not pressed, continuing to boot");
        return;
    }
    //start WiFi
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    //delay(1000);
    Serial.println("Connecting to WiFi..");
    Serial.println("Access Point Enabled");
    WiFi.softAP(ssid, password);
    while(WiFi.getMode() != WIFI_AP){
      delay(100);
      Serial.print(".");
    }
  Serial.println("AP Mode Activated");

    //Start LCD display
    pinMode(BATEN, OUTPUT);
    digitalWrite(BATEN, HIGH);
    delay(100);
    pinMode(LCDBK, OUTPUT);
    digitalWrite(LCDBK, HIGH);
    delay(100);
    
    //disable the speaker
    pinMode(SPEAKER_EN, OUTPUT);
    digitalWrite(SPEAKER_EN, LOW);


    // Init Display
    gfx->begin(80000000);
    gfx->fillScreen(BLACK);

    ShowOTAInfo();

    server.on("/", []() {
    server.send(200, "text/html", OTA_PAGE); // Serve the HTML content
    });

    server.on("/reboot", []() {
    server.send(200, "text/plain", "Rebooting ESP...");
    delay(1000);
    esp_restart();
    });

    ElegantOTA.begin(&server);    // Start ElegantOTA
    // ElegantOTA callbacks
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);

    server.begin();
    Serial.println("HTTP server started");
    gfx->printf("HTTP server started\n");

    //display the QR code
    String url = "http://" + WiFi.softAPIP().toString() + "/";
    
    while(1){
        server.handleClient();
        ElegantOTA.loop();
        checkConnectedClients(url);
    }
}