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


uint8_t probeBytes[] = {
  64,
  0,
  0,
  0,
  
  255,
  255,
  255,
  255,
  255,
  255,
  
  0,
  17,
  34,
  51,
  68,
  85,
  
  255,
  255,
  255,
  255,
  255,
  255,
  
  0,
  0,
  
  0, // ssid length?  
  7, // ssid length?
  
  109,
  111,
  111,
  109,
  109,
  111,
  111
};


//40 00 00 00 ffffffffffff d4ca6d190d65 ffffffffffff c03f
//CAPABILITIES: 000c54502d4c696e6b5f43324338010802040b160c1218242d1a2c1003ffff00000000000000000000000000000000000000000032043048606cdd1e00904c332c1003ffff000000000000000000000000000000000000000000


wifi_ieee80211_probe_t* probeData = (wifi_ieee80211_probe_t*)&probeBytes;

int previousChannel = 0;

WifiTaskParameter* wifiTaskParameter = NULL;

void wifiTask(void* parameter) {
  wifiTaskParameter = (WifiTaskParameter*)parameter;

  DevicesHistory* wifiDevicesHistory = wifiTaskParameter->devicesHistory;

  int set_channel = 0;

/*
  for(int i = 0; i < 10; i++) {
    Serial.println("WIFI Wait");
    delay(1000);
  }

  while(wifiDevicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_WIFI) {
    Serial.println("NOT WIFI MODE");
    delay(1000);
  }
  
  for(int i = 0; i < 10; i++) {
    Serial.println("WIFI Wait 2");
    delay(1000);
  }
*/
 
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  //esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  
//  WiFi.mode(WIFI_STA);
//  WiFi.disconnect();
  delay(100);

  while(true) {
    if(wifiDevicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_WIFI) {
      esp_wifi_stop();

      while(wifiDevicesHistory->getScanMode() != DEVICES_HISTORY_SCAN_MODE_WIFI) {
        Serial.println("NOT WIFI MODE");
        delay(100);
      }

      esp_wifi_start();
    }  
    
    int wifiChannel = wifiDevicesHistory->getWifiChannel();
    
    if(wifiChannel == 0) {
      for(int i = 1; i < 12; i++) {
        int response = esp_wifi_set_channel(i, WIFI_SECOND_CHAN_NONE);
        int txStatus = esp_wifi_80211_tx(WIFI_IF_STA, probeData, 33, true);
  
        delay(50);
      }

      previousChannel = wifiChannel;
    } else {   
      if(previousChannel != wifiChannel) {
        int response = esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);
        previousChannel = wifiChannel;
      }
      delay(50);
    }
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

  int ffCount = 0;
  for(int i = 0; i < 6; i++) {
    if(((uint8_t)snifferPacket->payload[4 + i]) == 0xff) {
      ffCount++;
    }
  }

  uint8_t receiver[6];
  uint8_t sender[6];
  bool receiverBroadcast = true;
  bool senderBroadcast = true;
  for(int i = 0; i < 6; i++) {
    receiver[i] = snifferPacket->payload[4 + i];
    sender[i]   = snifferPacket->payload[10 + i];

    if(receiver[i] != 0xff) {
      receiverBroadcast = false;
    }
    if(sender[i] != 0xff) {
      senderBroadcast = false;
    }
  }

  if(wifiTaskParameter->devicesHistory->apAddressBlank()) {
    //Serial.println("AP Address is blank");
  } else if(wifiTaskParameter->devicesHistory->isApAddress(sender, receiver) && (receiverBroadcast || senderBroadcast)) {
    //Serial.println("Is AP Broadcast");
  } else if(wifiTaskParameter->devicesHistory->isApAddress(sender, receiver)) {
//    Serial.println("Is AP Address");
//    Serial.print("Got packet: ");
//    Serial.print(receiver_address);
//    Serial.print(" - ");
//    Serial.println(sender_address);
//    Serial.println("");

    if ( xSemaphoreTake(wifiTaskParameter->apDevicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {        
        char deviceName[200];
        bzero(deviceName, 200);
        uint8_t deviceAddress[6];
        
        if(wifiTaskParameter->devicesHistory->isApAddress(sender, NULL)) {
          char receiver_address[] = "00:00:00:00:00:00";
          getMAC(receiver_address, snifferPacket->payload, 4);
          sprintf(deviceName, "%s", receiver_address);
          for(int i = 0; i < 6; i++) {
            deviceAddress[i] = receiver[i];
          }
        } else {
          char sender_address[] = "00:00:00:00:00:00";
          getMAC(sender_address, snifferPacket->payload, 10);
          sprintf(deviceName, "%s", sender_address);
          for(int i = 0; i < 6; i++) {
            deviceAddress[i] = sender[i];
          }
        }
        
        int foundDeviceIndex = -1;
        for(int deviceIndex = 0; deviceIndex < wifiTaskParameter->apDevicesHistory->getCount(); deviceIndex++) {
           if(wifiTaskParameter->apDevicesHistory->history[deviceIndex].checkAddress(deviceAddress)) {
                foundDeviceIndex = deviceIndex;
                break;
           }
        }

        if(foundDeviceIndex == -1) {
          wifiTaskParameter->apDevicesHistory->incrementCount();
          foundDeviceIndex = wifiTaskParameter->apDevicesHistory->getCount() - 1;

          wifiTaskParameter->apDevicesHistory->history[foundDeviceIndex].setAddress(deviceAddress);
          wifiTaskParameter->apDevicesHistory->history[foundDeviceIndex].setName(deviceName);
        }

        int rssi = snifferPacket->rx_ctrl.rssi;
        int channel = snifferPacket->rx_ctrl.channel;
        wifiTaskParameter->apDevicesHistory->history[foundDeviceIndex].setSignalLevel(rssi);
        wifiTaskParameter->apDevicesHistory->history[foundDeviceIndex].setChannel(channel);
        
        xSemaphoreGive( wifiTaskParameter->apDevicesHistory->xDevicesSemaphore );
    }
  } else {
    //Serial.println("Is Not AP Address");
  }

/*
  if(ffCount != 6) {
    char receiver_address[] = "00:00:00:00:00:00";
    char sender_address[] = "00:00:00:00:00:00";
    getMAC(receiver_address, snifferPacket->payload, 4);
    getMAC(sender_address, snifferPacket->payload, 10);
    Serial.print("Got packet: ");
    Serial.print(receiver_address);
    Serial.print(" - ");
    Serial.println(sender_address);
    Serial.println("");
  }
*/
  
  if (type == WIFI_PKT_MGMT)
  {
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    if(fctl == 0x4000) {
      Serial.println("");
      Serial.println("!!!! GOT PROBE PACKET !!!!");

      char deviceName[100];

      int offset = 4;
      
      char receiver_address[] = "00:00:00:00:00:00";
      getMAC(receiver_address, snifferPacket->payload, offset);
      sprintf(deviceName, "%s", receiver_address);
      Serial.println(receiver_address);

      offset += 6;
      
      char sender_address[] = "00:00:00:00:00:00";
      getMAC(sender_address, snifferPacket->payload, offset);
      sprintf(deviceName, "%s", sender_address);
      Serial.println(sender_address);

      offset += 6;

      // BSSID
      offset += 6;
      // fragment
      offset++;
      // seq number
      offset++;

      len -= offset;
      int payloadOffset = offset;
      
      while(len > 0) {
        uint8_t tagType = snifferPacket->payload[payloadOffset];
        uint8_t tagLength = snifferPacket->payload[payloadOffset + 1];
        payloadOffset += 2;
        len -= 2;

//        char pBuf[100];
//        sprintf(pBuf, "%d:%d", tagType, tagLength);
//        Serial.println(pBuf);

        if(tagType == 0) {
          for (int i = 0; i < tagLength; i++)
          {
            //Serial.print((char)snifferPacket->payload[i + 38]);
           display_string.concat((char)snifferPacket->payload[payloadOffset + i]);
          }
          if(tagLength > 0) {
            Serial.println(display_string.c_str());
//
//            if ( xSemaphoreTake(wifiTaskParameter->probeDevicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
//              bzero(deviceName, 200);
//              sprintf(deviceName, "%s\n%s", display_string.c_str(), sender_address);
//              
//              int foundDeviceIndex = -1;
//              for(int deviceIndex = 0; deviceIndex < wifiTaskParameter->probeDevicesHistory->getCount(); deviceIndex++) {
//                 if(wifiTaskParameter->probeDevicesHistory->history[deviceIndex].checkName(deviceName)) {
//                      foundDeviceIndex = deviceIndex;
//                      break;
//                 }
//              }
//      
//              if(foundDeviceIndex == -1) {
//                wifiTaskParameter->probeDevicesHistory->incrementCount();
//                foundDeviceIndex = wifiTaskParameter->probeDevicesHistory->getCount() - 1;
//      
//                wifiTaskParameter->probeDevicesHistory->history[foundDeviceIndex].setName(deviceName);
//              }
//      
//              int rssi = snifferPacket->rx_ctrl.rssi;
//              int channel = snifferPacket->rx_ctrl.channel;
//              wifiTaskParameter->probeDevicesHistory->history[foundDeviceIndex].setSignalLevel(rssi);
//              wifiTaskParameter->probeDevicesHistory->history[foundDeviceIndex].setChannel(channel);
//              
//              xSemaphoreGive( wifiTaskParameter->probeDevicesHistory->xDevicesSemaphore );
//            }


            
          }
        }
                
        payloadOffset += tagLength;
        len -= tagLength;
      }
    }

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

/*
*/
      if ( xSemaphoreTake(wifiTaskParameter->devicesHistory->xDevicesSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
        uint8_t deviceAddress[6];
        for(int i = 0; i < 6; i++) {
          deviceAddress[i] = snifferPacket->payload[i + 10];
        }
        
        char deviceName[200];
        bzero(deviceName, 200);
        sprintf(deviceName, "%s\n%s", display_string.c_str(), deviceAddressBSSID);
        
        int foundDeviceIndex = -1;
        for(int deviceIndex = 0; deviceIndex < wifiTaskParameter->devicesHistory->getCount(); deviceIndex++) {
//           if(wifiTaskParameter->devicesHistory->history[deviceIndex].checkName(deviceName)) {
           if(wifiTaskParameter->devicesHistory->history[deviceIndex].checkAddress(deviceAddress)) {
                foundDeviceIndex = deviceIndex;
                break;
           }
        }

        if(foundDeviceIndex == -1) {
          wifiTaskParameter->devicesHistory->incrementCount();
          foundDeviceIndex = wifiTaskParameter->devicesHistory->getCount() - 1;

          wifiTaskParameter->devicesHistory->history[foundDeviceIndex].setAddress(deviceAddress);
          wifiTaskParameter->devicesHistory->history[foundDeviceIndex].setName(deviceName);
          wifiTaskParameter->devicesHistory->history[foundDeviceIndex].clean();
        }

        int rssi = snifferPacket->rx_ctrl.rssi;
        int channel = snifferPacket->rx_ctrl.channel;
        wifiTaskParameter->devicesHistory->history[foundDeviceIndex].setSignalLevel(rssi);
        wifiTaskParameter->devicesHistory->history[foundDeviceIndex].setChannel(channel);
        
        xSemaphoreGive( wifiTaskParameter->devicesHistory->xDevicesSemaphore );
      }
    }
  }
}
