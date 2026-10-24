// SPDX-License-Identifier: GPL-3.0-or-later
#include "PS4Bluepad32.h"

#include <Arduino.h>
#include <Bluepad32.h>
#include <Preferences.h>
#include <string.h>

namespace ps4 {
namespace {

constexpr char kPreferencesNamespace[] = "ps4-bluepad32";
constexpr char kMacKey[] = "controller-mac";
constexpr uint32_t kButtonDebounceMs = 40;
constexpr size_t kBluetoothAddressSize = 6;

ControllerPtr activeController = nullptr;
Preferences preferences;
int8_t ledPin = 2;
int8_t macResetPin = 27;
bool started = false;
bool preferencesReady = false;
bool storageReady = false;
bool savedMacValid = false;
bool learnControllerMac = false;
uint8_t savedMac[kBluetoothAddressSize] = {};
bool resetButtonRawState = false;
bool resetButtonStableState = false;
uint32_t resetButtonLastChange = 0;

bool sameAddress(const uint8_t* left, const uint8_t* right) {
    for (size_t i = 0; i < kBluetoothAddressSize; ++i) {
        if (left[i] != right[i]) {
            return false;
        }
    }
    return true;
}

void printAddress(const uint8_t* address) {
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                  address[0], address[1], address[2],
                  address[3], address[4], address[5]);
}

void setLed(bool enabled) {
    if (ledPin >= 0) {
        digitalWrite(static_cast<uint8_t>(ledPin), enabled ? HIGH : LOW);
    }
}

void disconnectController(ControllerPtr controller, const char* reason) {
    Serial.printf("[PS4] Rejecting controller: %s\n", reason);
    controller->disconnect();
}

bool saveControllerMac(ControllerPtr controller) {
    const ControllerProperties properties = controller->getProperties();

    if (preferences.putBytes(kMacKey, properties.btaddr, kBluetoothAddressSize) != kBluetoothAddressSize) {
        Serial.println("[PS4] ERROR: Failed to save controller MAC in NVS.");
        return false;
    }

    memcpy(savedMac, properties.btaddr, kBluetoothAddressSize);
    savedMacValid = true;
    learnControllerMac = false;
    Serial.print("[PS4] Saved controller MAC: ");
    printAddress(savedMac);
    Serial.println();
    return true;
}

void onControllerConnected(ControllerPtr controller) {
    if (!storageReady) {
        disconnectController(controller, "MAC storage is unavailable");
        return;
    }

    if (activeController != nullptr) {
        disconnectController(controller, "another controller is already active");
        return;
    }

    const ControllerProperties properties = controller->getProperties();
    if (savedMacValid && !sameAddress(properties.btaddr, savedMac)) {
        Serial.print("[PS4] Saved MAC is ");
        printAddress(savedMac);
        Serial.print("; received ");
        printAddress(properties.btaddr);
        Serial.println();
        disconnectController(controller, "MAC does not match the paired controller");
        return;
    }

    activeController = controller;
    setLed(true);
    Serial.printf("[PS4] Connected: %s\n", controller->getModelName().c_str());
    Serial.println("[PS4] Waiting for gamepad input.");
}

void onControllerDisconnected(ControllerPtr controller) {
    if (activeController != controller) {
        return;
    }

    activeController = nullptr;
    setLed(false);
    Serial.println("[PS4] Controller disconnected");
}

int8_t axisPercent(int32_t value, bool invert) {
    if (value > 512) value = 512;
    if (value < -512) value = -512;
    if (invert) value = -value;

    int percent = value >= 0 ? (value * 100 + 256) / 512 : (value * 100 - 256) / 512;
    if (percent >= -3 && percent <= 3) percent = 0;
    return static_cast<int8_t>(percent);
}

uint8_t triggerPercent(int32_t value) {
    if (value < 0) value = 0;
    if (value > 1020) value = 1020;
    return static_cast<uint8_t>((value * 100 + 510) / 1020);
}

uint32_t readButtons(ControllerPtr controller) {
    uint32_t buttons = 0;
    const uint8_t dpad = controller->dpad();

    if (controller->a()) buttons |= BUTTON_CROSS;
    if (controller->b()) buttons |= BUTTON_CIRCLE;
    if (controller->x()) buttons |= BUTTON_SQUARE;
    if (controller->y()) buttons |= BUTTON_TRIANGLE;
    if (controller->l1()) buttons |= BUTTON_L1;
    if (controller->r1()) buttons |= BUTTON_R1;
    if (controller->l2()) buttons |= BUTTON_L2;
    if (controller->r2()) buttons |= BUTTON_R2;
    if (controller->thumbL()) buttons |= BUTTON_L3;
    if (controller->thumbR()) buttons |= BUTTON_R3;
    if (controller->miscSelect()) buttons |= BUTTON_SHARE;
    if (controller->miscStart()) buttons |= BUTTON_OPTIONS;
    if (controller->miscSystem()) buttons |= BUTTON_PS;
    if (controller->miscCapture()) buttons |= BUTTON_TOUCHPAD;
    if (dpad & DPAD_UP) buttons |= BUTTON_DPAD_UP;
    if (dpad & DPAD_DOWN) buttons |= BUTTON_DPAD_DOWN;
    if (dpad & DPAD_LEFT) buttons |= BUTTON_DPAD_LEFT;
    if (dpad & DPAD_RIGHT) buttons |= BUTTON_DPAD_RIGHT;

    return buttons;
}

void resetControllerMac() {
    if (!preferencesReady) {
        Serial.println("[PS4] ERROR: NVS Preferences is unavailable; MAC reset failed.");
        return;
    }

    if (preferences.isKey(kMacKey) && !preferences.remove(kMacKey)) {
        Serial.println("[PS4] ERROR: Failed to clear controller MAC from NVS.");
        return;
    }

    storageReady = true;
    memset(savedMac, 0, sizeof(savedMac));
    savedMacValid = false;
    learnControllerMac = true;
    Serial.println("[PS4] Saved controller MAC cleared; next controller will be saved.");

    if (activeController != nullptr && activeController->isConnected()) {
        Serial.println("[PS4] Disconnecting controller. Pair the controller to save its MAC again.");
        activeController->disconnect();
    }
}

void pollMacResetButton() {
    if (macResetPin < 0) {
        return;
    }

    const bool rawState = digitalRead(static_cast<uint8_t>(macResetPin)) == HIGH;
    const uint32_t now = millis();

    if (rawState != resetButtonRawState) {
        resetButtonRawState = rawState;
        resetButtonLastChange = now;
    }

    if (now - resetButtonLastChange >= kButtonDebounceMs &&
        resetButtonStableState != resetButtonRawState) {
        resetButtonStableState = resetButtonRawState;
        if (resetButtonStableState) {
            resetControllerMac();
        }
    }
}

}  // namespace

bool begin(int8_t builtinLedPin, int8_t controllerMacResetPin) {
    if (started) {
        Serial.println("[PS4] ERROR: begin() may only be called once.");
        return false;
    }
    started = true;
    ledPin = builtinLedPin;
    macResetPin = controllerMacResetPin;

    if (ledPin >= 0) {
        pinMode(static_cast<uint8_t>(ledPin), OUTPUT);
        setLed(false);
    }

    if (macResetPin >= 0) {
        pinMode(static_cast<uint8_t>(macResetPin), INPUT_PULLDOWN);
        resetButtonRawState = digitalRead(static_cast<uint8_t>(macResetPin)) == HIGH;
        resetButtonStableState = resetButtonRawState;
        resetButtonLastChange = millis();
    }

    preferencesReady = preferences.begin(kPreferencesNamespace, false);
    storageReady = preferencesReady;
    if (!preferencesReady) {
        Serial.println("[PS4] ERROR: Could not open NVS Preferences; controller connections will be rejected.");
    } else if (preferences.isKey(kMacKey)) {
        const size_t storedLength = preferences.getBytesLength(kMacKey);
        if (storedLength != kBluetoothAddressSize ||
            preferences.getBytes(kMacKey, savedMac, kBluetoothAddressSize) != kBluetoothAddressSize) {
            Serial.println("[PS4] ERROR: Saved controller MAC is invalid; press the reset button to pair again.");
            storageReady = false;
        } else {
            savedMacValid = true;
            Serial.print("[PS4] Saved controller MAC: ");
            printAddress(savedMac);
            Serial.println();
        }
    } else {
        learnControllerMac = true;
        Serial.println("[PS4] No saved controller MAC; first controller will be saved.");
    }

    if (preferencesReady && macResetPin >= 0 && resetButtonStableState) {
        resetControllerMac();
    }

    BP32.setup(&onControllerConnected, &onControllerDisconnected);
    BP32.enableVirtualDevice(false);
    Serial.println("[PS4] Waiting for a controller. Hold SHARE + PS to pair.");
    return storageReady;
}

void update() {
    if (!started) {
        return;
    }
    pollMacResetButton();
    BP32.update();
}

bool isConnected() {
    return activeController != nullptr && activeController->isConnected();
}

bool getData(Data& data) {
    if (!isConnected() || !activeController->hasData()) {
        return false;
    }

    if (!activeController->isGamepad()) {
        disconnectController(activeController, "device is not a gamepad");
        return false;
    }

    if (learnControllerMac && !savedMacValid && !saveControllerMac(activeController)) {
        activeController->disconnect();
        return false;
    }

    data.buttons = readButtons(activeController);
    data.leftX = axisPercent(activeController->axisX(), false);
    data.leftY = axisPercent(activeController->axisY(), true);
    data.rightX = axisPercent(activeController->axisRX(), false);
    data.rightY = axisPercent(activeController->axisRY(), true);
    data.l2 = triggerPercent(activeController->brake());
    data.r2 = triggerPercent(activeController->throttle());
    return true;
}

}  // namespace ps4
