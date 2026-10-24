// SPDX-License-Identifier: GPL-3.0-or-later
#include <PS4Bluepad32.h>

namespace {

ps4::Data currentData = {};
ps4::Data previousData = {};

struct ButtonLog {
    uint32_t mask;
    const char* name;
};

constexpr ButtonLog kButtonLogs[] = {
    {ps4::BUTTON_CROSS, "Cross (X)"},
    {ps4::BUTTON_CIRCLE, "Circle (O)"},
    {ps4::BUTTON_SQUARE, "Square"},
    {ps4::BUTTON_TRIANGLE, "Triangle"},
    {ps4::BUTTON_L1, "L1"},
    {ps4::BUTTON_R1, "R1"},
    {ps4::BUTTON_L2, "L2"},
    {ps4::BUTTON_R2, "R2"},
    {ps4::BUTTON_L3, "L3"},
    {ps4::BUTTON_R3, "R3"},
    {ps4::BUTTON_SHARE, "Share"},
    {ps4::BUTTON_OPTIONS, "Options"},
    {ps4::BUTTON_PS, "PS"},
    {ps4::BUTTON_TOUCHPAD, "Touchpad"},
    {ps4::BUTTON_DPAD_UP, "D-pad Up"},
    {ps4::BUTTON_DPAD_DOWN, "D-pad Down"},
    {ps4::BUTTON_DPAD_LEFT, "D-pad Left"},
    {ps4::BUTTON_DPAD_RIGHT, "D-pad Right"},
};

void printNewlyPressedButtons(uint32_t buttons) {
    for (const ButtonLog& button : kButtonLogs) {
        if ((buttons & button.mask) != 0 && (previousData.buttons & button.mask) == 0) {
            Serial.printf("[PS4] Pressed: %s\n", button.name);
        }
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    if (!ps4::begin()) {
        Serial.println("PS4Bluepad32 could not initialize NVS or validate its saved MAC.");
    }
    Serial.println("Send 'r' in the Serial Monitor to forget the saved controller.");
}

void loop() {
    if (Serial.available() > 0) {
        const char command = static_cast<char>(Serial.read());
        if (command == 'r' || command == 'R') {
            if (!ps4::resetMac()) {
                Serial.println("Could not reset the saved controller MAC.");
            }
        }
    }

    ps4::update();

    if (!ps4::isConnected()) {
        previousData = {};
        delay(1);
        return;
    }

    if (ps4::getData(currentData)) {
        printNewlyPressedButtons(currentData.buttons);

        if (currentData.leftX != previousData.leftX || currentData.leftY != previousData.leftY) {
            Serial.printf("[PS4] Left stick: X=%d%% Y=%d%%\n", currentData.leftX, currentData.leftY);
        }
        if (currentData.rightX != previousData.rightX || currentData.rightY != previousData.rightY) {
            Serial.printf("[PS4] Right stick: X=%d%% Y=%d%%\n", currentData.rightX, currentData.rightY);
        }
        if (currentData.l2 != previousData.l2 || currentData.r2 != previousData.r2) {
            Serial.printf("[PS4] Triggers: L2=%u%% R2=%u%%\n", currentData.l2, currentData.r2);
        }

        previousData = currentData;
    }

    delay(1);
}
