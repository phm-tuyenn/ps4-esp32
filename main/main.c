#include "sdkconfig.h"

#include <stddef.h>
#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <btstack_stdio_esp32.h>
#include <arduino_platform.h>
#include <uni.h>

int app_main(void) {
#ifndef CONFIG_ESP_CONSOLE_UART_NONE
#ifndef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    btstack_stdio_init();
#endif
#endif

    btstack_init();
    uni_platform_set_custom(get_arduino_platform());
    uni_init(0, NULL);
    btstack_run_loop_execute();
    return 0;
}
