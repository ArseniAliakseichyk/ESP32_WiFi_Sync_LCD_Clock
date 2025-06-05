# ESP32 WiFi Sync LCD Clock

This project is a low-power clock using **ESP32** and a **SPI LCD display**.  
It gets time from the **internet (NTP)** and shows it on the screen.

## 🔋 Features

- **Low Power**
  - Wi-Fi turns on only to update time
  - Radio module sleeps between updates
  - Works up to 30 days on one 18650 battery

- **Smart Sync**
  - Time updates when the clock starts
  - Time sync every 30 minutes
  - Default time zone is UTC+2 (PL)

- **Good Display**
  - Only changes numbers that need to update
  - Other parts of the screen draw only once
  - Time is centered and clear
  - No screen flicker

## 📊 Technical Info

| Parameter             | Value               |
|----------------------|---------------------|
| Microcontroller      | ESP32               |
| Display              | SPI LCD 160x128     |
| Time Sync Protocol   | NTP (pool.ntp.org)  |
| Time Zone (default)  | UTC+3               |
| Sync Interval        | 30 minutes          |
| Accuracy             | ±1 second/month     |
| Power (active)       | 80 mA               |
| Power (sleep)        | < 100 µA            |

## 🛠️ Setup

1. **Install ESP-IDF**

2. **Clone the repository:**

```bash
git clone https://github.com/yourusername/ESP32_WiFi_Sync_LCD_Clock.git
cd ESP32_WiFi_Sync_LCD_Clock
```
## Set your Wi-Fi in main/main.c:

```c
#define WIFI_SSID "Your_SSID"
#define WIFI_PASS "Your_PASSWORD"
Set time zone (if needed):

setenv("TZ", "GMT-2", 1);
```
## Build and upload:

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
## Enjoy your ESP32 Clock! 😊