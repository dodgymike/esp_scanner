#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "button_task.h"
#include "button_state.h"
#include "bluetooth_task.h"
#include "wifi_task.h"
#include "devices_history.h"

#include <BLEDevice.h>

#include <TFT_eSPI.h>

#define BaudRate 115200

#define GRAPH_HEIGHT (20.0f)

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

int deviceOffset;
ButtonState* buttonState = NULL;
DevicesHistory* devicesHistory = NULL;
unsigned long previousTime = 0;
unsigned long previousBlankTime = 0;


#define MODE_SHOW_DEVICES (0)
#define MODE_SHOW_DEVICE (1)


unsigned int mode = MODE_SHOW_DEVICES;

char displayDeviceOffset = 0;

TaskHandle_t xHandle = NULL;
int previousScanMode = -1;

void setup()
{   
  Serial.begin(BaudRate);
  Serial.println("SETUP");

  tft.init();
  tft.setRotation(4);
  tft.fillScreen(TFT_BLACK);

  Serial.println("AFTER TFT INIT");

  Serial.println("BUTTON");
  buttonState = new ButtonState();

  devicesHistory = new DevicesHistory();
  deviceOffset = 0;

  Serial.println("Starting buttonTask");
  xTaskCreate(
    buttonTask,       /* Task function. */
    "buttonTask",     /* String with name of task. */
    2000,             /* Stack size in words. */
    (void*)buttonState,              /* Parameter passed as input of the task */
    2,                 /* Priority of the task. */
    NULL);             /* Task handle. */
}


void loop()
{
  unsigned long currentTime = millis();
  if(currentTime - previousTime < 100) {
    return;
  }

  previousTime = currentTime;

  if(mode == MODE_SHOW_DEVICES) {
    if(buttonState->pressed(ButtonState::up)) {
      deviceOffset--;
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::down)) {
      deviceOffset++;
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::right)) {
      mode = MODE_SHOW_DEVICE;
      
      displayDeviceOffset = deviceOffset;

      devicesHistory->setWifiChannel(devicesHistory->history[displayDeviceOffset].getChannel());
      
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::a)) {
      if(devicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_WIFI) {
        Serial.println("WIFI SCAN MODE");
        devicesHistory->setScanMode(DEVICES_HISTORY_SCAN_MODE_WIFI);
        if(xHandle != NULL) {
          devicesHistory->clean();
          vTaskDelete( xHandle );
          //BLEDevice::deinit(true);
          BLEDevice::deinit(false);
        }

        Serial.println("Starting wifiTask");
        xTaskCreate(
          wifiTask,
          "wifiTask",
          25000,
          (void*)devicesHistory,
          2,
          &xHandle);
      }
    } else if(buttonState->pressed(ButtonState::b)) {
      if(devicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_BTLE) {
        Serial.println("BTLE SCAN MODE");
        devicesHistory->setScanMode(DEVICES_HISTORY_SCAN_MODE_BTLE);

        if(xHandle != NULL) {
          Serial.println("DELETING TASK");
          devicesHistory->clean();
          vTaskDelete( xHandle );
          esp_wifi_stop();

        } else {
        }

        Serial.println("INIT BTLE");
        BLEDevice::init("ABCDEF");

        Serial.println("Starting bluetoothTask");
        xTaskCreate(
          bluetoothTask,
          "bluetoothTask",
          25000,
          (void*)devicesHistory,
          2,
          &xHandle);
      }
    }
  } else if(mode == MODE_SHOW_DEVICE) {
    if(currentTime - previousBlankTime > 1000) {
      //tft.fillScreen(TFT_BLACK);
      previousBlankTime = currentTime;
    }

    if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_DEVICES;
      devicesHistory->setWifiChannel(0);
      
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::up)) {
      if ( xSemaphoreTake(devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
        devicesHistory->history[displayDeviceOffset].switchBuffer();

        devicesHistory->phase++;
        if(devicesHistory->phase >= LOCATION_HISTORY_BUFFERS) {
          devicesHistory->phase = 0;
        }

        tft.fillScreen(TFT_BLACK);

        xSemaphoreGive(devicesHistory->xDevicesSemaphore);
      }
    }
  }

  if(mode == MODE_SHOW_DEVICES) {
    showDevices();
  } else if(mode == MODE_SHOW_DEVICE) {
    showDevice(devicesHistory, displayDeviceOffset);
  }
}

#define PIXELS_PER_DEGREE (360 / DEVICE_HISTORY_SIZE)

int signalDrawBufferCurrent[DEVICE_HISTORY_SIZE];
int signalDrawBufferPrevious[DEVICE_HISTORY_SIZE];
int previousSignalLevelIndex = 0;

void showDevice(DevicesHistory* devicesHistory, int displayDeviceOffset) {
  DeviceHistory* displayDevice = &(devicesHistory->history[displayDeviceOffset]);
  int currentSignalLevelIndex = displayDevice->getSignalLevelsIndex();

  if(currentSignalLevelIndex == previousSignalLevelIndex) {
    return;
  }

  previousSignalLevelIndex = currentSignalLevelIndex;

//  float angle = 3.0 * ((float)currentSignalLevelIndex);
//  float angleToRadians = 6.28318530718 * angle / 360.0;
//  
//  int xA = 120 + ((int)(sin(angleToRadians) * 100)) + 5;
//  int yA = 120 + ((int)(cos(angleToRadians) * 100)) + 5;
//  int xB = 120 - ((int)(sin(angleToRadians) * 100)) - 5;
//  int yB = 120 - ((int)(cos(angleToRadians) * 100)) - 5;
//
//  tft.fillRect(xA, yA, xB, yB, TFT_BLACK);
  

//  Serial.print(currentSignalLevelIndex);

//  if(blankScreen) {
//    tft.fillScreen(TFT_BLACK);
//  }
  
//  for(int bufferIndex = 0; bufferIndex < DEVICE_HISTORY_BUFFERS; bufferIndex++) {
//    drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * bufferIndex), (devicesHistory->history[displayDeviceOffset].getSignalLevelBufferIndex() == bufferIndex), &(devicesHistory->history[displayDeviceOffset]), bufferIndex, devicesHistory->xDevicesSemaphore);
//  }

/*
  drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * bufferIndex), (devicesHistory->history[displayDeviceOffset].getSignalLevelBufferIndex() == bufferIndex), &(devicesHistory->history[displayDeviceOffset]), bufferIndex, devicesHistory->xDevicesSemaphore);
  drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * bufferIndex), (devicesHistory->history[displayDeviceOffset].getSignalLevelBufferIndex() == bufferIndex), &(devicesHistory->history[displayDeviceOffset]), bufferIndex, devicesHistory->xDevicesSemaphore);
  drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * bufferIndex), (devicesHistory->history[displayDeviceOffset].getSignalLevelBufferIndex() == bufferIndex), &(devicesHistory->history[displayDeviceOffset]), bufferIndex, devicesHistory->xDevicesSemaphore);
*/

  displayDevice->copySignalLevels(devicesHistory->locationSignalLevels[devicesHistory->phase]);
  displayDevice->copySignalLevels(signalDrawBufferCurrent);

  int locationBufferIndex = 0;
  drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * locationBufferIndex), (devicesHistory->phase == locationBufferIndex), devicesHistory->locationSignalLevels[locationBufferIndex], devicesHistory->locationSignalLevelsIndex[locationBufferIndex], "1", devicesHistory->xDevicesSemaphore);
  locationBufferIndex++;
  drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * locationBufferIndex), (devicesHistory->phase == locationBufferIndex), devicesHistory->locationSignalLevels[locationBufferIndex], devicesHistory->locationSignalLevelsIndex[locationBufferIndex], "2", devicesHistory->xDevicesSemaphore);
  locationBufferIndex++;
  drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * locationBufferIndex), (devicesHistory->phase == locationBufferIndex), devicesHistory->locationSignalLevels[locationBufferIndex], devicesHistory->locationSignalLevelsIndex[locationBufferIndex], "3", devicesHistory->xDevicesSemaphore);
  
  float accA = 0;
  float accB = 0;
  float accC = 0;

  float countA = 0;
  float countB = 0;
  float countC = 0;
  
  for(int signalLevelIndex = 0; signalLevelIndex < DEVICE_HISTORY_SIZE; signalLevelIndex++) {
    if(devicesHistory->locationSignalLevels[0][signalLevelIndex] != -100) {
      accA += abs(devicesHistory->locationSignalLevels[0][signalLevelIndex]);
      countA++;
    }
    if(devicesHistory->locationSignalLevels[1][signalLevelIndex] != -100) {
      accB += abs(devicesHistory->locationSignalLevels[1][signalLevelIndex]);
      countB++;
    }
    if(devicesHistory->locationSignalLevels[2][signalLevelIndex] != -100) {
      accC += abs(devicesHistory->locationSignalLevels[2][signalLevelIndex]);
      countC++;
    }
  }

  if(countA > 0) {
    accA /= countA;
  }
  if(countB > 0) {
    accB /= countB;
  }
  if(countC > 0) {
    accC /= countC;
  }

  /*
  tft.drawLine(120, 0, 120, 240, TFT_GREEN);
  for(int i = 0; i <= 240; i += 20) {
    tft.drawLine(110, i, 130, i, TFT_GREEN);
  }
  */

  int y = accB - accA;
  int x = accB - accC;

  tft.drawLine(120, 120, 120 + x, 120 + y, TFT_WHITE);
//  tft.fillRect(120, 120, 120 + x, 120 + y, TFT_WHITE);

//  tft.fillRect(120, 120, 120 + x, 120 + y, TFT_WHITE);

//  tft.fillRect(abs(minC), abs(minA), abs(minB), abs(minB), TFT_WHITE);
//  tft.fillRect(120 - 5, 50 + abs(minA), 10, abs(minB), TFT_WHITE);

//  tft.fillRect(110, 50 + abs(minA), 130, 150 - abs(minB), TFT_WHITE);
//  tft.fillRect(110, 50, 130, 60, TFT_WHITE);

  
  int fromX = 0;
  int fromY = 0;
  int fromXPrevious = 0;
  int fromYPrevious = 0;

  for(int guideRadius = 10; guideRadius <= 100; guideRadius += 20) {
    tft.drawCircle(120, 120, guideRadius, TFT_DARKGREY);
  }

  for(int signalLevelIndex = 0.0; signalLevelIndex < DEVICE_HISTORY_SIZE; signalLevelIndex++) {
    if(signalDrawBufferCurrent[signalLevelIndex] == -100) {
      continue;
    }
    
    float angle = 3.0 * ((float)signalLevelIndex);
    float angleToRadians = 6.28318530718 * angle / 360.0;
    
    int x = 120 + ((int)(sin(angleToRadians) * signalDrawBufferCurrent[signalLevelIndex]));
    int y = 120 + ((int)(cos(angleToRadians) * signalDrawBufferCurrent[signalLevelIndex]));
    int xPrevious = 120 + ((int)(sin(angleToRadians) * signalDrawBufferPrevious[signalLevelIndex]));
    int yPrevious = 120 + ((int)(cos(angleToRadians) * signalDrawBufferPrevious[signalLevelIndex]));

    tft.drawLine(xPrevious, yPrevious, fromXPrevious, fromYPrevious, TFT_BLACK);

    if((signalLevelIndex >= 4) && (abs(signalLevelIndex - currentSignalLevelIndex) <= 4)) {
//      tft.drawLine(xPrevious, yPrevious, fromX, fromY, TFT_BLACK);
    }
    
    if((signalLevelIndex >= 3) && (abs(signalLevelIndex - currentSignalLevelIndex) <= 3)) {
//      tft.drawLine(xPrevious, yPrevious, fromX, fromY, TFT_BLACK);
      tft.drawLine(x, y, fromX, fromY, TFT_WHITE);
    } else if(signalLevelIndex != 0) {
//      tft.drawLine(xPrevious, yPrevious, fromX, fromY, TFT_BLACK);
      tft.drawLine(x, y, fromX, fromY, TFT_ORANGE);
    }

    fromX = x;
    fromY = y;
    fromXPrevious = xPrevious;
    fromYPrevious = yPrevious;
  }
  
  // overwrite the previous history with the current
  for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
    signalDrawBufferPrevious[i] = signalDrawBufferCurrent[i];
  }
}

void drawDeviceHistory(int y, bool isSelectedDevice, DeviceHistory* deviceHistory, SemaphoreHandle_t xSemaphore) {
   drawDeviceHistory(y, isSelectedDevice, deviceHistory->getSignalLevels(0), deviceHistory->getSignalLevelBufferIndex(), deviceHistory->name, xSemaphore);
}

//void drawDeviceHistory(int y, bool isSelectedDevice, DeviceHistory* deviceHistory, int bufferIndex, SemaphoreHandle_t xSemaphore) {
void drawDeviceHistory(int y, bool isSelectedDevice, int signalLevels[], int signalLevelsIndex, char* deviceName, SemaphoreHandle_t xSemaphore) {
  if ( xSemaphoreTake(xSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
    tft.setCursor(GRAPH_HEIGHT, y);
    tft.println(deviceName);
    
    if(isSelectedDevice) {
      tft.fillCircle(GRAPH_HEIGHT / 2, y, 3, TFT_WHITE);
    } else {
      tft.fillCircle(GRAPH_HEIGHT / 2, y, 3, TFT_BLACK);
    }
    
    for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
      int lineHeight = (GRAPH_HEIGHT * ((signalLevels[i] + 100.0f)/100.0f));
      //int lineHeight = (GRAPH_HEIGHT * ((deviceHistory->getSignalLevels(bufferIndex)[i] + 100.0f)/100.0f));
      
      int lineX = 240 - DEVICE_HISTORY_SIZE + i;
      if(lineX > 240) {
        lineX = 240;
      }
      
      tft.drawLine(lineX, y, lineX, y + GRAPH_HEIGHT, TFT_BLACK);

      //int signalLevelsIndex = deviceHistory->getSignalLevelsIndex(bufferIndex);
      
      int lineYStart = y + (GRAPH_HEIGHT / 4) + 2;          
      if(signalLevelsIndex == i) {
        lineYStart += 3;
      } else {
        tft.drawLine(lineX, lineYStart + 1, lineX, lineYStart + 3, TFT_BLACK);
      }
      if(lineYStart > 240) {
        lineYStart = 240;
      }
      
      int lineYEnd = y - lineHeight + (GRAPH_HEIGHT / 4) + 2;
      if(signalLevelsIndex == i) {
        lineYEnd += 3;
      }
      if(lineYEnd > 240) {
        lineYEnd = 240;
      }
      
      if(signalLevelsIndex == i) {
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
    xSemaphoreGive(xSemaphore);
  }  
}

void showDevices() {
  if(deviceOffset < 0) {
    deviceOffset = devicesHistory->getCount() - 1;
  } else if(deviceOffset >= devicesHistory->getCount()) {
    deviceOffset = 0;
  }
  
  int displayDeviceStartIndex = 0;
  int displayDeviceEndIndex = devicesHistory->getCount();
  if(displayDeviceEndIndex > 11) {
    if(deviceOffset >= 5) {
      displayDeviceStartIndex = deviceOffset - 5;
      displayDeviceEndIndex = deviceOffset + 5 + 1;
    } else {
      displayDeviceStartIndex = 0;
      displayDeviceEndIndex = 11;
    }
  }

  if(displayDeviceEndIndex > devicesHistory->getCount()) {
    displayDeviceEndIndex = devicesHistory->getCount();
  }
  
  for(int foundDeviceIndex = displayDeviceStartIndex; foundDeviceIndex < displayDeviceEndIndex; foundDeviceIndex++) {
    int y = GRAPH_HEIGHT + (GRAPH_HEIGHT * (foundDeviceIndex - displayDeviceStartIndex));

    drawDeviceHistory(y, (deviceOffset == foundDeviceIndex), &(devicesHistory->history[foundDeviceIndex]), devicesHistory->xDevicesSemaphore);
  }
}
