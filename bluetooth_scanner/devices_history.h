#ifndef _DEVICES_HISTORY_H_
#define _DEVICES_HISTORY_H_

#include <Arduino.h>

#define DEVICE_ADDRESS_SIZE (50)
#define DEVICE_HISTORY_SIZE (80)

class DeviceHistory {
  public:
    char name[DEVICE_ADDRESS_SIZE];
    int signalLevels[DEVICE_HISTORY_SIZE];
    int signalLevelsIndex;

    DeviceHistory();
    bool checkName(const char* nameToCheck);
};

class DevicesHistory {
  private:
    int count;
    SemaphoreHandle_t xCountSemaphore;
      
  public:
    SemaphoreHandle_t xDevicesSemaphore;
    DeviceHistory history[100];

    int getCount();
    void incrementCount();

    DevicesHistory();
};

#endif
