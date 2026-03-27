# SpinRX


## Getting started
- install esp-idf v6
- source "~/.espressif/v6.0/esp-idf/export.sh" (activates idf.py command in this session)
- idf.py set-target esp32c3 (without this, the compiler won't know your device is an esp32-c3)
- idf.py menuconfig (activate BLE nimble)
- idf.py add-dependency esp-idf-lib/aht (install 3rd party components, )
- idf.py build flash monitor (all at once or 3 separated commands)
