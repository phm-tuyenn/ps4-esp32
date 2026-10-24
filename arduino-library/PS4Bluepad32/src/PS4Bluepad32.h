// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef PS4_BLUEPAD32_H
#define PS4_BLUEPAD32_H

#include <stdint.h>

namespace ps4 {

enum Button : uint32_t {
    BUTTON_CROSS = 1u << 0,
    BUTTON_CIRCLE = 1u << 1,
    BUTTON_SQUARE = 1u << 2,
    BUTTON_TRIANGLE = 1u << 3,
    BUTTON_L1 = 1u << 4,
    BUTTON_R1 = 1u << 5,
    BUTTON_L2 = 1u << 6,
    BUTTON_R2 = 1u << 7,
    BUTTON_L3 = 1u << 8,
    BUTTON_R3 = 1u << 9,
    BUTTON_SHARE = 1u << 10,
    BUTTON_OPTIONS = 1u << 11,
    BUTTON_PS = 1u << 12,
    BUTTON_TOUCHPAD = 1u << 13,
    BUTTON_DPAD_UP = 1u << 14,
    BUTTON_DPAD_DOWN = 1u << 15,
    BUTTON_DPAD_LEFT = 1u << 16,
    BUTTON_DPAD_RIGHT = 1u << 17,
};

struct Data {
    uint32_t buttons;
    int8_t leftX;
    int8_t leftY;
    int8_t rightX;
    int8_t rightY;
    uint8_t l2;
    uint8_t r2;
};

bool begin(int8_t ledPin = 2);
void update();
bool isConnected();
bool getData(Data& data);
bool resetMac();

}  // namespace ps4

#endif  // PS4_BLUEPAD32_H
