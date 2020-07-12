#ifndef __WIFI_TASK_H__
#define __WIFI_TASK_H__

#include <Arduino.h>
#include "devices_history.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_bt.h"

//extern DevicesHistory* devicesHistory;

void wifiTask(void* parameter);

typedef struct
{   
  int16_t fctl;
  int16_t duration;
  uint8_t da; 
  uint8_t sa; 
  uint8_t bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;
    
typedef struct {
  WifiMgmtHdr hdr; 
  uint8_t payload[0];
} wifi_ieee80211_packet_t;

typedef struct {
  uint8_t payload[];
  char ssid[];
} __attribute__((packed)) wifi_ieee80211_probe_t;

static void getMAC(char *addr, uint8_t* data, uint16_t offset);

#endif
