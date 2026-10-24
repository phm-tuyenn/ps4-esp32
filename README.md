# PS4 Bluepad32 Serial Monitor

ESP32 dùng Bluepad32 để kết nối tay cầm DualShock 4 và in các nút vừa nhấn ra
serial monitor. Firmware không tạo Wi-Fi AP và không chạy web server.

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

## Kết nối tay cầm

1. Mở serial monitor.
2. Giữ **SHARE + PS** trên tay cầm DualShock 4 đến khi đèn nhấp nháy.
3. Khi serial monitor báo tay cầm đã kết nối, nhấn các nút. Mỗi lần nhấn sẽ
   được in một dòng, ví dụ `[PS4] Pressed: Cross (X)`.

Firmware log các nút △ ○ × □, D-pad, Share, Options, PS, touchpad, L1/R1/L2/R2
và L3/R3. Nút được log khi chuyển từ nhả sang nhấn; giữ nút không tạo log lặp.
Hai joystick được log theo X/Y từ -100% đến 100%; trục Y dương hướng lên.
L2/R2 được log từ 0% đến 100%. Giá trị analog được log khi phần trăm thay đổi,
với vùng chết nhỏ ở giữa joystick để tránh nhiễu khi cần ở vị trí nghỉ.
LED tích hợp trên GPIO2 sáng khi tay cầm kết nối và tắt khi ngắt kết nối.
Nếu có dữ liệu thô từ tay cầm nhưng không nhận diện được nút, serial monitor
cũng in các mask `buttons`, `misc` và `dpad` khi chúng thay đổi.

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
