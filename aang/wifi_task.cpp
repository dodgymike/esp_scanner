#include "wifi_task.h"

#include "WiFi.h"
#include "devices_history.h"

void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

String translateEncryptionType(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case (0):
      return "Open";
    case (1):
      return "WEP";
    case (2):
      return "WPA_PSK";
    case (3):
      return "WPA2_PSK";
    case (4):
      return "WPA_WPA2_PSK";
    case (5):
      return "WPA2_ENTERPRISE";
    default:
      return "UNKOWN";
  }
}

DevicesHistory* wifiDevicesHistory = NULL;

void wifiTask(void* parameter) {
  wifiDevicesHistory = (DevicesHistory*)parameter;

  int set_channel = 1;

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  
//  WiFi.mode(WIFI_STA);
//  WiFi.disconnect();
  delay(100);

  while(true) {  
    delay(1000);
  }
}

void getMAC(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    if ((snifferPacket->payload[0] == 0x80))
    {
//      delay(random(0, 10));
//      Serial.print("RSSI: ");
//      Serial.print(snifferPacket->rx_ctrl.rssi);
//      Serial.print(" Ch: ");
//      Serial.print(snifferPacket->rx_ctrl.channel);
      //Serial.print(" BSSID: ");
      char deviceAddressBSSID[] = "00:00:00:00:00:00";
      getMAC(deviceAddressBSSID, snifferPacket->payload, 10);
      //Serial.print(deviceAddressBSSID);
      //display_string.concat(deviceAddressBSSID);
      //Serial.print(" ESSID: ");
      //display_string.concat(" -> ");
      for (int i = 0; i < snifferPacket->payload[37]; i++)
      {
        //Serial.print((char)snifferPacket->payload[i + 38]);
        display_string.concat((char)snifferPacket->payload[i + 38]);
      }

//      int temp_len = display_string.length();
//      for (int i = 0; i < 40 - temp_len; i++)
//      {
//        display_string.concat(" ");
//      }

//      Serial.print(" ");
//      Serial.println();

      if ( xSemaphoreTake( wifiDevicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
/*
            //char deviceAddress[200];
            //char bssid[100];
            char ssidBuffer[100];
              
            //bzero(deviceAddress, 200);
            //bzero(bssid, 100);
            bzero(ssidBuffer, 100);
  
            //WiFi.SSID(ssid);
  
  //          WiFi.BSSID(i).copy(bssid, 80);
  //          WiFi.SSID(i).copy(ssid, 80);
  
  //          snprintf(deviceAddress, 80, "%x:%x:%x:%x:%x:%x\n%s", WiFi.BSSID(i)[0], WiFi.BSSID(i)[1], WiFi.BSSID(i)[2], WiFi.BSSID(i)[3], WiFi.BSSID(i)[4], WiFi.BSSID(i)[5], ssid);
            //snprintf(deviceAddress, 80, "%s\n%s", WiFi.BSSIDstr(i), WiFi.SSID(i));
            //snprintf(deviceAddress, 80, "%s", WiFi.SSID(i));
  //          String ssid = WiFi.SSID(i);
  //          //ssid.copy(ssidBuffer, 80);
  //          
  //          sprintf(deviceAddress, "%s\n%x:%x:%x:%x:%x:%x", ssid.c_str(), WiFi.BSSID(i)[0], WiFi.BSSID(i)[1], WiFi.BSSID(i)[2], WiFi.BSSID(i)[3], WiFi.BSSID(i)[4], WiFi.BSSID(i)[5]);
                        
*/

            char deviceAddress[200];
            bzero(deviceAddress, 200);
            sprintf(deviceAddress, "%s\n%s", display_string.c_str(), deviceAddressBSSID);
            
            int foundDeviceIndex = -1;
            for(int deviceIndex = 0; deviceIndex < wifiDevicesHistory->getCount(); deviceIndex++) {
               if(wifiDevicesHistory->history[deviceIndex].checkName(deviceAddress)) {
                    foundDeviceIndex = deviceIndex;
                    break;
               }
            }

            if(foundDeviceIndex == -1) {
              wifiDevicesHistory->incrementCount();
              foundDeviceIndex = wifiDevicesHistory->getCount() - 1;
  
              wifiDevicesHistory->history[foundDeviceIndex].setName(deviceAddress);
            }

            int rssi = snifferPacket->rx_ctrl.rssi;
            wifiDevicesHistory->history[foundDeviceIndex].setSignalLevel(rssi);
            
            /*
            wifiDevicesHistory->history[foundDeviceIndex].setSignalLevel(snifferPacket->rx_ctrl.rssi);
        
      */
        xSemaphoreGive( wifiDevicesHistory->xDevicesSemaphore );
      }
    }
  }
}

/*
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

            if(foundDevice.haveName()) {
//              strncpy(deviceAddress, foundDevice.getName(), 180);
              bzero(deviceAddress, 180);
              foundDevice.getName().copy(deviceAddress, 20);
            }
            
            devicesHistory->history[foundDeviceIndex].setName(deviceAddress);
          }
          
          devicesHistory->history[foundDeviceIndex].setSignalLevel(foundDevice.getRSSI());
      }
      
      xSemaphoreGive( devicesHistory->xDevicesSemaphore );
    }
         
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    delay(100);
  }
 */
