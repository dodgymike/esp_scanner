# esp_scanner
ESP32 BT/BTLE/Wifi scanner

# Purpose
Uses active scanning to detect Bluetooth devices in the vicinity of an ESP32. Displays them on a 240x240 ST7789 LCD screen.

# Hardware
Built for a Wroom32 module. 
Uses Bodmer's TFT_eSPI module, so screen pin setup is done there. 
Expects capacitive touch buttons on pin 14 (UP) and pin33 (DOWN).
