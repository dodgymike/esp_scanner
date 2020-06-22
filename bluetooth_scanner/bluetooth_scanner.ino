#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <math.h>

#include <TFT_eSPI.h>

#define BaudRate 115200

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define SCAN_NAME_SIZE (20)
#define GRAPH_HEIGHT (20.0f)

#define DEVICE_ADDRESS_SIZE (50)
#define DEVICE_HISTORY_SIZE (80)

class ButtonState {
  private:
    bool buttonStates[6];
    int  buttonTops[6];
    
  public:
    static const int up    = 0;
    static const int down  = 1;
    static const int right = 2;
    static const int left  = 3;
  
    SemaphoreHandle_t xButtonSemaphore;

    void set(int button, int value) {
      if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
        buttonStates[button] = (value <= buttonTops[button]);

        xSemaphoreGive(xButtonSemaphore);
      }
    }

    void setTop(int button, int value) {
      if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
        buttonTops[button] = value;
        
        xSemaphoreGive(xButtonSemaphore);
      }
    }

    bool pressed(int button) {
      bool rv = false;

      if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
        if(buttonStates[button]) {
          rv = true;
          buttonStates[button] = false;
        }

        xSemaphoreGive(xButtonSemaphore);
      }

      return rv;
    }

    ButtonState()
      : xButtonSemaphore(xSemaphoreCreateMutex())
    {
      for(int i = 0; i < 6; i++) {
        buttonStates[i] = false;
        buttonTops[i] = 0;
      }
  
      xSemaphoreGive(xButtonSemaphore);
    }
};

class DeviceHistory {
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
      
      if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
        rv = count;
        xSemaphoreGive(xCountSemaphore);
      }      

      return rv;
    }

    void incrementCount() {
      if (xSemaphoreTake(xCountSemaphore, ( TickType_t ) 5 ) == pdTRUE) {  
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

#define BUTTON_GPIO_UP          (14)
#define BUTTON_GPIO_DOWN        (33)
#define BUTTON_GPIO_LEFT        (32)
#define BUTTON_GPIO_RIGHT       (27)
#define BUTTON_GPIO_B           (2)
#define BUTTON_GPIO_LEFT_SELECT (12)
#define BUTTON_GPIO_A           (15)

void buttonTask(void* parameter) {
  ButtonState* buttonState = (ButtonState*)parameter;

  digitalWrite(BUTTON_GPIO_UP,   LOW); // UP
  digitalWrite(BUTTON_GPIO_DOWN, LOW); // DOWN
  digitalWrite(BUTTON_GPIO_RIGHT,     LOW); // RIGHT
  digitalWrite(BUTTON_GPIO_LEFT,      LOW); // LEFT

  pinMode(BUTTON_GPIO_UP,    INPUT); // UP
  pinMode(BUTTON_GPIO_DOWN,  INPUT); // DOWN
  pinMode(BUTTON_GPIO_RIGHT, INPUT); // UP
  pinMode(BUTTON_GPIO_LEFT,  INPUT); // DOWN

  unsigned long startMillis = millis();

  int upA    = 0;
  int downA  = 0;
  int leftA  = 0;
  int rightA = 0;
  int checkCount = 0;

  Serial.println("Getting Top");
  while(millis() - startMillis < 1500) {
    upA    += touchRead(BUTTON_GPIO_UP);
    downA  += touchRead(BUTTON_GPIO_DOWN);
    leftA  += touchRead(BUTTON_GPIO_LEFT);
    rightA += touchRead(BUTTON_GPIO_RIGHT);
    checkCount++;

    delay(50);
  }

  Serial.println("Setting Top");
  buttonState->setTop(ButtonState::up,    upA    / checkCount);
  buttonState->setTop(ButtonState::down,  downA  / checkCount);
  buttonState->setTop(ButtonState::left,  leftA  / checkCount);
  buttonState->setTop(ButtonState::right, rightA / checkCount);

  Serial.println("Handling buttons");
  while(true) {
    Serial.println("BOOP");
    
    buttonState->set(ButtonState::up,    touchRead(BUTTON_GPIO_UP));
    buttonState->set(ButtonState::down,  touchRead(BUTTON_GPIO_DOWN));
    buttonState->set(ButtonState::left,  touchRead(BUTTON_GPIO_LEFT));
    buttonState->set(ButtonState::right, touchRead(BUTTON_GPIO_RIGHT));

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

unsigned long previousTime = 0;

void loop()
{
  unsigned long currentTime = millis();
  if(currentTime - previousTime < 100) {
    return;
  }

  previousTime = currentTime;
  
  if(buttonState->pressed(ButtonState::up)) {
    deviceOffset--;
    tft.fillScreen(TFT_BLACK);
  } else if(buttonState->pressed(ButtonState::down)) {
    deviceOffset++;
    tft.fillScreen(TFT_BLACK);
  }
  
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
