# PS4Bluepad32

Reusable ESP-IDF component for connecting a DualShock 4 with Bluepad32 and
reading button and analog state. It is designed for PlatformIO projects using
`framework = espidf`; it is not an Arduino-framework `lib_deps` library.

## Download

- [Download the complete PlatformIO project as ZIP](https://github.com/phm-tuyenn/ps4-esp32/archive/refs/heads/main.zip)
- [Browse the PS4Bluepad32 component](https://github.com/phm-tuyenn/ps4-esp32/tree/main/components/PS4Bluepad32)

The simplest way to try the library is to download or clone the complete
repository and build it as-is. The repository includes the PlatformIO
configuration, dependency preparation script, and a serial monitor example.

## Build the included PlatformIO example

1. Install VS Code and the PlatformIO IDE extension.
2. Download and extract the ZIP above, or clone the repository:

   ```powershell
   git clone https://github.com/phm-tuyenn/ps4-esp32.git
   cd ps4-esp32
   ```

3. Install Python 3.10–3.13 and PlatformIO Core 6.1.x. This project uses
   ESP-IDF 5.5.1 with the Arduino core as an ESP-IDF component. Do not change
   the environment to `framework = arduino`.
4. Prepare dependencies once. Open PowerShell in the project directory:

   ```powershell
   git clone https://github.com/bluekitchen/btstack.git external/btstack
   git -C external/btstack checkout 8a8b616e04b07612a273978131efad32a083b1ed
   powershell -ExecutionPolicy Bypass -File .\scripts\prepare-dependencies.ps1
   ```

5. Open the project folder in VS Code and let PlatformIO finish initializing.
   Build, flash, and open Serial Monitor (115200 baud):

   ```powershell
   py -3.13 -m platformio run -e esp32doit-devkit-v1
   py -3.13 -m platformio run -e esp32doit-devkit-v1 -t upload
   py -3.13 -m platformio device monitor --port COM21 --baud 115200
   ```

   Replace `COM21` with your ESP32 serial port. The equivalent PlatformIO
   commands are available from the VS Code PlatformIO toolbar.

This included example auto-connects to the first controller, stores its MAC,
and prints button presses, joystick percentages, and L2/R2 percentages. Hold
**SHARE + PS** to pair a DualShock 4.

## Use the component in another PlatformIO ESP-IDF project

This is an ESP-IDF component, so PlatformIO's `lib_deps` mechanism is not used.
The consuming project must also provide Bluepad32, the Bluepad32 Arduino
adapter, Arduino core, and BTstack.

1. Copy `components/PS4Bluepad32` from the downloaded repository into the
   consuming project's `components/PS4Bluepad32`.
2. Prepare the same Bluepad32/Arduino/BTstack dependencies in the consumer's
   ESP-IDF component directories. See the root repository's
   `scripts/prepare-dependencies.ps1`, `main/idf_component.yml`, and
   `CMakeLists.txt` for the pinned dependency versions and integration setup.
   The library folder alone does not download these dependencies.
3. Configure `platformio.ini` for the ESP-IDF framework and point the
   application source directory at `main`:

   ```ini
   [platformio]
   src_dir = main

   [env:esp32doit-devkit-v1]
   platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.31-2/platform-espressif32.zip
   board = esp32doit-devkit-v1
   framework = espidf
   monitor_speed = 115200
   ```

4. Add the component dependency to `main/CMakeLists.txt`:

   ```cmake
   idf_component_register(
       SRCS "main.cpp"
       INCLUDE_DIRS "."
       REQUIRES PS4Bluepad32
   )
   ```

5. Include the public header and call the API from `main/main.cpp` as shown
   below, then build with `pio run`.

## API

```cpp
#include <PS4Bluepad32.h>

void setup() {
    ps4::begin();
}

void loop() {
    ps4::update();

    if (ps4::isConnected()) {
        ps4::Data data;
        if (ps4::getData(data)) {
            if (data.buttons & ps4::BUTTON_CROSS) {
                // Cross is pressed.
            }
        }
    }
}
```

Call `ps4::update()` continuously. `ps4::getData()` returns `true` when a new
controller report is available. `Data::buttons` is a bitmask of the `BUTTON_*`
constants. Stick axes are signed percentages from -100 to 100 (positive Y is
up); L2 and R2 are percentages from 0 to 100.

The component uses GPIO2 for the built-in LED. Call `ps4::resetMac()` after
`ps4::begin()` to clear the saved controller MAC/allowlist. It returns `true`
on success, disconnects the active controller if any, and saves the next
controller that connects. It returns `false` and prints an error if called
before initialization or if Bluepad32 cannot clear the allowlist.

For example, call it from your own menu, command handler, or button event:

```cpp
void forgetControllerAndRePair() {
    if (!ps4::resetMac()) {
        Console.println("Could not reset the saved controller MAC.");
    }
}
```

Turn off the old controller before resetting if you want to pair a different
one, and keep other controllers off while pairing. This clears the allowlist,
not Bluetooth bond keys.

See `examples/SerialMonitor` for a complete serial monitor example.
