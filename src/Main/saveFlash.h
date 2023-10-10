// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   saveFlash.h
//   Header file that contains the code for saving and retrieving datafrom the flash memory
// */
// /////////////////////////////////////////////////////////////////

#include <Preferences.h>

#define NAMESPACE1 "BATTERY"
#define KEY1 "BINFO"

Preferences preferences;

// Function to initalize the preferences and search for the key if it exists
void initPreferences(void * batteryDataRcvd, size_t size) {
  preferences.begin(NAMESPACE1, false);
  if(preferences.isKey(KEY1)) {
    Serial.println("Key Found");
    //gfx->println("Key Found");
    //load the data from the preferences
    int ret = preferences.getBytes(KEY1, batteryDataRcvd, size);
    //gfx->printf("get : %d", ret);
    //delay(2000);

    
  } else {
    Serial.println("Key Not Found");
    //gfx->println("Key No Found");
    //create the key
    int ret = preferences.putBytes(KEY1, batteryDataRcvd, size); 
    //gfx->printf("put : %d", ret);
    //delay(2000);
    }
    preferences.end();
}


void savePreferences(void * batteryDataRcvd, size_t size) {
    preferences.begin(NAMESPACE1, false);
    preferences.putBytes(KEY1, batteryDataRcvd, size);
    preferences.end();
}
