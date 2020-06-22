#include "devices_history.h"

DeviceHistory::DeviceHistory()
  : name(""), signalLevelsIndex(0)
{}

bool DeviceHistory::checkName(const char* nameToCheck) {
  return (strncmp(name, nameToCheck, DEVICE_ADDRESS_SIZE) == 0);
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
