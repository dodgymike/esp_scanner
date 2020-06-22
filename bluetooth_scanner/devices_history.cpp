#include "devices_history.h"

DeviceHistory::DeviceHistory()
  : name(""), signalLevelsIndex(0), signalLevelBufferIndex(0)
{
  for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
    signalLevels[0][i] = -100;
    signalLevels[1][i] = -100;
  }
}

bool DeviceHistory::checkName(const char* nameToCheck) {
  return (strncmp(name, nameToCheck, DEVICE_ADDRESS_SIZE) == 0);
}

void DeviceHistory::setName(const char* nameIn) {
    strncpy(name, nameIn, (DEVICE_ADDRESS_SIZE - 1));
    name[(DEVICE_ADDRESS_SIZE - 1)] = 0;
}

int* DeviceHistory::getSignalLevels() {
  return signalLevels[signalLevelBufferIndex];
}

void DeviceHistory::setSignalLevel(int signalLevel) {
  signalLevels[signalLevelBufferIndex][signalLevelsIndex] = signalLevel;
  
  signalLevelsIndex++;
  if(signalLevelsIndex >= DEVICE_HISTORY_SIZE) {
    signalLevelsIndex = 0;
  }
}

void DeviceHistory::setSignalLevelBuffer(int signalLevelBufferIndexIn) {
  signalLevelBufferIndex = signalLevelBufferIndexIn;
}

int DeviceHistory::getSignalLevelsIndex() {
  return signalLevelsIndex;
}

int DevicesHistory::getCount() {
  int rv = 0;
  
  if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    rv = count;
    xSemaphoreGive(xCountSemaphore);
  }      

  return rv;
}

void DevicesHistory::incrementCount() {
  if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    count++;
    xSemaphoreGive(xCountSemaphore);
  }      
}

DevicesHistory::DevicesHistory()
  : count(0), xDevicesSemaphore(xSemaphoreCreateMutex()),  xCountSemaphore(xSemaphoreCreateMutex())
{
  xSemaphoreGive(xDevicesSemaphore);
  xSemaphoreGive(xCountSemaphore);
}
