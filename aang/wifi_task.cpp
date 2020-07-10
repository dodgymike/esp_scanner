#include "wifi_task.h"

#include "WiFi.h"
#include "devices_history.h"

String translateEncryptionType(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case (0):
      return "Open";
    case (1):
      return "WEP";
    case (2):
      return "WPA_PSK";
    case (3):
      return "WPA2_PSK";
    case (4):
      return "WPA_WPA2_PSK";
    case (5):
      return "WPA2_ENTERPRISE";
    default:
      return "UNKOWN";
  }
}

void wifiTask(void* parameter) {
  DevicesHistory* devicesHistory = (DevicesHistory*)parameter;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  while(true) {
    int n = WiFi.scanNetworks();
    
    if ( xSemaphoreTake( devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
      /*
      for (int i = 0; i < n; ++i) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(") ");
        Serial.print(" [");
        Serial.print(WiFi.channel(i));
        Serial.print("] ");
        String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
        Serial.println(encryptionTypeDescription);
      }
      */
      for (int i = 0; i < n; ++i) {
          char deviceAddress[200];
          char bssid[100];
          char ssidBuffer[100];
            
          bzero(deviceAddress, 200);
          bzero(bssid, 100);
          bzero(ssidBuffer, 100);

          //WiFi.SSID(ssid);

//          WiFi.BSSID(i).copy(bssid, 80);
//          WiFi.SSID(i).copy(ssid, 80);

//          snprintf(deviceAddress, 80, "%x:%x:%x:%x:%x:%x\n%s", WiFi.BSSID(i)[0], WiFi.BSSID(i)[1], WiFi.BSSID(i)[2], WiFi.BSSID(i)[3], WiFi.BSSID(i)[4], WiFi.BSSID(i)[5], ssid);
          //snprintf(deviceAddress, 80, "%s\n%s", WiFi.BSSIDstr(i), WiFi.SSID(i));
          //snprintf(deviceAddress, 80, "%s", WiFi.SSID(i));
          String ssid = WiFi.SSID(i);
          //ssid.copy(ssidBuffer, 80);
          
          sprintf(deviceAddress, "%s\n%x:%x:%x:%x:%x:%x", ssid.c_str(), WiFi.BSSID(i)[0], WiFi.BSSID(i)[1], WiFi.BSSID(i)[2], WiFi.BSSID(i)[3], WiFi.BSSID(i)[4], WiFi.BSSID(i)[5]);
            
                      
          int foundDeviceIndex = -1;
          for(int deviceIndex = 0; deviceIndex < devicesHistory->getCount(); deviceIndex++) {
             if(devicesHistory->history[deviceIndex].checkName(deviceAddress)) {
                  foundDeviceIndex = deviceIndex;
                  break;
             }
          }
          
          if(foundDeviceIndex == -1) {
            devicesHistory->incrementCount();
            foundDeviceIndex = devicesHistory->getCount() - 1;

//            if(foundDevice.haveName()) {
//              strncpy(deviceAddress, foundDevice.getName(), 180);
//              bzero(deviceAddress, 180);
//              foundDevice.getName().copy(deviceAddress, 20);
//            }
            
            devicesHistory->history[foundDeviceIndex].setName(deviceAddress);
          }
          
          devicesHistory->history[foundDeviceIndex].setSignalLevel(WiFi.RSSI(i));
      }
      
      xSemaphoreGive( devicesHistory->xDevicesSemaphore );
    }
  
    delay(50);
  }
}



/*
  DevicesHistory* devicesHistory = (DevicesHistory*)parameter;
  
  int scanTime = 1; //In seconds
  int scanIndex = 0;
  
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  while(true) {
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

    if ( xSemaphoreTake( devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
      for(int i = 0; i < foundDevices.getCount(); i++) {
          BLEAdvertisedDevice foundDevice = foundDevices.getDevice(i);

          char deviceAddress[200];
          bzero(deviceAddress, 200);
          foundDevice.getAddress().toString().copy(deviceAddress, 180);
          
          int foundDeviceIndex = -1;
          for(int deviceIndex = 0; deviceIndex < devicesHistory->getCount(); deviceIndex++) {
             if(devicesHistory->history[deviceIndex].checkName(deviceAddress)) {
                  foundDeviceIndex = deviceIndex;
                  break;
             }
          }
          
          if(foundDeviceIndex == -1) {
            devicesHistory->incrementCount();
            foundDeviceIndex = devicesHistory->getCount() - 1;

            if(foundDevice.haveName()) {
//              strncpy(deviceAddress, foundDevice.getName(), 180);
              bzero(deviceAddress, 180);
              foundDevice.getName().copy(deviceAddress, 20);
            }
            
            devicesHistory->history[foundDeviceIndex].setName(deviceAddress);
          }
          
          devicesHistory->history[foundDeviceIndex].setSignalLevel(foundDevice.getRSSI());
      }
      
      xSemaphoreGive( devicesHistory->xDevicesSemaphore );
    }
         
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    delay(100);
  }
 */
