#include <string.h>
#include <stdlib.h>
#include <math.h>


#include <math.h>

#include "button_task.h"
#include "button_state.h"
#include "bluetooth_task.h"
#include "devices_history.h"

#include <TFT_eSPI.h>

#define BaudRate 115200

#define GRAPH_HEIGHT (20.0f)

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

int deviceOffset;
ButtonState* buttonState = NULL;
DevicesHistory* devicesHistory = NULL;
unsigned long previousTime = 0;


#define MODE_SHOW_DEVICES (0)
#define MODE_SHOW_DEVICE (1)

unsigned int mode = MODE_SHOW_DEVICES;

char displayDeviceOffset = 0;

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
    25000,             /* Stack size in words. */
    (void*)buttonState,              /* Parameter passed as input of the task */
    2,                 /* Priority of the task. */
    NULL);             /* Task handle. */

  Serial.println("Starting bluetoothTask");
  xTaskCreate(
    bluetoothTask,       /* Task function. */
    "bluetoothTask",     /* String with name of task. */
    25000,             /* Stack size in words. */
    (void*)devicesHistory,              /* Parameter passed as input of the task */
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
      
      tft.fillScreen(TFT_BLACK);
    }
  } else if(mode == MODE_SHOW_DEVICE) {
    if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_DEVICES;
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::right)) {
      if ( xSemaphoreTake(devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
        devicesHistory->history[displayDeviceOffset].switchBuffer();
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

void showDevice(DevicesHistory* devicesHistory, int displayDeviceOffset) {
  for(int bufferIndex = 0; bufferIndex < DEVICE_HISTORY_BUFFERS; bufferIndex++) {
    drawDeviceHistory(GRAPH_HEIGHT + (GRAPH_HEIGHT * bufferIndex), (devicesHistory->history[displayDeviceOffset].getSignalLevelBufferIndex() == bufferIndex), &(devicesHistory->history[displayDeviceOffset]), bufferIndex, devicesHistory->xDevicesSemaphore);
  }

  
}

void drawDeviceHistory(int y, bool isSelectedDevice, DeviceHistory* deviceHistory, SemaphoreHandle_t xSemaphore) {
   drawDeviceHistory(y, isSelectedDevice, deviceHistory, deviceHistory->getSignalLevelBufferIndex(), xSemaphore);
}

void drawDeviceHistory(int y, bool isSelectedDevice, DeviceHistory* deviceHistory, int bufferIndex, SemaphoreHandle_t xSemaphore) {
  if ( xSemaphoreTake(xSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
    tft.setCursor(GRAPH_HEIGHT, y);
    tft.println(deviceHistory->name);
    
    if(isSelectedDevice) {
      tft.fillCircle(GRAPH_HEIGHT / 2, y, 3, TFT_WHITE);
    } else {
      tft.fillCircle(GRAPH_HEIGHT / 2, y, 3, TFT_BLACK);
    }
    
    for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
      int lineHeight = (GRAPH_HEIGHT * ((deviceHistory->getSignalLevels(bufferIndex)[i] + 100.0f)/100.0f));
      int lineX = 240 - DEVICE_HISTORY_SIZE + i;
      if(lineX > 240) {
        lineX = 240;
      }
      
      tft.drawLine(lineX, y, lineX, y + GRAPH_HEIGHT, TFT_BLACK);

      int signalLevelsIndex = deviceHistory->getSignalLevelsIndex(bufferIndex);
      
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
