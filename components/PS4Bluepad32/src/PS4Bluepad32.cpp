#include "PS4Bluepad32.h"

#include <Arduino.h>
#include <Bluepad32.h>
#include <bt/uni_bt_allowlist.h>

namespace ps4 {
namespace {

constexpr uint8_t kBuiltinLedPin = 2;
constexpr uint8_t kMacResetPin = 27;
constexpr uint32_t kButtonDebounceMs = 40;

ControllerPtr activeController = nullptr;
bool learnControllerMac = false;
bool resetButtonRawState = false;
bool resetButtonStableState = false;
uint32_t resetButtonLastChange = 0;

bool allowlistHasAddresses() {
    const bd_addr_t* addresses = nullptr;
    int total = 0;
    const bd_addr_t zeroAddress = {0, 0, 0, 0, 0, 0};

    uni_bt_allowlist_get_all(&addresses, &total);
    for (int i = 0; i < total; ++i) {
        if (bd_addr_cmp(addresses[i], zeroAddress) != 0) {
            return true;
        }
    }
    return false;
}

void resetControllerMac() {
    uni_bt_allowlist_set_enabled(false);
    if (!uni_bt_allowlist_remove_all()) {
        Console.println("[PS4] ERROR: Could not clear saved controller MAC.");
        return;
    }

    learnControllerMac = true;
    Console.println("[PS4] Saved controller MAC cleared; next controller will be saved.");

    if (activeController != nullptr && activeController->isConnected()) {
        Console.println("[PS4] Disconnecting controller. Pair the controller to save its MAC again.");
        activeController->disconnect();
    }
}

void learnAndLockControllerMac(ControllerPtr controller) {
    const ControllerProperties properties = controller->getProperties();
    bd_addr_t address;
    memcpy(address, properties.btaddr, sizeof(address));

    const bd_addr_t* addresses = nullptr;
    int total = 0;
    bool alreadyAllowed = false;
    uni_bt_allowlist_get_all(&addresses, &total);
    for (int i = 0; i < total; ++i) {
        if (bd_addr_cmp(addresses[i], address) == 0) {
            alreadyAllowed = true;
            break;
        }
    }

    if (!alreadyAllowed && !uni_bt_allowlist_add_addr(address)) {
        Console.printf("[PS4] ERROR: Could not save controller MAC %s\n", bd_addr_to_str(address));
        return;
    }

    uni_bt_allowlist_set_enabled(true);
    learnControllerMac = false;
    Console.printf("[PS4] Saved controller MAC: %s\n", bd_addr_to_str(address));
    Console.println("[PS4] Bluetooth allowlist enabled.");
}

void onControllerConnected(ControllerPtr controller) {
    if (activeController != nullptr) {
        return;
    }

    activeController = controller;
    digitalWrite(kBuiltinLedPin, HIGH);
    Console.printf("[PS4] Connected: %s\n", controller->getModelName().c_str());
    if (learnControllerMac) {
        learnAndLockControllerMac(controller);
    }
    Console.println("[PS4] Controller ready.");
}

void onControllerDisconnected(ControllerPtr controller) {
    if (activeController != controller) {
        return;
    }

    activeController = nullptr;
    digitalWrite(kBuiltinLedPin, LOW);
    Console.println("[PS4] Controller disconnected");
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

void pollMacResetButton() {
    const bool rawState = digitalRead(kMacResetPin) == HIGH;
    const uint32_t now = millis();

    if (rawState != resetButtonRawState) {
        resetButtonRawState = rawState;
        resetButtonLastChange = now;
    }

    if (now - resetButtonLastChange >= kButtonDebounceMs && resetButtonStableState != resetButtonRawState) {
        resetButtonStableState = resetButtonRawState;
        if (resetButtonStableState) {
            resetControllerMac();
        }
    }
}

}  // namespace

void begin() {
    pinMode(kBuiltinLedPin, OUTPUT);
    digitalWrite(kBuiltinLedPin, LOW);
    pinMode(kMacResetPin, INPUT_PULLDOWN);

    resetButtonRawState = digitalRead(kMacResetPin) == HIGH;
    resetButtonStableState = resetButtonRawState;
    resetButtonLastChange = millis();

    if (resetButtonStableState) {
        resetControllerMac();
    } else if (uni_bt_allowlist_is_enabled() || allowlistHasAddresses()) {
        uni_bt_allowlist_set_enabled(true);
        Console.println("[PS4] Saved controller MAC found; allowlist enabled.");
    } else {
        learnControllerMac = true;
        Console.println("[PS4] No saved controller MAC; first connected controller will be saved.");
    }

    BP32.setup(&onControllerConnected, &onControllerDisconnected);
    BP32.enableVirtualDevice(false);
    Console.println("[PS4] Waiting for a controller. Hold SHARE + PS to pair.");
}

void update() {
    pollMacResetButton();
    BP32.update();
}

bool isConnected() {
    return activeController != nullptr && activeController->isConnected();
}

bool getData(Data& data) {
    if (!isConnected() || !activeController->hasData() || !activeController->isGamepad()) {
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
