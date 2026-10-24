#include <Arduino.h>
#include <Bluepad32.h>
#include <bt/uni_bt_allowlist.h>

namespace {

constexpr uint8_t kBuiltinLedPin = 2;
constexpr uint8_t kMacResetPin = 27;

ControllerPtr activeController = nullptr;
uint32_t previousButtons = 0;
uint16_t previousRawButtons = 0;
uint16_t previousRawMiscButtons = 0;
uint8_t previousRawDpad = 0;
int previousLeftX = -101;
int previousLeftY = -101;
int previousRightX = -101;
int previousRightY = -101;
int previousL2 = -1;
int previousR2 = -1;
bool learnControllerMac = false;
bool resetButtonRawState = false;
bool resetButtonStableState = false;
uint32_t resetButtonLastChange = 0;

enum ButtonMask : uint32_t {
    kCross = 1u << 0,
    kCircle = 1u << 1,
    kSquare = 1u << 2,
    kTriangle = 1u << 3,
    kL1 = 1u << 4,
    kR1 = 1u << 5,
    kL2 = 1u << 6,
    kR2 = 1u << 7,
    kL3 = 1u << 8,
    kR3 = 1u << 9,
    kShare = 1u << 10,
    kOptions = 1u << 11,
    kPs = 1u << 12,
    kTouchpad = 1u << 13,
    kDpadUp = 1u << 14,
    kDpadDown = 1u << 15,
    kDpadLeft = 1u << 16,
    kDpadRight = 1u << 17,
};

struct ButtonName {
    uint32_t mask;
    const char* name;
};

constexpr ButtonName kButtonNames[] = {
    {kCross, "Cross (X)"},
    {kCircle, "Circle (O)"},
    {kSquare, "Square"},
    {kTriangle, "Triangle"},
    {kL1, "L1"},
    {kR1, "R1"},
    {kL2, "L2"},
    {kR2, "R2"},
    {kL3, "L3"},
    {kR3, "R3"},
    {kShare, "Share"},
    {kOptions, "Options"},
    {kPs, "PS"},
    {kTouchpad, "Touchpad"},
    {kDpadUp, "D-pad Up"},
    {kDpadDown, "D-pad Down"},
    {kDpadLeft, "D-pad Left"},
    {kDpadRight, "D-pad Right"},
};

uint32_t readPressedButtons(ControllerPtr controller) {
    uint32_t pressed = 0;
    const uint8_t dpad = controller->dpad();

    if (controller->a()) pressed |= kCross;
    if (controller->b()) pressed |= kCircle;
    if (controller->x()) pressed |= kSquare;
    if (controller->y()) pressed |= kTriangle;
    if (controller->l1()) pressed |= kL1;
    if (controller->r1()) pressed |= kR1;
    if (controller->l2()) pressed |= kL2;
    if (controller->r2()) pressed |= kR2;
    if (controller->thumbL()) pressed |= kL3;
    if (controller->thumbR()) pressed |= kR3;
    if (controller->miscSelect()) pressed |= kShare;
    if (controller->miscStart()) pressed |= kOptions;
    if (controller->miscSystem()) pressed |= kPs;
    if (controller->miscCapture()) pressed |= kTouchpad;
    if (dpad & DPAD_UP) pressed |= kDpadUp;
    if (dpad & DPAD_DOWN) pressed |= kDpadDown;
    if (dpad & DPAD_LEFT) pressed |= kDpadLeft;
    if (dpad & DPAD_RIGHT) pressed |= kDpadRight;

    return pressed;
}

int axisPercent(int32_t value, bool invert) {
    if (value > 512) value = 512;
    if (value < -512) value = -512;
    if (invert) value = -value;

    int percent = value >= 0 ? (value * 100 + 256) / 512 : (value * 100 - 256) / 512;
    if (percent >= -3 && percent <= 3) percent = 0;
    return percent;
}

int triggerPercent(int32_t value) {
    if (value < 0) value = 0;
    if (value > 1020) value = 1020;
    return (value * 100 + 510) / 1020;
}

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
    if (activeController == nullptr) {
        activeController = controller;
        previousButtons = 0;
        previousRawButtons = 0;
        previousRawMiscButtons = 0;
        previousRawDpad = 0;
        previousLeftX = -101;
        previousLeftY = -101;
        previousRightX = -101;
        previousRightY = -101;
        previousL2 = -1;
        previousR2 = -1;
        digitalWrite(kBuiltinLedPin, HIGH);
        Console.printf("[PS4] Connected: %s\n", controller->getModelName().c_str());
        if (learnControllerMac) {
            learnAndLockControllerMac(controller);
        }
        Console.println("[PS4] Press buttons to see them here.");
    }
}

void onControllerDisconnected(ControllerPtr controller) {
    if (activeController == controller) {
        activeController = nullptr;
        previousButtons = 0;
        previousRawButtons = 0;
        previousRawMiscButtons = 0;
        previousRawDpad = 0;
        previousLeftX = -101;
        previousLeftY = -101;
        previousRightX = -101;
        previousRightY = -101;
        previousL2 = -1;
        previousR2 = -1;
        digitalWrite(kBuiltinLedPin, LOW);
        Console.println("[PS4] Controller disconnected");
    }
}

void logControllerInput() {
    if (activeController == nullptr || !activeController->isConnected() || !activeController->hasData()) {
        return;
    }

    if (!activeController->isGamepad()) {
        return;
    }

    const uint32_t pressed = readPressedButtons(activeController);
    const uint16_t rawButtons = activeController->buttons();
    const uint16_t rawMiscButtons = activeController->miscButtons();
    const uint8_t rawDpad = activeController->dpad();
    const int leftX = axisPercent(activeController->axisX(), false);
    const int leftY = axisPercent(activeController->axisY(), true);
    const int rightX = axisPercent(activeController->axisRX(), false);
    const int rightY = axisPercent(activeController->axisRY(), true);
    const int l2 = triggerPercent(activeController->brake());
    const int r2 = triggerPercent(activeController->throttle());

    if (rawButtons != previousRawButtons || rawMiscButtons != previousRawMiscButtons || rawDpad != previousRawDpad) {
        Console.printf("[PS4] Input: buttons=0x%04X misc=0x%04X dpad=0x%02X\n",
                       rawButtons, rawMiscButtons, rawDpad);
    }

    if (leftX != previousLeftX || leftY != previousLeftY) {
        Console.printf("[PS4] Left stick: X=%d%% Y=%d%%\n", leftX, leftY);
    }
    if (rightX != previousRightX || rightY != previousRightY) {
        Console.printf("[PS4] Right stick: X=%d%% Y=%d%%\n", rightX, rightY);
    }
    if (l2 != previousL2 || r2 != previousR2) {
        Console.printf("[PS4] Triggers: L2=%d%% R2=%d%%\n", l2, r2);
    }

    for (const ButtonName& button : kButtonNames) {
        if ((pressed & button.mask) != 0 && (previousButtons & button.mask) == 0) {
            Console.printf("[PS4] Pressed: %s\n", button.name);
        }
    }
    previousButtons = pressed;
    previousRawButtons = rawButtons;
    previousRawMiscButtons = rawMiscButtons;
    previousRawDpad = rawDpad;
    previousLeftX = leftX;
    previousLeftY = leftY;
    previousRightX = rightX;
    previousRightY = rightY;
    previousL2 = l2;
    previousR2 = r2;
}

void pollMacResetButton() {
    const bool rawState = digitalRead(kMacResetPin) == HIGH;
    const uint32_t now = millis();

    if (rawState != resetButtonRawState) {
        resetButtonRawState = rawState;
        resetButtonLastChange = now;
    }

    if (now - resetButtonLastChange >= 40 && resetButtonStableState != resetButtonRawState) {
        resetButtonStableState = resetButtonRawState;
        if (resetButtonStableState) {
            resetControllerMac();
        }
    }
}

}  // namespace

void setup() {
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

void loop() {
    pollMacResetButton();
    BP32.update();
    logControllerInput();
    delay(1);
}
