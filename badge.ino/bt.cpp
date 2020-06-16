#include "bt.hpp"

/*
void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice) {
    //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
}
*/

#define SCAN_NAME_SIZE (20)

struct ScanParameters {
  char scanNames[SCAN_NAME_SIZE][50];
  int scanRssis[SCAN_NAME_SIZE];
  int scanCount;
};

struct Dimensions
{
  int x;
  int y;
  int width;
  int height;
};


SemaphoreHandle_t xScanSemaphore;
ScanParameters scanParameters;

int scanTime = 2; //In seconds
int selectedScanEntry = 0;

char scanMac[25];
double scanHistory[360];
int scanHistoryIndex = 0;
int gooseHuntActive = 0;

void genericTask( void * parameter ){
  //delay(2000);

  ScanParameters *scanParameters = (ScanParameters*)parameter;
  
  int scanTime = 5;

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  while(true)
  {
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

    if ( xSemaphoreTake( xScanSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
        for(int i = 0; i < foundDevices.getCount(); i++) 
        {
            BLEAdvertisedDevice foundDevice = foundDevices.getDevice(i);      
            //sprintf(scanParameters->scanNames[i], "%s (%d)", foundDevice.getAddress().toString().c_str(), foundDevice.getRSSI());
            sprintf(scanParameters->scanNames[i], "%s", foundDevice.getAddress().toString().c_str());
            scanParameters->scanRssis[i] = foundDevice.getRSSI();
        }

        scanParameters->scanCount = foundDevices.getCount();

        xSemaphoreGive( xScanSemaphore ); // Now free or "Give" the Serial Port for others.
    }

    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

    //delay(500);
  }
}


int upButtonCounter = 0;
int downButtonCounter = 0;

bool initup = false;
bool btLoop()
{
    if (!initup) {
        initup = true;
        int i = 0;

        xScanSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the scans
        xSemaphoreGive(xScanSemaphore);  // Make the scan available for use, by "Giving" the Semaphore.

        scanParameters.scanCount = 0;
        for(int i = 0; i < SCAN_NAME_SIZE; i++) 
        {
          scanParameters.scanNames[i][0] = 0;
        }
        
//        Serial.println("Creating TASK");
        xTaskCreate(
            genericTask,       /* Task function. */
            "genericTask",     /* String with name of task. */
            25000,             /* Stack size in words. */
            (void*)&scanParameters,              /* Parameter passed as input of the task */
            2,                 /* Priority of the task. */
            NULL);             /* Task handle. */
    }

      Dimensions pixelDimensions;
      pixelDimensions.width = 2;
      pixelDimensions.height = 2;

      float angleMult = 6.283318 / 360.0;
      
      for(int i = 0; i < 360; i++) {
//        pixelDimensions.x = i * 2;
//        pixelDimensions.y = 50 + (-1 * scanHistory[i]);

        float angle = ((float)i) * angleMult;
        pixelDimensions.x = 120.0 + (-0.5 * scanHistory[i] * sin(angle));
        pixelDimensions.y = 140.0 + (-0.5 * scanHistory[i] * cos(angle));

      }

      if ( xSemaphoreTake( xScanSemaphore, ( TickType_t ) 5 ) == pdTRUE )
      {
          for(int i = 0; i < scanParameters.scanCount; i++) {
            if(strncmp(scanMac, scanParameters.scanNames[i], 25) != 0) {
              continue;
            }

            scanHistory[scanHistoryIndex] = scanParameters.scanRssis[i];
            scanHistoryIndex++;
            if(scanHistoryIndex >=  360) {
              scanHistoryIndex = 0;
            }
          }
          
          xSemaphoreGive( xScanSemaphore );
      }    
    //}

    if(0) {
    } else 
    {
        gooseHuntActive ++;
        if ( xSemaphoreTake( xScanSemaphore, ( TickType_t ) 5 ) == pdTRUE )
        {
            xSemaphoreGive( xScanSemaphore );
        }
      }
    
      // See if we can obtain or "Take" the Serial Semaphore.
      // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
      if ( xSemaphoreTake( xScanSemaphore, ( TickType_t ) 5 ) == pdTRUE )
      {
          int y = 18;
          for(int i = 0; i < scanParameters.scanCount; i++) {
            int textColour = 0xFF;
            if(selectedScanEntry == i) {
              textColour = 0x3F;
              
              y += 16;
            } else {
              y += 8;
            }
          }
  
          
          xSemaphoreGive( xScanSemaphore );
      }

//    }
  
    return false;
}
