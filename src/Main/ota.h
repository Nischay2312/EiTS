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

const char* ssid = "ESP_OTA";
const char* password = "ESP_OTA";

WebServer server(80);

unsigned long ota_progress_millis = 0;

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
    gfx->printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
    gfx->fillScreen(BLACK);
    gfx->setCursor(0,0);
    gfx->printf("OTA update finished successfully!\nRestarting in 10 seconds\n");
  } else {
    Serial.println("There was an error during OTA update!");
    gfx->fillScreen(BLACK);
    gfx->setCursor(0,0);
    gfx->printf("There was an error during OTA update!\nRestarting in 10 seconds\n");
  }

  //Restart the esp in 10 seconds
  for(int i = 0; i < 10; i++){
    delay(1000);
  }
  esp_restart();
}

void checkOTA(){
    //Set the Button Pin to Input with Pullup
    pinMode(BUTTON, INPUT_PULLUP);
    delay(250);
    bool do_OTA = false;

    //wait for 5 seconds and if the button is pressed, start OTA
    unsigned long timeStart = millis();
    while(millis() - timeStart < 5000){
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
    Serial.println("Connecting to WiFi..");
    while(WiFi.status() != WL_CONNECTED){
        delay(100);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
    
    //Start LCD display
    pinMode(LCDBK, OUTPUT);
    digitalWrite(LCDBK, HIGH);
    delay(100);

    // Init Display
    gfx->begin(80000000);
    gfx->fillScreen(BLACK);

    //prin the network name, password and IP address
    gfx->setCursor(0,0);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->printf("Connect to the network for OTA\n");
    gfx->printf("SSID: %s\n", ssid);
    gfx->printf("Password: %s\n", password);
    gfx->printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    
    //Serial print the info too
    Serial.printf("Connect to the network for OTA\n");
    Serial.printf("SSID: %s\n", ssid);
    Serial.printf("Password: %s\n", password);
    Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());

    server.on("/", []() {
        server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
        gfx->printf("Hi! This is ElegantOTA Demo.\n");
    });

    ElegantOTA.begin(&server);    // Start ElegantOTA
    // ElegantOTA callbacks
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);

    server.begin();
    Serial.println("HTTP server started");
    gfx->printf("HTTP server started\n");
    
    while(1){
        server.handleClient();
        ElegantOTA.loop();
    }
}