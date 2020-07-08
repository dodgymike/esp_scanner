#ifndef _DEVICES_HISTORY_H_
#define _DEVICES_HISTORY_H_

#include <Arduino.h>

#define DEVICE_ADDRESS_SIZE (50)
#define DEVICE_HISTORY_SIZE (120)
#define DEVICE_HISTORY_BUFFERS (1)

class DeviceHistory {
    int signalLevels[DEVICE_HISTORY_BUFFERS][DEVICE_HISTORY_SIZE];
    int signalLevelsIndex[DEVICE_HISTORY_BUFFERS];
    int signalLevelBufferIndex;
  public:
    char name[DEVICE_ADDRESS_SIZE];

    int getSignalLevelBufferIndex();
    int getSignalLevelsIndex();
    int getSignalLevelsIndex(int bufferIndex);
    int* getSignalLevels();
    int* getSignalLevels(int bufferIndex);
    void switchBuffer();
    void setSignalLevel(int signalLevel);
    void setSignalLevelBuffer(int signalLevelBufferIndexIn);

    DeviceHistory();

    void setName(const char* name);
    bool checkName(const char* nameToCheck);
    void copySignalLevels(int signalLevels[]);
};


#define LOCATION_HISTORY_BUFFERS (3)

class DevicesHistory {
  private:
    int count;
    SemaphoreHandle_t xCountSemaphore;
      
  public:
    SemaphoreHandle_t xDevicesSemaphore;
    DeviceHistory history[100];

    int locationSignalLevels[LOCATION_HISTORY_BUFFERS][DEVICE_HISTORY_SIZE];
    int locationSignalLevelsIndex[LOCATION_HISTORY_BUFFERS];
    int phase;

    int getCount();
    void incrementCount();

    DevicesHistory();
};

#endif
