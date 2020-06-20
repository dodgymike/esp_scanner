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

#define DEVICE_ADDRESS_SIZE (50)
#define DEVICE_HISTORY_SIZE (80)


class ButtonState {
  public:
    bool up;
    bool down;
    bool left;
    bool right;
  
    bool leftSelect;
  
    SemaphoreHandle_t xButtonSemaphore;

    bool upPressed() {
      bool rv = false;

      if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
        if(up) {
//         upA = 200;
          
          rv = true;
          up = false;
        }

        xSemaphoreGive(xButtonSemaphore);
      }

      return rv;
    }

    bool downPressed() {
      bool rv = false;

      if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
        if(down) {
//          downA = 200;
          
          rv = true;
          down = false;
        }

        xSemaphoreGive(xButtonSemaphore);
      }

      return rv;
    }

  ButtonState()
    : up(false), down(false), left(false), right(false), xButtonSemaphore(xSemaphoreCreateMutex())
    {
      xSemaphoreGive(xButtonSemaphore);
    }
};

class DeviceHistory {
//  private:
  public:
    char name[DEVICE_ADDRESS_SIZE];
    int signalLevels[DEVICE_HISTORY_SIZE];
    int signalLevelsIndex;

    DeviceHistory()
      : name(""), signalLevelsIndex(0)
    {}

    bool checkName(const char* nameToCheck) {
      return (strncmp(name, nameToCheck, DEVICE_ADDRESS_SIZE) == 0);
    }
};

class DevicesHistory {
  private:
    int count;
    SemaphoreHandle_t xCountSemaphore;
      
  public:
    int getCount() {
      int rv = 0;
      
      if ( xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
        rv = count;
        xSemaphoreGive(xCountSemaphore);
      }      

      return rv;
    }

    void incrementCount() {
      if ( xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
        count++;
        xSemaphoreGive(xCountSemaphore);
      }      
    }

    DeviceHistory history[100];
    
    SemaphoreHandle_t xDevicesSemaphore;

    DevicesHistory()
      : count(0), xDevicesSemaphore(xSemaphoreCreateMutex()),  xCountSemaphore(xSemaphoreCreateMutex())
    {
      xSemaphoreGive(xDevicesSemaphore);
      xSemaphoreGive(xCountSemaphore);
    }
};

void bluetoothTask(void* parameter) {
  DevicesHistory* devicesHistory = (DevicesHistory*)parameter;
  
  int scanTime = 1; //In seconds
  int scanIndex = 0;
  
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  while(true) {
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

    if ( xSemaphoreTake( devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
      for(int i = 0; i < foundDevices.getCount(); i++) {
          BLEAdvertisedDevice foundDevice = foundDevices.getDevice(i);
      
          char deviceAddress[200];
          bzero(deviceAddress, 200);
          foundDevice.getAddress().toString().copy(deviceAddress, 180);
          
          int foundDeviceIndex = -1;
          for(int deviceIndex = 0; deviceIndex < devicesHistory->getCount(); deviceIndex++) {
             if(devicesHistory->history[deviceIndex].checkName(deviceAddress)) {
                  foundDeviceIndex = deviceIndex;
                  break;
             }
          }
          
          if(foundDeviceIndex == -1) {
            devicesHistory->incrementCount();
            foundDeviceIndex = devicesHistory->getCount() - 1;
      
            strncpy(devicesHistory->history[foundDeviceIndex].name, deviceAddress, (DEVICE_ADDRESS_SIZE - 1));
            devicesHistory->history[foundDeviceIndex].name[(DEVICE_ADDRESS_SIZE - 1)] = 0;
            
            devicesHistory->history[foundDeviceIndex].signalLevelsIndex = 0;
            for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
              devicesHistory->history[foundDeviceIndex].signalLevels[i] = -100;
            }
          }
          
          devicesHistory->history[foundDeviceIndex].signalLevels[devicesHistory->history[foundDeviceIndex].signalLevelsIndex] = foundDevice.getRSSI();
          devicesHistory->history[foundDeviceIndex].signalLevelsIndex++;
          if(devicesHistory->history[foundDeviceIndex].signalLevelsIndex >= DEVICE_HISTORY_SIZE) {
            devicesHistory->history[foundDeviceIndex].signalLevelsIndex = 0;
          }
      }
      
      xSemaphoreGive( devicesHistory->xDevicesSemaphore );
    }
         
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    delay(100);
  }
}

#define BUTTON_GPIO_UP    (14)
#define BUTTON_GPIO_DOWN  (33)

void buttonTask(void* parameter) {
  ButtonState* buttonState = (ButtonState*)parameter;

//  digitalWrite(2, LOW);  // B
//  digitalWrite(4, LOW);
//  digitalWrite(12, LOW); // LEFT SELECT
//  digitalWrite(13, LOW); // RIGHT SELECT (NOT WORKING)
  digitalWrite(BUTTON_GPIO_UP, LOW); // UP
//  digitalWrite(15, LOW); // A
//  digitalWrite(27, LOW); // RIGHT
//  digitalWrite(32, LOW); // LEFT
  digitalWrite(BUTTON_GPIO_DOWN, LOW); // DOWN

//  pinMode(2, INPUT);  // B
//  pinMode(4, INPUT);
//  pinMode(12, INPUT); // LEFT SELECT
//  pinMode(13, INPUT);
  pinMode(BUTTON_GPIO_UP, INPUT); // UP
//  pinMode(15, INPUT); // A
//  pinMode(27, INPUT); // RIGHT
//  pinMode(32, INPUT);
  pinMode(BUTTON_GPIO_DOWN, INPUT); // DOWN

//  pinMode(19, OUTPUT);
//  pinMode(22, OUTPUT);
//  
//  digitalWrite(19, HIGH);
//  digitalWrite(22, HIGH);

  unsigned long startMillis = millis();
  int upA   = 0;
  int upC   = 0;
  int downA = 0;
  int downC = 0;
  
  while(millis() - startMillis < 1500) {
    int cUp         = readAnalogSensorRaw(BUTTON_GPIO_UP);   // UP
    int cDown       = readAnalogSensorRaw(BUTTON_GPIO_DOWN); // DOWN  

    upA += cUp;
    upC++;
    
    downA += cDown;
    downC++;

    delay(50);
  }

  int upInitial   = (upA / upC) - 4; //(upA / upC) / 2; //-10
  int downInitial = (downA / downC) - 4; //(downA / downC) / 2; //-10

  while(true) {
//    int b1          = readAnalogSensorRaw(2);  // B
//    int b2          = readAnalogSensorRaw(4);
//    int cLeftSelect = readAnalogSensorRaw(12); // LEFT SELECT
//    int b4          = readAnalogSensorRaw(13);
    int cUp         = readAnalogSensorRaw(BUTTON_GPIO_UP);   // UP
//    int b6          = readAnalogSensorRaw(15); // A
//    int cRight      = readAnalogSensorRaw(27); // RIGHT
//    int cLeft       = readAnalogSensorRaw(32); // LEFT
    int cDown       = readAnalogSensorRaw(BUTTON_GPIO_DOWN); // DOWN  


    if ( xSemaphoreTake( buttonState->xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
//      buttonState->upA         += cUp;
//      buttonState->downA       += cDown;
//      buttonState->leftA       += cLeft;
//      buttonState->rightA      += cRight;
//      buttonState->leftSelectA += cLeftSelect;
      
//      buttonState->upCounter         += 1;
//      buttonState->downCounter       += 1;
//      buttonState->leftCounter       += 1;
//      buttonState->rightCounter      += 1;
//      buttonState->leftSelectCounter += 1;

//      int upSelector         = buttonState->upA         / buttonState->upCounter;
//      int downSelector       = buttonState->downA       / buttonState->downCounter;
//      int leftSelector       = buttonState->leftA       / buttonState->leftCounter;
//      int rightSelector      = buttonState->rightA      / buttonState->rightCounter;
//      int leftSelectSelector = buttonState->leftSelectA / buttonState->leftSelectCounter;

//      int upSelector         = buttonState->upA         / 5;
//      int downSelector       = buttonState->downA       / 5;
//      int leftSelector       = buttonState->leftA       / 5;
//      int rightSelector      = buttonState->rightA      / 5;
//      int leftSelectSelector = buttonState->leftSelectA / 5;

/*
      if(cUp < 10) {
        buttonState->upCounter++;
      } else {
        buttonState->upCounter /= 2;
      }
      if(buttonState->upCounter >= 10) {
        buttonState->upCounter = 10;
      }
      
      if(cDown < 10) {
        buttonState->downCounter++;
      } else {
        buttonState->downCounter /= 2;
      }
      if(buttonState->downCounter >= 10) {
        buttonState->downCounter = 10;
      }

      buttonState->up = (buttonState->upCounter >= 2);
      buttonState->down = (buttonState->downCounter >= 2);
*/

      buttonState->up = (cUp < upInitial);
      buttonState->down = (cDown < downInitial);

/*
      Serial.println("===================");
//      Serial.println(buttonState->upA);
//      Serial.println(buttonState->upCounter);
      Serial.println(upSelector);
      Serial.println(buttonState->up);  
      Serial.println(cUp);  
      Serial.println("===");
      Serial.println(downSelector);
      Serial.println(buttonState->down);  
      Serial.println(cDown);  
*/

/*
      buttonState->upA         *= 2.0f;
      buttonState->downA       *= 2.0f;
      buttonState->leftA       *= 2.0f;
      buttonState->rightA      *= 2.0f;
      buttonState->leftSelectA *= 2.0f;
*/

      xSemaphoreGive( buttonState->xButtonSemaphore );
    }

    delay(50);
  }
}

int deviceOffset;
ButtonState* buttonState = NULL;
DevicesHistory* devicesHistory = NULL;

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
/*
*/  
  xTaskCreate(
    buttonTask,       /* Task function. */
    "buttonTask",     /* String with name of task. */
    25000,             /* Stack size in words. */
    (void*)buttonState,              /* Parameter passed as input of the task */
    2,                 /* Priority of the task. */
    NULL);             /* Task handle. */

  xTaskCreate(
    bluetoothTask,       /* Task function. */
    "bluetoothTask",     /* String with name of task. */
    25000,             /* Stack size in words. */
    (void*)devicesHistory,              /* Parameter passed as input of the task */
    2,                 /* Priority of the task. */
    NULL);             /* Task handle. */
}

#define minimum(a,b) (((a) < (b)) ? (a) : (b))

static int TOUCH_SENSE = 10;

int inputVal = 0;
bool readAnalogSensor(int pin)
{
  inputVal = touchRead(pin);

/*
  Serial.print("Touch value is Pin");
  Serial.print(pin);
  Serial.print(" = ");
  Serial.println( inputVal);
*/

  bool buttonPressed = inputVal < TOUCH_SENSE;

  return buttonPressed;
}

int readAnalogSensorRaw(int pin) {
  inputVal = touchRead(pin);

/*
  Serial.print("Touch value is Pin");
  Serial.print(pin);
  Serial.print(" = ");
  Serial.println( inputVal);
*/

  return inputVal;
}

unsigned long previousTime = 0;

void loop()
{
  unsigned long currentTime = millis();
  if(currentTime - previousTime < 100) {
    return;
  }

  previousTime = currentTime;
  
  if(buttonState->upPressed()) {
    deviceOffset--;
    tft.fillScreen(TFT_BLACK);
  } else if(buttonState->downPressed()) {
    deviceOffset++;
    tft.fillScreen(TFT_BLACK);
  }
  
  //tft.fillScreen(TFT_BLACK);

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
    if ( xSemaphoreTake( devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
      int y = GRAPH_HEIGHT + (GRAPH_HEIGHT * (foundDeviceIndex - displayDeviceStartIndex));
      tft.setCursor(GRAPH_HEIGHT, y);
      tft.println(devicesHistory->history[foundDeviceIndex].name);
      
      if(deviceOffset == foundDeviceIndex) {
        tft.fillCircle(GRAPH_HEIGHT / 2, y, 3, TFT_WHITE);
      } else {
        tft.fillCircle(GRAPH_HEIGHT / 2, y, 3, TFT_BLACK);
      }
      
      for(int i = 0; i < DEVICE_HISTORY_SIZE; i++) {
        int lineHeight = (GRAPH_HEIGHT * ((devicesHistory->history[foundDeviceIndex].signalLevels[i] + 100.0f)/100.0f));
        int lineX = 240 - DEVICE_HISTORY_SIZE + i;
        if(lineX > 240) {
          lineX = 240;
        }
        
        tft.drawLine(lineX, y, lineX, y + GRAPH_HEIGHT, TFT_BLACK);
        
        int lineYStart = y + (GRAPH_HEIGHT / 4) + 2;          
        if(devicesHistory->history[foundDeviceIndex].signalLevelsIndex == i) {
          lineYStart += 3;
        } else {
          tft.drawLine(lineX, lineYStart + 1, lineX, lineYStart + 3, TFT_BLACK);
        }
        if(lineYStart > 240) {
          lineYStart = 240;
        }
        
        int lineYEnd = y - lineHeight + (GRAPH_HEIGHT / 4) + 2;
        if(devicesHistory->history[foundDeviceIndex].signalLevelsIndex == i) {
          lineYEnd += 3;
        }
        if(lineYEnd > 240) {
          lineYEnd = 240;
        }
        
        if(devicesHistory->history[foundDeviceIndex].signalLevelsIndex == i) {
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
      xSemaphoreGive(devicesHistory->xDevicesSemaphore);
    }  
  }
}
