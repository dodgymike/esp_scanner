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

#include <BitBang_I2C.h>

#define BaudRate 115200

#define GRAPH_HEIGHT (20.0f)

/*
lis3mdl
LSM6DS33
*/

#define LIS3MDL_SA1_HIGH_ADDRESS  0b0011110
#define LIS3MDL_SA1_LOW_ADDRESS   0b0011100

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

ButtonState* buttonState = NULL;

#define SDA_PIN 5
#define SCL_PIN 17

BBI2C bbi2c;

uint8_t WHO_AM_I    = 0x0F;

uint8_t CTRL_REG1   = 0x20;
uint8_t CTRL_REG2   = 0x21;
uint8_t CTRL_REG3   = 0x22;
uint8_t CTRL_REG4   = 0x23;
uint8_t CTRL_REG5   = 0x24;

uint8_t STATUS_REG  = 0x27;
uint8_t OUT_X_L     = 0x28;
uint8_t OUT_X_H     = 0x29;
uint8_t OUT_Y_L     = 0x2A;
uint8_t OUT_Y_H     = 0x2B;
uint8_t OUT_Z_L     = 0x2C;
uint8_t OUT_Z_H     = 0x2D;
uint8_t TEMP_OUT_L  = 0x2E;
uint8_t TEMP_OUT_H  = 0x2F;
uint8_t INT_CFG     = 0x30;
uint8_t INT_SRC     = 0x31;
uint8_t INT_THS_L   = 0x32;
uint8_t INT_THS_H   = 0x33;

int deviceOffset = 0;
int apDeviceOffset = 0;
int probeDeviceOffset = 0;

struct WifiTaskParameter* histories;

unsigned long previousTime = 0;
unsigned long previousBlankTime = 0;

#define MODE_SHOW_DEVICES       (0)
#define MODE_SHOW_DEVICE        (1)
#define MODE_SHOW_AP_DEVICES    (2)
#define MODE_SHOW_AP_DEVICE     (3)
#define MODE_SHOW_PROBE_DEVICES (4)
#define MODE_SHOW_PROBE_DEVICE  (5)

unsigned int mode = MODE_SHOW_DEVICES;

int displayDeviceOffset = 0;
int apDisplayDeviceOffset = 0;
int probeDisplayDeviceOffset = 0;

TaskHandle_t xHandle = NULL;
int previousScanMode = -1;

void showDevices(DevicesHistory* devicesHistory);
void showDevice(DevicesHistory* devicesHistory, int displayDeviceOffset);

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

  histories = new WifiTaskParameter();
  histories->apDevicesHistory = new DevicesHistory();
  histories->devicesHistory = new DevicesHistory();
  histories->probeDevicesHistory = new DevicesHistory();

  deviceOffset = 0;

  Serial.println("Starting buttonTask");
  xTaskCreate(
    buttonTask,       /* Task function. */
    "buttonTask",     /* String with name of task. */
    2000,             /* Stack size in words. */
    (void*)buttonState,              /* Parameter passed as input of the task */
    2,                 /* Priority of the task. */
    NULL);             /* Task handle. */

  memset(&bbi2c, 0, sizeof(bbi2c));
  bbi2c.bWire = 0; // use bit bang, not wire library
  bbi2c.iSDA = SDA_PIN;
  bbi2c.iSCL = SCL_PIN;

  I2CInit(&bbi2c, 100000L);
  delay(100); // allow devices to power up

  Serial.println("==============================================");
  Serial.println(I2CDiscoverDevice(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS) == 17);
  delay(5000);

  uint8_t whoAmIBuffer[1];
  bzero(whoAmIBuffer, 1);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, WHO_AM_I, whoAmIBuffer, 1);
  Serial.println("==============================================");
  Serial.println(whoAmIBuffer[0] == 0x3D);
  Serial.println(whoAmIBuffer[0]);
  delay(500);

  uint8_t initBuffer1[2] = {
    // 0x70 = 0b01110000
    // OM = 11 (ultra-high-performance mode for X and Y); DO = 100 (10 Hz ODR)
    CTRL_REG1,
    0x70
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer1, 2);
  bzero(initBuffer1, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG1, &(initBuffer1[0]), 1);
  Serial.println("==============================================");
  Serial.println(initBuffer1[0] == 0x70);
  delay(500);


  uint8_t initBuffer2[2] = {
    // 0x00 = 0b00000000
    // FS = 00 (+/- 4 gauss full scale)
    CTRL_REG2,
    0x00,
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer2, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG2, &(initBuffer2[0]), 1);
  Serial.println("==============================================");
  Serial.println(initBuffer2[0] == 0x00);
  delay(500);

  uint8_t initBuffer3[2] = {
    // 0x00 = 0b00000000
    // MD = 00 (continuous-conversion mode)
    CTRL_REG3,
    0x00
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer3, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG3, &(initBuffer3[0]), 1);
  Serial.println("==============================================");
  Serial.println(initBuffer3[0] == 0x00);
  delay(500);

  uint8_t initBuffer4[2] = {
    // 0x0C = 0b00001100
    // OMZ = 11 (ultra-high-performance mode for Z)
    CTRL_REG4,
    0x0C,
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer4, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG4, &(initBuffer4[0]), 1);
  Serial.println("==============================================");
  Serial.println(initBuffer4[0] == 0x0C);
  delay(500);
}

void loop()
{
  unsigned long currentTime = millis();
  if(currentTime - previousTime < 100) {
    return;
  }

  previousTime = currentTime;

  tft.setCursor(200, 0);
  if(histories->devicesHistory->getScanMode() == DEVICES_HISTORY_SCAN_MODE_WIFI) {
    tft.println("Wifi");    
  } else if(histories->devicesHistory->getScanMode() == DEVICES_HISTORY_SCAN_MODE_BTLE) {
    tft.println("BTLE");
  } else {
    tft.println("None");
  }

  if(mode == MODE_SHOW_DEVICES) {
    tft.setCursor(0, 0);
    tft.println("AP List");
    
    if(buttonState->pressed(ButtonState::up)) {
      if(histories->devicesHistory->getCount() == 0) {
        deviceOffset = 0;
      } else {
        deviceOffset--;
      }
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::down)) {
      if(histories->devicesHistory->getCount() == 0) {
        deviceOffset = 0;
      } else {
        deviceOffset++;
      }
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::right)) {
      mode = MODE_SHOW_DEVICE;
      
      displayDeviceOffset = deviceOffset;

      histories->devicesHistory->setWifiChannel(histories->devicesHistory->history[deviceOffset].getChannel());
      
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_AP_DEVICES;

      displayDeviceOffset = deviceOffset;

      uint8_t apAddress[6];
      histories->devicesHistory->history[deviceOffset].getAddress(apAddress);
      histories->devicesHistory->setApAddress(apAddress);
      histories->devicesHistory->setWifiChannel(histories->devicesHistory->history[deviceOffset].getChannel());

      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::leftS)) {
        Serial.println("PROBE LIST");
      mode = MODE_SHOW_PROBE_DEVICES;

      histories->devicesHistory->setWifiChannel(1);

      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::a)) {
      if(histories->devicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_WIFI) {
        Serial.println("WIFI SCAN MODE");
        histories->devicesHistory->setScanMode(DEVICES_HISTORY_SCAN_MODE_WIFI);
        if(xHandle != NULL) {
          histories->devicesHistory->clean();
          vTaskDelete( xHandle );
          //BLEDevice::deinit(true);
          BLEDevice::deinit(false);
        }

        Serial.println("Starting wifiTask");
        xTaskCreate(
          wifiTask,
          "wifiTask",
          30000,
          (void*)histories,
          2,
          &xHandle);
      }
    } else if(buttonState->pressed(ButtonState::b)) {
      if(histories->devicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_BTLE) {
        Serial.println("BTLE SCAN MODE");
        histories->devicesHistory->setScanMode(DEVICES_HISTORY_SCAN_MODE_BTLE);

        if(xHandle != NULL) {
          Serial.println("DELETING TASK");
          histories->devicesHistory->clean();
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
          (void*)histories->devicesHistory,
          2,
          &xHandle);
      }
    }
  } else if(mode == MODE_SHOW_DEVICE) {
    tft.setCursor(0, 0);
    tft.println("AP");
    
    if(currentTime - previousBlankTime > 1000) {
      //tft.fillScreen(TFT_BLACK);
      previousBlankTime = currentTime;
    }

    if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_DEVICES;
      histories->devicesHistory->setWifiChannel(0);
      
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::up)) {
      if ( xSemaphoreTake(histories->devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
        histories->devicesHistory->history[displayDeviceOffset].switchBuffer();

        histories->devicesHistory->phase++;
        if(histories->devicesHistory->phase >= LOCATION_HISTORY_BUFFERS) {
          histories->devicesHistory->phase = 0;
        }

        tft.fillScreen(TFT_BLACK);

        xSemaphoreGive(histories->devicesHistory->xDevicesSemaphore);
      }
    }
  } else if(mode == MODE_SHOW_AP_DEVICES) {
    tft.setCursor(0, 0);
    tft.println("AP Devices");

    if(currentTime - previousBlankTime > 1000) {
      //tft.fillScreen(TFT_BLACK);
      previousBlankTime = currentTime;
    }

    if(buttonState->pressed(ButtonState::up)) {
      if(histories->apDevicesHistory->getCount() == 0) {
        apDeviceOffset = 0;
      } else {
        apDeviceOffset--;
      }
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::down)) {
      if(histories->apDevicesHistory->getCount() == 0) {
        apDeviceOffset = 0;
      } else {
        apDeviceOffset++;
      }
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::right)) {
      mode = MODE_SHOW_AP_DEVICE;
      
      apDisplayDeviceOffset = apDeviceOffset;

      int apChannel = histories->apDevicesHistory->history[apDeviceOffset].getChannel();
      histories->apDevicesHistory->setWifiChannel(apChannel);
      
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_DEVICES;

      uint8_t apAddress[6];
      bzero(apAddress, 6);
      histories->devicesHistory->setApAddress(apAddress);
      histories->apDevicesHistory->clean();

      tft.fillScreen(TFT_BLACK);
    }
  } else if(mode == MODE_SHOW_AP_DEVICE) {
    tft.setCursor(0, 0);
    tft.println("AP Device");

    if(currentTime - previousBlankTime > 1000) {
      //tft.fillScreen(TFT_BLACK);
      previousBlankTime = currentTime;
    }

    if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_AP_DEVICES;
//      histories->apDevicesHistory->setWifiChannel(0);
      int apChannel = histories->apDevicesHistory->history[apDeviceOffset].getChannel();
      histories->apDevicesHistory->setWifiChannel(apChannel);
      
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::up)) {
      if ( xSemaphoreTake(histories->apDevicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
        histories->apDevicesHistory->history[displayDeviceOffset].switchBuffer();

        histories->apDevicesHistory->phase++;
        if(histories->apDevicesHistory->phase >= LOCATION_HISTORY_BUFFERS) {
          histories->apDevicesHistory->phase = 0;
        }

        tft.fillScreen(TFT_BLACK);

        xSemaphoreGive(histories->apDevicesHistory->xDevicesSemaphore);
      }
    }
  } else if(mode == MODE_SHOW_PROBE_DEVICES) {
    tft.setCursor(0, 0);
    tft.println("Probe List");
    
    if(currentTime - previousBlankTime > 1000) {
      //tft.fillScreen(TFT_BLACK);
      previousBlankTime = currentTime;
    }

    if(buttonState->pressed(ButtonState::up)) {
      probeDeviceOffset--;
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::down)) {
      probeDeviceOffset++;
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::right)) {
      mode = MODE_SHOW_PROBE_DEVICE;
      
      probeDisplayDeviceOffset = probeDeviceOffset;
//
//      histories->devicesHistory->setWifiChannel(histories->devicesHistory->history[apDeviceOffset].getChannel());
      
      tft.fillScreen(TFT_BLACK);
    } else if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_DEVICES;

      histories->probeDevicesHistory->clean();

      tft.fillScreen(TFT_BLACK);
    }
  } else if(mode == MODE_SHOW_PROBE_DEVICE) {
    tft.setCursor(0, 0);
    tft.println("Probe Device");

    if(currentTime - previousBlankTime > 1000) {
      //tft.fillScreen(TFT_BLACK);
      previousBlankTime = currentTime;
    }

    if(buttonState->pressed(ButtonState::left)) {
      mode = MODE_SHOW_PROBE_DEVICES;
//      histories->apDevicesHistory->setWifiChannel(0);
//      int apChannel = histories->apDevicesHistory->history[apDeviceOffset].getChannel();
//      histories->apDevicesHistory->setWifiChannel(apChannel);
      
      tft.fillScreen(TFT_BLACK);
    }
  }

  if(mode == MODE_SHOW_DEVICES) {
    deviceOffset = showDevices(histories->devicesHistory, deviceOffset);
  } else if(mode == MODE_SHOW_DEVICE) {
    showDevice(histories->devicesHistory, displayDeviceOffset);
  } else if(mode == MODE_SHOW_AP_DEVICES) {
    apDeviceOffset = showDevices(histories->apDevicesHistory, apDeviceOffset);
  } else if(mode == MODE_SHOW_AP_DEVICE) {
    showDevice(histories->apDevicesHistory, apDisplayDeviceOffset);
  } else if(mode == MODE_SHOW_PROBE_DEVICES) {
    probeDeviceOffset = showDevices(histories->probeDevicesHistory, probeDeviceOffset);
  } else if(mode == MODE_SHOW_PROBE_DEVICE) {
    showDevice(histories->probeDevicesHistory, probeDeviceOffset);
  }

/*
  unsigned char ucMap[16];

  I2CScan(&bbi2c, ucMap);
  I2CRead(uint8_t u8Address, uint8_t *pu8Data, int iLength);
  I2CReadRegister(uint8_t iAddr, uint8_t u8Register, uint8_t *pData, int iLen);
  I2CWrite(uint8_t iAddr, uint8_t *pData, int iLen); 
*/

  
//  uint8_t initBuffer[2] = {
//    OUT_X_L | 0x80,
////    OUT_X_L | 0x06,
//  };
//  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer, 1);

  uint16_t valueL;
  uint16_t valueH;
  
  uint8_t magXYZBytes[6];
  bzero(magXYZBytes, 6);
  
//  I2CRead(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, magXYZBytes, 6);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_X_L, &(magXYZBytes[0]), 1);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_X_H, &(magXYZBytes[1]), 1);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Y_L, &(magXYZBytes[2]), 1);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Y_H, &(magXYZBytes[3]), 1);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Z_L, &(magXYZBytes[4]), 1);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Z_H, &(magXYZBytes[5]), 1);

  for(int i = 0; i < 3; i++) {
    uint16_t valueL = magXYZBytes[(i * 2) + 0];
    uint16_t valueH = magXYZBytes[(i * 2) + 1];
    Serial.print((valueH << 8) + valueL);
    Serial.print(":");
  }
  Serial.println("");

/*
Device found at 0x1E, type = Unknown
Device found at 0x5D, type = LPS25H
Device found at 0x6B, type = LSM6DS3
*/
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

int showDevices(DevicesHistory* devicesHistory, int offset) {
  if(offset < 0) {
    offset = devicesHistory->getCount() - 1;
  } else if(offset >= devicesHistory->getCount()) {
    offset = 0;
  }

  if(devicesHistory->getCount() == 0) {
    tft.setCursor(80, 100);
    tft.println("No Devices Found");
    return offset;
  }

  int displayDeviceStartIndex = 0;
  int displayDeviceEndIndex = devicesHistory->getCount();

  
  if(displayDeviceEndIndex > 11) {
    if(offset >= 5) {
      displayDeviceStartIndex = offset - 5;
      displayDeviceEndIndex = offset + 5 + 1;
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

    drawDeviceHistory(y, (offset == foundDeviceIndex), &(devicesHistory->history[foundDeviceIndex]), devicesHistory->xDevicesSemaphore);
  }

  return offset;
}
