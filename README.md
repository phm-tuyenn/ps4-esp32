# PS4 Bluepad32 Controller API

ESP32 dùng Bluepad32 để kết nối tay cầm DualShock 4. API được đóng gói thành
ESP-IDF component trong `components/PS4Bluepad32`; ví dụ đang được build nằm
trong `src/main.cpp`. Firmware không tạo Wi-Fi AP và không chạy web server.

## Build và nạp firmware

Dự án dùng tích hợp Bluepad32 với ESP-IDF + Arduino, yêu cầu ESP-IDF 5.5 trở lên
và Python 3.10–3.13 cho PlatformIO. Trước lần build đầu tiên, tải đúng phiên bản
BTstack mà Bluepad32 sử dụng, sau đó chạy script tải các source đã ghim phiên
bản cho Bluepad32 và Arduino:

```sh
git clone https://github.com/bluekitchen/btstack.git external/btstack
git -C external/btstack checkout 8a8b616e04b07612a273978131efad32a083b1ed
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-dependencies.ps1
```

Build, nạp và mở serial monitor ở tốc độ 115200:

```sh
pio run -e esp32doit-devkit-v1
pio run -e esp32doit-devkit-v1 -t upload
pio device monitor -b 115200
```

## API tay cầm

Trong chương trình của bạn, include `PS4Bluepad32.h` và gọi API như sau:

```cpp
#include <PS4Bluepad32.h>

void setup() {
    ps4::begin();
}

void loop() {
    ps4::update();  // Gọi liên tục trong loop để xử lý Bluetooth.

    if (ps4::isConnected()) {
        ps4::Data data;
        if (ps4::getData(data)) {  // true khi vừa nhận được report mới
            if (data.buttons & ps4::BUTTON_CROSS) {
                // Cross đang được nhấn
            }

            int8_t leftX = data.leftX;   // -100..100 %
            int8_t leftY = data.leftY;   // -100..100 %, dương hướng lên
            int8_t rightX = data.rightX;
            int8_t rightY = data.rightY;
            uint8_t l2 = data.l2;        // 0..100 %
            uint8_t r2 = data.r2;        // 0..100 %
            (void)leftX;
            (void)leftY;
            (void)rightX;
            (void)rightY;
            (void)l2;
            (void)r2;
        }
    }
}
```

`ps4::begin()` khởi tạo Bluepad32, bật quét kết nối, cấu hình LED và nút reset
MAC. `ps4::update()` phải được gọi liên tục. `ps4::isConnected()` kiểm tra kết
nối hiện tại. `ps4::getData(data)` chép dữ liệu mới nhất vào `data` và trả về
`true` chỉ khi có report mới trong lượt update đó.

### Dùng component trong project ESP-IDF khác

Tải thư mục `components/PS4Bluepad32` từ repository này vào thư mục
`components/PS4Bluepad32` của project. Project cần tích hợp sẵn các ESP-IDF
components `bluepad32`, `bluepad32_arduino`, `arduino` và `btstack`; component
này khai báo chúng làm dependency.

Trong `main/CMakeLists.txt` của project, thêm `PS4Bluepad32` vào `REQUIRES`:

```cmake
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES PS4Bluepad32
)
```

Sau đó include `<PS4Bluepad32.h>` trong mã nguồn. Hướng dẫn cài đặt và ví dụ
ngắn cũng có trong `components/PS4Bluepad32/README.md`.

`data.buttons` là bitmask; các bit được định nghĩa bằng `ps4::BUTTON_CROSS`,
`BUTTON_CIRCLE`, `BUTTON_SQUARE`, `BUTTON_TRIANGLE`, `BUTTON_L1`, `BUTTON_R1`,
`BUTTON_L2`, `BUTTON_R2`, `BUTTON_L3`, `BUTTON_R3`, `BUTTON_SHARE`,
`BUTTON_OPTIONS`, `BUTTON_PS`, `BUTTON_TOUCHPAD`, và `BUTTON_DPAD_UP/DOWN/LEFT/RIGHT`.
Joystick trả về -100%..100% với vùng chết nhỏ ở giữa; cò L2/R2 trả về 0%..100%.
LED tích hợp trên GPIO2 sáng khi tay cầm kết nối và tắt khi ngắt kết nối.

`src/main.cpp` là ví dụ hoạt động: in các nút mới nhấn cùng giá trị joystick và
L2/R2 ra Serial Monitor ở 115200 baud.

## Dùng component trong project khác

Hướng dẫn chi tiết về cài component và API nằm tại
[`components/PS4Bluepad32/README.md`](./components/PS4Bluepad32/README.md).
Đây là ESP-IDF component dành cho project PlatformIO dùng `framework = espidf`.

## Thư viện Arduino IDE

Thư viện Arduino IDE riêng nằm tại
[`arduino-library/PS4Bluepad32`](./arduino-library/PS4Bluepad32). Cài board
package **ESP32 + Bluepad32**, sau đó chép thư mục `PS4Bluepad32` vào
`Documents\Arduino\libraries\PS4Bluepad32`. Hướng dẫn từng bước và ví dụ nằm
trong [README của Arduino library](./arduino-library/PS4Bluepad32/README.md).
Đây là thư viện Arduino thật với `library.properties`, nhưng chưa được phát
hành trên Arduino Library Manager.

## Kết nối tay cầm

1. Mở serial monitor.
2. Giữ **SHARE + PS** trên tay cầm DualShock 4 đến khi đèn nhấp nháy.
3. Chờ dòng `[PS4] Connected`, sau đó nhấn nút hoặc di chuyển cần analog.

## Dùng nhiều ESP32 và tay cầm

Mỗi ESP32 tự lưu MAC của tay cầm đầu tiên kết nối đến nó và bật allowlist
Bluepad32. MAC được lưu riêng trên từng ESP32 và giữ lại sau khi mất nguồn, nên
những lần kết nối sau chỉ nhận tay cầm đã lưu. Nếu cài nhiều bộ cùng lúc, chỉ
bật chế độ ghép đôi trên tay cầm dự định ghép với ESP32 đó trong lần kết nối
đầu tiên.

MAC được lưu trong NVS nên vẫn còn sau khi rút nguồn và cắm ESP32 lại. Để xóa
MAC và học lại, nối nút nhấn thường mở giữa **GPIO27 và 3V3**, sau đó nhấn nút.
Firmware khử dội nút và reset một lần cho mỗi lần nhấn; không cần khởi động lại.
ESP32 sẽ ngắt tay cầm hiện tại nếu có, rồi lưu MAC của tay cầm tiếp theo kết nối.
Tắt tay cầm cũ trước khi reset nếu muốn ghép một tay cầm khác; sau đó giữ
**SHARE + PS** trên tay cầm mới để đưa nó vào chế độ ghép đôi.

GPIO27 dùng `INPUT_PULLDOWN`, vì vậy nút nối chân GPIO27 lên 3V3 khi nhấn.
Không nối GPIO27 với 5V. Giữ nút trong lúc bật nguồn cũng xóa MAC. Chức năng
reset chỉ xóa MAC allowlist, không xóa khóa Bluetooth đã ghép trước đó; hãy tắt
các tay cầm khác trong lúc học MAC mới để tránh ESP32 lưu nhầm tay cầm.
