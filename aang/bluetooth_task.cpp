#include "bluetooth_task.h"

#include "devices_history.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

void bluetoothTask(void* parameter) {
  Serial.println("STARTING BTLE TASK");

  DevicesHistory* devicesHistory = (DevicesHistory*)parameter;
  
  int scanTime = 1; //In seconds
  int scanIndex = 0;

  while(true) {
    Serial.println("BTLE Wait");
    delay(1000);
  }
  
/*
  for(int i = 0; i < 10; i++) {
    Serial.println("BTLE Wait");
    delay(1000);
  }

  while(devicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_BTLE) {
    Serial.println("NOT BTLE MODE");
    delay(1000);
  }
  
  for(int i = 0; i < 10; i++) {
    Serial.println("BTLE Wait 2");
    delay(1000);
  }
*/

  Serial.println("BEFORE BTLE AA");
  BLEDevice::init("");
  Serial.println("BEFORE BTLE A");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  Serial.println("BEFORE BTLE B");
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  Serial.println("BEFORE BTLE C");
  pBLEScan->setInterval(100);
  Serial.println("BEFORE BTLE D");
  pBLEScan->setWindow(99);  // less or equal setInterval value

  Serial.println("BEFORE BTLE LOOP");
  while(true) {
    Serial.println("STARTING BTLE LOOP");
    
    while(devicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_BTLE) {
      Serial.println("NOT BTLE MODE");
      delay(1000);
    }
    
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
}
