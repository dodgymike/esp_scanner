#include "devices_history.h"

DeviceHistory::DeviceHistory()
  : name(""), signalLevelBufferIndex(0), channel(0), xDeviceSemaphore(xSemaphoreCreateMutex())
{
  xSemaphoreGive(xDeviceSemaphore);

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

void DeviceHistory::copySignalLevels(int signalLevelsOut[]) {
  for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
    signalLevelsOut[i] = signalLevels[signalLevelBufferIndex][i];
  }
}

void DeviceHistory::setChannel(int channelIn) {
  if (xSemaphoreTake(xDeviceSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    channel = channelIn;
    xSemaphoreGive(xDeviceSemaphore);
  }      
}

int DeviceHistory::getChannel() {
  int rv = 0;
  
  if (xSemaphoreTake(xDeviceSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    rv = channel;
    xSemaphoreGive(xDeviceSemaphore);
  }      

  return rv;
}

int DevicesHistory::getCount() {
  int rv = 0;
  
  if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    rv = count;
    xSemaphoreGive(xCountSemaphore);
  }      

  return rv;
}

void DevicesHistory::clean() {
  if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {
    locationSignalLevelsIndex[0] = 0;
    locationSignalLevelsIndex[1] = 0;
    locationSignalLevelsIndex[2] = 0;
  
    phase = 0;
  
    for(int bufferIndex = 0; bufferIndex < LOCATION_HISTORY_BUFFERS; bufferIndex++) {
      for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
        locationSignalLevels[bufferIndex][i] = -100;
      }
    }
  
    count = 0;

    xSemaphoreGive(xCountSemaphore);
  }      
}

void DevicesHistory::incrementCount() {
  if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    count++;
    xSemaphoreGive(xCountSemaphore);
  }      
}

void DevicesHistory::setWifiChannel(int wifiChannelIn) {
  if (xSemaphoreTake(xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    wifiChannel = wifiChannelIn;
    xSemaphoreGive(xDevicesSemaphore);
  }      
}

int DevicesHistory::getWifiChannel() {
  int rv = 0;
  if (xSemaphoreTake(xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    rv = wifiChannel;
    xSemaphoreGive(xDevicesSemaphore);
  }      

  return rv;
}

void DevicesHistory::setScanMode(int scanModeIn) {
  if (xSemaphoreTake(xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    scanMode = scanModeIn;
    xSemaphoreGive(xDevicesSemaphore);
  }      
}

int DevicesHistory::getScanMode() {
  int rv = 0;
  if (xSemaphoreTake(xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
    rv = scanMode;
    xSemaphoreGive(xDevicesSemaphore);
  }      

  return rv;
}


DevicesHistory::DevicesHistory()
  : count(0), wifiChannel(0), xDevicesSemaphore(xSemaphoreCreateMutex()), xCountSemaphore(xSemaphoreCreateMutex()), scanMode(DEVICES_HISTORY_SCAN_MODE_NONE)
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
