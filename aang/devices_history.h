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
    int channel;

    SemaphoreHandle_t xDeviceSemaphore;
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

    void setChannel(int channel);
    int getChannel();

    DeviceHistory();

    void setName(const char* name);
    bool checkName(const char* nameToCheck);
    void copySignalLevels(int signalLevels[]);
};


#define LOCATION_HISTORY_BUFFERS (3)

#define DEVICES_HISTORY_SCAN_MODE_NONE    (-1)
#define DEVICES_HISTORY_SCAN_MODE_BTLE    (0)
#define DEVICES_HISTORY_SCAN_MODE_WIFI    (1)

class DevicesHistory {
  private:
    int count;
    SemaphoreHandle_t xCountSemaphore;
    int wifiChannel;
    int scanMode;
    char apAddress[DEVICE_ADDRESS_SIZE];
      
  public:
       
    SemaphoreHandle_t xDevicesSemaphore;
    DeviceHistory history[100];

    int locationSignalLevels[LOCATION_HISTORY_BUFFERS][DEVICE_HISTORY_SIZE];
    int locationSignalLevelsIndex[LOCATION_HISTORY_BUFFERS];
    int phase;

    void clean();

    int getCount();
    void incrementCount();
    
    void setWifiChannel(int wifiChannelIn);
    int getWifiChannel();

    void setScanMode(int scanModeIn);
    int getScanMode();

    void setApAddress(char* apAddress);
    const char* getApAddress();

    DevicesHistory();
};

#endif
