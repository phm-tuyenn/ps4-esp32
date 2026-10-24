# PS4Bluepad32 for Arduino IDE

Arduino IDE library wrapper for the official ESP32 + Bluepad32 board package.
It provides a small API for DualShock 4 button, joystick, and trigger input,
and saves the first paired controller MAC in ESP32 Preferences.

## Requirements

- Arduino IDE 2.x.
- An ESP32 supported by the Bluepad32 Arduino board package.
- The official `ESP32 + Bluepad32` board package. The regular Espressif ESP32
  Arduino board package alone does not provide `<Bluepad32.h>`.

## Install the board packages

1. Open **File → Preferences** in Arduino IDE.
2. Add these URLs under **Additional boards manager URLs** (separate them with
   a comma if there are already other URLs):

   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json
   ```

3. Open **Tools → Board → Boards Manager** and install both `esp32` and
   `esp32_bluepad32`.
4. Select the target board under **Tools → Board → ESP32 + Bluepad32 Arduino**.

## Install this library

1. Download the project ZIP from
   <https://github.com/phm-tuyenn/ps4-esp32/archive/refs/heads/main.zip>.
2. Extract the ZIP.
3. Copy the folder
   `ps4-esp32-main/arduino-library/PS4Bluepad32` into the Arduino sketchbook's
   `libraries` directory. The default Windows path is
   `Documents\Arduino\libraries\PS4Bluepad32`.
4. Restart Arduino IDE. The library should appear under **File → Examples →
   PS4Bluepad32 → SerialMonitor**.

## Select the board and upload the example

1. Open **File → Examples → PS4Bluepad32 → SerialMonitor**.
2. Select your ESP32 + Bluepad32 board in **Tools → Board**.
3. Select the serial port in **Tools → Port**.
4. Click **Upload**.
5. Open **Tools → Serial Monitor** and set the baud rate to **115200**.
6. Hold **SHARE + PS** on the DualShock 4 until its light flashes.

The example reports newly pressed buttons, joystick axes as -100%..100%, and
L2/R2 as 0%..100%.

## Use the API in your sketch

```cpp
#include <PS4Bluepad32.h>

void setup() {
    Serial.begin(115200);
    if (!ps4::begin()) {
        Serial.println("PS4Bluepad32 initialization failed; check Serial Monitor.");
    }
}

void loop() {
    ps4::update();  // Call on every loop iteration.

    if (ps4::isConnected()) {
        ps4::Data data;
        if (ps4::getData(data)) {  // True only when a new controller report arrived.
            if (data.buttons & ps4::BUTTON_CROSS) {
                // Cross is currently pressed.
            }

            int8_t leftX = data.leftX;  // -100..100
            int8_t leftY = data.leftY;  // -100..100; positive is up
            uint8_t l2 = data.l2;       // 0..100
            uint8_t r2 = data.r2;       // 0..100
            (void)leftX;
            (void)leftY;
            (void)l2;
            (void)r2;
        }
    }
}
```

The full button mask list is in `src/PS4Bluepad32.h`. `begin()` accepts optional
LED and MAC-reset pin numbers; defaults are GPIO2 and GPIO27. Pass `-1` to
disable either pin, for example `ps4::begin(LED_BUILTIN, 27)`.

## Saved controller MAC and reset button

On first pairing, the MAC of the first valid gamepad is saved in ESP32
Preferences (NVS). It persists across power cycles. After that, a controller
with a different MAC is disconnected when Bluepad32 invokes the connection
callback. The Arduino Bluepad32 package does not expose the ESP-IDF allowlist
used by the ESP-IDF component, so this is callback-level rejection rather than
filtering before the Bluetooth connection begins.

The default reset input is GPIO27 configured as `INPUT_PULLDOWN`. Connect a
normally-open push button between **GPIO27 and 3V3**. Pressing it clears the
saved MAC and disconnects the current controller; the next valid controller
that sends a gamepad report is saved. Turn off the old controller before
resetting if you want to pair a different one. Do not connect GPIO27 to 5V.

If your board uses different pins, pass them to `begin(ledPin, resetPin)`.
Use `-1` to disable a pin; for example, `ps4::begin(-1, 27)` disables LED
control but leaves MAC reset enabled.

## Notes

- This library requires the **ESP32 + Bluepad32** board package; the standard
  ESP32 board package is not enough.
- It is installed by copying the library folder into the Arduino sketchbook's
  `libraries` directory. It is not yet published in Arduino Library Manager.
- The sibling `components/PS4Bluepad32` folder is a separate ESP-IDF component
  for PlatformIO projects using `framework = espidf`.
