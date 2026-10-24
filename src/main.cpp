#include <Arduino.h>
#include <Bluepad32.h>

#include <PS4Bluepad32.h>

namespace {

ps4::Data controllerData = {};
ps4::Data previousData = {};
bool hasPreviousData = false;

void logButtonPresses(uint32_t buttons) {
    struct ButtonLog {
        uint32_t mask;
        const char* name;
    };

    constexpr ButtonLog buttonLogs[] = {
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

    for (const ButtonLog& button : buttonLogs) {
        if ((buttons & button.mask) != 0 && (previousData.buttons & button.mask) == 0) {
            Console.printf("[PS4] Pressed: %s\n", button.name);
        }
    }
}

}  // namespace

void setup() {
    ps4::begin();
}

void loop() {
    ps4::update();

    if (!ps4::isConnected()) {
        hasPreviousData = false;
        previousData = {};
        delay(1);
        return;
    }

    if (ps4::getData(controllerData)) {
        logButtonPresses(controllerData.buttons);

        if (!hasPreviousData ||
            controllerData.leftX != previousData.leftX ||
            controllerData.leftY != previousData.leftY) {
            Console.printf("[PS4] Left stick: X=%d%% Y=%d%%\n",
                           controllerData.leftX, controllerData.leftY);
        }
        if (!hasPreviousData ||
            controllerData.rightX != previousData.rightX ||
            controllerData.rightY != previousData.rightY) {
            Console.printf("[PS4] Right stick: X=%d%% Y=%d%%\n",
                           controllerData.rightX, controllerData.rightY);
        }
        if (!hasPreviousData ||
            controllerData.l2 != previousData.l2 ||
            controllerData.r2 != previousData.r2) {
            Console.printf("[PS4] Triggers: L2=%u%% R2=%u%%\n",
                           controllerData.l2, controllerData.r2);
        }

        previousData = controllerData;
        hasPreviousData = true;
    }

    delay(1);
}
