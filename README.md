# FriendlyBot (Two-Eyed Bot)

An interactive, retro-futuristic desktop companion and kitchen assistant powered by an **ESP32-S3** and a dual-color 0.96" OLED display. Shaped with two potentiometer "eyes" and an OLED "mouth", it blends daily utility (NTP clock, outdoor weather, cooking timer) with playful character (sarcastic aphorisms, couple decision arbitrator, motion-reactive percussive maintenance).

---

## ✨ Features & Operating Modes

### 1. Desk Companion (Clock, Outdoor Weather & Aphorisms)
* **Smart Proximity Wake-Up (VL53L0X ToF Laser)**: The display powers off after 30 seconds of inactivity to preserve OLED life and save energy. Approaching within $\le 1.0\text{ m}$ or waving a hand wakes the device immediately.
* **NTP Clock & Outdoor Weather**: Synchronizes local time via NTP and fetches outdoor conditions (temperature and humidity) via the OpenWeatherMap API.
* **Sentence Engine & Aphorisms**: Displays witty phrases, milestone humor, or custom sayings (bilingual Italian/English support). Sentences that exceed screen space scroll smoothly upward like movie credits.
* **Interactive Sentence Controls (Action Button)**:
  * **Single Click**: Rewinds the currently scrolling sentence back to the start and resets the reading timer, letting you catch what you missed.
  * **Double Click** (within 400 ms): Immediately advances to the next sentence.
  * **From Sleep or Weather**: A single click immediately wakes the display and shows a new sentence.
* **Ambient Light Sensing (LDR)**: Automatically dims the display in dark environments and mutes random daytime gags at night.

---

### 2. Cooking / Kitchen Timer
* **Rotary Activation**: Rotating the **Timer Potentiometer** automatically transitions the bot into Timer mode with a retro CRT shutter animation.
* **Non-Linear 270° Mapping**: Calibrated from **0.5 to 30 minutes** with denser steps for shorter durations (e.g. boiling eggs or tea) and wider steps for longer cooking.
* **Start / Pause / Reset**:
  * **Single Click**: Starts or pauses the countdown timer.
  * **Long Press (1.5 s)**: Resets the timer back to its dial setting.
* **Audible Alarm**: An active buzzer beeps intermittently when the timer expires.

---

### 3. Couple Arbitrator ("The Decider")
* **Rotary Category Selection**: Rotating the **Arbitrator Potentiometer** enters Arbitrator mode to settle everyday household debates.
* **Categories**:
  * 🍕 *Food / Cosa mangiamo?* (Pizza, Pasta, Her Choice, His Choice, Delivery)
  * ⚖️ *Who's Right? / Chi ha ragione?* (Her, Him, Wine Compromise)
  * 🧹 *Chores / A chi tocca?* (Dishes, Dog Walk, Couch)
  * 🎬 *Entertainment / Cosa guardiamo?* (Movie, Series, Sleep Early)
* **Roll & Vote**: Press the front Action button (or shake the bot) to spin the roulette, complete with audio tick feedback. Automatically returns to the clock after 15 seconds of inactivity.

---

### 4. "Percussive Maintenance" (The Slap Gag)
* **Retro Glitch Trigger**: Randomly during daytime hours when preparing to enter sleep, the bot may simulate a 1970s television glitch with an `ERRORE 50` / `ERROR 50` prompt: `>> DAI UNA BOTTA! <<` / `>> SMACK UNIT! <<`.
* **Shock Detection**: Giving the enclosure a firm tap or slap triggers the **MPU-6050** accelerometer to detect the shock spike, play a repair animation with audio chirp, and return to normal operation.
* **Night Safety**: Automatically disabled in dark rooms via the light sensor.

---

### 5. Hardware Diagnostics Mode
* **Boot Self-Test**: Hold down the front Action button while powering on or resetting the bot.
* **Live Telemetry**: Displays normalized pot readings (0.00–1.00), timer minutes, light sensor ADC levels, ToF distance in millimeters, MPU shock/shake flags, and USB power status. Press the Action button to exit back to normal operation.

---

### 6. Web Configuration Portal & LittleFS Management
* **Zero-Code Wi-Fi Setup**: If Wi-Fi is unconfigured or unavailable, the bot starts a captive SoftAP access point (`Console50-Setup`). The screen displays the SSID, IP (`192.168.4.1`), and hostname (`bot50.local`), with an option to skip directly to offline operation.
* **Responsive Web Dashboard**: Once connected to your local network, browse to `http://bot50.local`:
  * **System Status**: Power profile (USB vs Battery), Wi-Fi signal strength, and uptime.
  * **Network & API**: Configure Wi-Fi credentials, OpenWeatherMap API key, coordinates, and language (Italian / English).
  * **Hardware Customization**:
    * Invert Timer Potentiometer rotation direction.
    * Invert Arbitrator Potentiometer rotation direction.
    * Invert Power Sense polarity.
    * Adjust Night Mode light threshold.
    * Adjust Slap sensitivity threshold (G-force).
  * **Sentence Manager**: Add, edit, or delete custom sentences stored in the on-board LittleFS partition.
  * **OTA Updates**: Flash updated firmware binaries directly from your web browser.

---

## 🛠️ Hardware & Bill of Materials (BOM)

| Component | Role | Notes |
| :--- | :--- | :--- |
| **ESP32-S3 Mini / DevKitC-1** | MCU / System Brain | Dual-core, 4MB Flash, 2MB PSRAM recommended (ESP32-S3FH4R2 Mini) |
| **0.96" OLED Display** | Screen ("Mouth") | I2C, SSD1306/SSD1315, 128×64, Yellow/Blue zone |
| **VL53L0X Time-of-Flight Sensor** | Smart Proximity Wake-Up | I2C Laser distance sensor ($\le 1.0\text{ m}$) |
| **MPU-6050 Breakout** | Motion & Shock Sensor | I2C Accelerometer / Gyroscope |
| **2× 10kΩ Linear Potentiometers** | Rotary "Eyes" | Timer Dial & Arbitrator Dial (neutral placement) |
| **5528 LDR (Photoresistor)** | Ambient Light Sensor | Auto-dimming & night gag mute |
| **Active 5V Buzzer** | Audio Feedback | Driven via 2N2222 NPN transistor |
| **2N2222 NPN Transistor** | Buzzer Driver | Controls active buzzer |
| **1N4007 Diode** | Flyback Protection | Across buzzer terminals |
| **1kΩ Resistor** | Transistor Base Resistor | In series with ESP32 buzzer pin |
| **10kΩ Resistor** | LDR Divider | Pull-down for photoresistor |
| **Tactile Pushbutton** | Front Action Button | Cheek / front panel button (`GPIO 10`) |
| **Microswitch / Tactile Button** | Rear Factory Reset Button | Hidden pinhole switch on back panel (`GPIO 12`) |
| **DPDT Toggle / Slide Switch** | Power Selection & Sense | Pole 1: USB vs Battery power; Pole 2: `GPIO 4` level |
| **USB-C Female Breakout** | Power Input | Panel-mounted on rear cover |
| **4× AA NiMH Batteries & Bay** | Portable Power | 4.8V nominal rechargeable battery pack |

---

## 🔌 Wiring & Pin Mapping

All components communicate with the ESP32-S3 using standard 3.3V logic levels.

```
                            ESP32-S3
                     ┌──────────────────┐
                     │   GPIO 8 (SDA)   │───► OLED, MPU6050, VL53L0X (Shared I2C)
                     │   GPIO 9 (SCL)   │───► OLED, MPU6050, VL53L0X (Shared I2C)
                     │   GPIO 6 (INT)   │───► MPU6050 Interrupt Pin
                     │                  │
                     │   GPIO 1 (ADC1)  │───► Timer Potentiometer Wiper (0–3.3V)
                     │   GPIO 2 (ADC1)  │───► Arbitrator Potentiometer Wiper (0–3.3V)
                     │   GPIO 3 (ADC1)  │───► 5528 LDR Divider Midpoint
                     │                  │
                     │   GPIO 4 (IN)    │───► DPDT Switch Pole 2 (Power Sense: USB/Batt)
                     │   GPIO 5 (OUT)   │───► 1kΩ Resistor ──► 2N2222 Base ──► Buzzer
                     │                  │
                     │   GPIO 10 (IN)   │───► Front Action Button (Active LOW)
                     │   GPIO 12 (IN)   │───► Hidden Reset Button (Active LOW, 3s hold)
                     │                  │
                     │   5V / VIN       │◄─── DPDT Switch Pole 1 (USB 5V or Batt +4.8V)
                     │   3.3V & GND     │───► Sensor Power Rails & Common Ground
                     └──────────────────┘
```

### Power Switching & USB Detection (DPDT Switch)
Power selection and source detection are handled mechanically by a **Double-Pole Double-Throw (DPDT)** switch:
* **Pole 1 (Power Rail)**: Switches the ESP32 `5V / VIN` input between the panel-mount USB-C 5V supply and the +4.8V positive battery terminal.
* **Pole 2 (Power Sense)**: Connects `GPIO 4` to a logic state reflecting the active source (e.g. pulled LOW or switched to 3.3V). The firmware debounce and inversion settings allow any switch orientation to be mapped correctly from the web portal.

### Potentiometer Placement Flexibility
The firmware refers to the two rotary inputs by their function (**Timer** on `GPIO 1` and **Arbitrator** on `GPIO 2`) rather than "left" or "right". You can place either control on either eye of the bot to fit your physical wiring. If a potentiometer reads backwards relative to its physical rotation, toggle the inversion switch in the web dashboard—no re-soldering required.

---

## 💻 Building & Flashing

The project is built with **PlatformIO** targeting the Espressif 32 platform with Arduino framework and C++20 standard (`-std=gnu++2a`).

### 1. Build and Upload Firmware
```bash
# Compile and flash firmware to ESP32-S3
pio run -e esp32-s3-mini --target upload

# Upload LittleFS filesystem partition (custom jokes and web assets)
pio run -e esp32-s3-mini --target uploadfs

# Open serial monitor
pio device monitor -b 115200
```

### 2. Custom Partition Table (`partitions.csv`)
The project utilizes a custom partition layout supporting dual OTA slots and a LittleFS data partition:
* `ota_0` / `ota_1`: 1.85 MB each for seamless over-the-air firmware updates.
* `spiffs` (LittleFS): 250 KB for storing custom aphorisms and localized files.
* `nvs`: 24 KB for Wi-Fi credentials and persistent hardware configuration.

### 3. Running Native Unit Tests
Pure business logic (sentence selection, text formatting, state machine transitions, non-linear pot mapping, time formatting, and sensor filters) is verified via native C++ tests:
```bash
clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks \
  src/i18n.cpp src/display.cpp src/textFormatter.cpp src/sentenceEngine.cpp \
  src/drivers/proximitySensor.cpp src/drivers/ldrSensor.cpp src/drivers/motionSensor.cpp \
  src/services/timeService.cpp src/services/weatherService.cpp src/services/webUtils.cpp \
  src/services/configServer.cpp src/services/networkWorker.cpp src/controllers/deskCompanionController.cpp \
  test/test_desk_companion_controller.cpp -o test/test_desk_companion_controller && ./test/test_desk_companion_controller
```

---

## 3D Model

A 3D model is available at https://makerworld.com/en/models/3309871-interactive-desk-companion-kitchen-timer

---

## 📄 License & Attribution

© 2026 Simone Tellini — [tellini.info](https://tellini.info)  
Licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)**.
