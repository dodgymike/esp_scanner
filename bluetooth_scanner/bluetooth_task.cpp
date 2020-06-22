#include "bluetooth_task.h"

#include "devices_history.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

void bluetoothTask(void* parameter) {
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
      
            strncpy(devicesHistory->history[foundDeviceIndex].name, deviceAddress, (DEVICE_ADDRESS_SIZE - 1));
            devicesHistory->history[foundDeviceIndex].name[(DEVICE_ADDRESS_SIZE - 1)] = 0;
            
            devicesHistory->history[foundDeviceIndex].signalLevelsIndex = 0;
            for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
              devicesHistory->history[foundDeviceIndex].signalLevels[i] = -100;
            }
          }
          
          devicesHistory->history[foundDeviceIndex].signalLevels[devicesHistory->history[foundDeviceIndex].signalLevelsIndex] = foundDevice.getRSSI();
          devicesHistory->history[foundDeviceIndex].signalLevelsIndex++;
          if(devicesHistory->history[foundDeviceIndex].signalLevelsIndex >= DEVICE_HISTORY_SIZE) {
            devicesHistory->history[foundDeviceIndex].signalLevelsIndex = 0;
          }
      }
      
      xSemaphoreGive( devicesHistory->xDevicesSemaphore );
    }
         
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    delay(100);
  }
}
