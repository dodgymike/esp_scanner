#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    
    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      
      for (int i = 0; i < rxValue.length(); i++) { 
        Serial.print(rxValue[i]); 
      } 
      Serial.println(); // Do stuff based on the command received from the app 
      if (rxValue.find("ON") != -1) { 
        Serial.println("Turning ON!"); 
//        digitalWrite(LED, HIGH); 
      } else if (rxValue.find("OFF") != -1) { 
        Serial.println("Turning OFF!"); 
//        digitalWrite(LED, LOW); 
      } 
      Serial.println(); 
      Serial.println("*********"); 
    } 
  } 
};

void setup() {
  BLEDevice::init("FooMoo");

  BLEServer *pServer = BLEDevice::createServer(); 
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                     CHARACTERISTIC_UUID,
                                     BLECharacteristic::PROPERTY_READ |
                                     BLECharacteristic::PROPERTY_WRITE
                                     );

  pCharacteristic->setValue("Hello World says Neil");

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  //BLEAdvertisementData *padvertisementData = 
  //pAdvertising->setAdvertisingData(advertisementData);
  pAdvertising->setMaxInterval(2000);
  pAdvertising->setMinInterval(150);
  
  pAdvertising->start();


//  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
//  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
//  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
//  pBLEScan->setInterval(100);
//  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // put your main code here, to run repeatedly:

}
