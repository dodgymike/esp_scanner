#include <string.h>
#include <stdlib.h>
#include <math.h>

//#include "game.hpp"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <math.h>

//#include "bt.hpp"

//#include <Adafruit_I2CDevice.h>
//  
//#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
//#include <SPI.h>             // Arduino SPI library
  
// ST7789 TFT module connections
//#define TFT_CS    10  // define chip select pin
//#define TFT_DC     16  // define data/command pin
//#define TFT_RST    21  // define reset pin, or set to -1 and connect to Arduino RESET pin
//#define TFT_MISO 19
//#define TFT_MOSI 23
//#define TFT_SCLK 18

// 21 #define TFT_MISO 19
// 22 #define TFT_MOSI 23
// 23 #define TFT_SCLK 18
// 24 //#define TFT_CS    -1 // Not connected
// 25 //#define TFT_DC    2
// 26 #define TFT_DC    16
// 27 #define TFT_RST   21  // Connect reset to ensure display initialises

#include <TFT_eSPI.h>

#define BaudRate 115200

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define SCAN_NAME_SIZE (20)
#define GRAPH_HEIGHT (20.0f)

/*
struct ScanParameters {
  char scanNames[SCAN_NAME_SIZE][50];
  int scanRssis[SCAN_NAME_SIZE];
  int scanCount;
};
*/

#define DEVICE_HISTORY_SIZE 80

class DeviceHistory {
//  private:
  public:
    char name[50];
    int signalLevels[DEVICE_HISTORY_SIZE];
    int signalLevelsIndex;

    DeviceHistory()
      : name(""), signalLevelsIndex(0)
    {}

    bool checkName(const char* nameToCheck) {
      return (strncmp(name, nameToCheck, 50) == 0);
    }
};

//std::map<std::string, struct DeviceHistory*> devicesHistory;
//std::map<std::string, std::string> devicesHistory;

DeviceHistory devicesHistory[100];
int devicesHistoryCount;

void setup()
{   
  Serial.begin(BaudRate);

  tft.init();
  tft.setRotation(4);
  tft.fillScreen(TFT_BLACK);

  devicesHistoryCount = 0;
}


void loop()
{
  int scanTime = 1; //In seconds
  int scanIndex = 0;
  
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  while(true)
  {
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      tft.fillScreen(TFT_BLACK);
  
      for(int i = 0; i < foundDevices.getCount(); i++) 
      {
          BLEAdvertisedDevice foundDevice = foundDevices.getDevice(i);

          char deviceAddress[200];
          bzero(deviceAddress, 200);
          foundDevice.getAddress().toString().copy(deviceAddress, 180);
          
          int foundDeviceIndex = -1;
          for(int deviceIndex = 0; deviceIndex < devicesHistoryCount; deviceIndex++) {
             if(devicesHistory[deviceIndex].checkName(deviceAddress)) {
                  foundDeviceIndex = deviceIndex;
                  break;
             }
          }
          
          if(foundDeviceIndex == -1) {
            devicesHistoryCount++;
            Serial.print("DHC: ");
            Serial.print(devicesHistoryCount);
            Serial.println("");
            foundDeviceIndex = devicesHistoryCount - 1;

            strncpy(devicesHistory[foundDeviceIndex].name, deviceAddress, 49);
            devicesHistory[foundDeviceIndex].name[49] = 0;
            
            devicesHistory[foundDeviceIndex].signalLevelsIndex = 0;
            for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
              devicesHistory[foundDeviceIndex].signalLevels[i] = -100;
            }
          }
          
          devicesHistory[foundDeviceIndex].signalLevels[devicesHistory[foundDeviceIndex].signalLevelsIndex] = foundDevice.getRSSI();
          devicesHistory[foundDeviceIndex].signalLevelsIndex++;
          if(devicesHistory[foundDeviceIndex].signalLevelsIndex >= DEVICE_HISTORY_SIZE) {
            devicesHistory[foundDeviceIndex].signalLevelsIndex = 0;
          }

/*
          if(foundDevice.haveName()) {
            sprintf(devicesHistory[foundDeviceIndex].name, "%s (%d) %s", deviceAddress, foundDevice.getRSSI(), foundDevice.getName());
          } else {
            sprintf(devicesHistory[foundDeviceIndex].name, "%s (%d)", deviceAddress, foundDevice.getRSSI());
          }
*/
      }

      for(int foundDeviceIndex = 0; foundDeviceIndex < devicesHistoryCount; foundDeviceIndex++) {
          int y = GRAPH_HEIGHT + (GRAPH_HEIGHT * foundDeviceIndex);
          tft.setCursor(GRAPH_HEIGHT, y);
          tft.println(devicesHistory[foundDeviceIndex].name);
          for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
            int lineHeight = (GRAPH_HEIGHT * ((devicesHistory[foundDeviceIndex].signalLevels[i] + 100.0f)/100.0f));
            int lineX = 240 - DEVICE_HISTORY_SIZE + i;
            if(lineX > 240) {
              lineX = 240;
            }

            int lineYStart = y + (GRAPH_HEIGHT / 4) + 2;
            if(devicesHistory[foundDeviceIndex].signalLevelsIndex == i) {
              lineYStart += 3;
            }
            if(lineYStart > 240) {
              lineYStart = 240;
            }

            int lineYEnd = y - lineHeight + (GRAPH_HEIGHT / 4) + 2;
            if(devicesHistory[foundDeviceIndex].signalLevelsIndex == i) {
              lineYEnd += 3;
            }
            if(lineYEnd > 240) {
              lineYEnd = 240;
            }

            if(devicesHistory[foundDeviceIndex].signalLevelsIndex == i) {
              tft.drawLine(lineX, lineYStart, lineX, lineYEnd, TFT_WHITE);
            } else if(lineHeight >= 11) {
              tft.drawLine(lineX, lineYStart, lineX, lineYEnd, TFT_GREEN);
            } else if(lineHeight >= 7) {
              tft.drawLine(lineX, lineYStart, lineX, lineYEnd, TFT_YELLOW);
            } else if(lineHeight >= 3) {
              tft.drawLine(lineX, lineYStart, lineX, lineYEnd, TFT_ORANGE);
            } else {
              tft.drawLine(lineX, lineYStart, lineX, lineYEnd, TFT_RED);
            }
          }
      }

      pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  }
}
