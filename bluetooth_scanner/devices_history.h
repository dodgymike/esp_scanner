#ifndef _DEVICES_HISTORY_H_
#define _DEVICES_HISTORY_H_

#include <Arduino.h>

#define DEVICE_ADDRESS_SIZE (50)
#define DEVICE_HISTORY_SIZE (80)

class DeviceHistory {
    int signalLevels[2][DEVICE_HISTORY_SIZE];
    int signalLevelsIndex;
    int signalLevelBufferIndex;
  public:
    char name[DEVICE_ADDRESS_SIZE];

    int getSignalLevelsIndex();
    int* getSignalLevels();
    void setSignalLevelBuffer();
    void setSignalLevel(int signalLevel);
    void setSignalLevelBuffer(int signalLevelBufferIndexIn);

    DeviceHistory();

    void setName(const char* name);
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
