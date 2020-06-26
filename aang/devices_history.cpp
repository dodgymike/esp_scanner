#include "devices_history.h"

DeviceHistory::DeviceHistory()
  : name(""), signalLevelBufferIndex(0)
{
  for(int bufferIndex = 0; bufferIndex < DEVICE_HISTORY_BUFFERS; bufferIndex++) {
    for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
      signalLevels[bufferIndex][i] = -100;
      signalLevelsIndex[bufferIndex] = 0;
    }
  }
}

void DeviceHistory::switchBuffer() {
  signalLevelBufferIndex++;
  if(signalLevelBufferIndex >= DEVICE_HISTORY_BUFFERS) {
    signalLevelBufferIndex = 0;
  }

/*
*/
  for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
    signalLevels[signalLevelBufferIndex][i] = -100;
  }
  signalLevelsIndex[signalLevelBufferIndex] = 0;
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

int* DeviceHistory::getSignalLevels(int bufferIndex) {
  return signalLevels[bufferIndex];
}

void DeviceHistory::setSignalLevel(int signalLevel) {
  signalLevels[signalLevelBufferIndex][signalLevelsIndex[signalLevelBufferIndex]] = signalLevel;
  
  signalLevelsIndex[signalLevelBufferIndex]++;
  if(signalLevelsIndex[signalLevelBufferIndex] >= DEVICE_HISTORY_SIZE) {
    signalLevelsIndex[signalLevelBufferIndex] = 0;
  }
}

int DeviceHistory::getSignalLevelsIndex() {
  return signalLevelsIndex[signalLevelBufferIndex];
}

int DeviceHistory::getSignalLevelsIndex(int bufferIndex) {
  return signalLevelsIndex[bufferIndex];
}

int DeviceHistory::getSignalLevelBufferIndex() {
  return signalLevelBufferIndex;
}

int DevicesHistory::getCount() {
  int rv = 0;
  
  if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    rv = count;
    xSemaphoreGive(xCountSemaphore);
  }      

  return rv;
}

void DeviceHistory::copySignalLevels(int signalLevelsOut[]) {
  for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
    signalLevelsOut[i] = signalLevels[signalLevelBufferIndex][i];
  }
}

void DevicesHistory::incrementCount() {
  if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    count++;
    xSemaphoreGive(xCountSemaphore);
  }      
}

DevicesHistory::DevicesHistory()
  : count(0), xDevicesSemaphore(xSemaphoreCreateMutex()), xCountSemaphore(xSemaphoreCreateMutex())
{
  locationSignalLevelsIndex[0] = 0;
  locationSignalLevelsIndex[1] = 0;
  locationSignalLevelsIndex[2] = 0;

  phase = 0;

  for(int bufferIndex = 0; bufferIndex < LOCATION_HISTORY_BUFFERS; bufferIndex++) {
    for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
      locationSignalLevels[bufferIndex][i] = -100;
    }
  }
    
  xSemaphoreGive(xDevicesSemaphore);
  xSemaphoreGive(xCountSemaphore);
}
