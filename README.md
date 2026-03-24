Spinix is an OSS bike computer that sits near the wheel and tracks your rides without manual intervention. It uses a MagnicLight frictionless generator to charge its battery.

# Components
- **SpinTX**: the main module that sits near the back wheel and records every move
- **SpinRX**: a module that sits on the handlebars and displays info from SpinTX
- **SpinMobile**: the mobile app that will collect and process all the SpinTX data and post the rides to Strava


# Features
## SpinTX
- GPS
- Speed
- Cadence
- Temperature
- Humidity
- Barometer
- Red light
- MagnicLight generator

## SpinRX
- Receives data from SpinTX
- Shows all the data on a display

## SpinMobile
- Receives data from SpinTX
- Process the data and creates rides
- Save the rides to Strava
- Save sonfigurations to SpinTX


# Getting started

## SpinTX/SpinRX
- install esp-idf
- get_idf (bring idf.py command to this folder)
- idf.py set-target esp32c3 (without this, the compiler won't know your device is an esp32-c3)
- idf.py menuconfig (optional configs? BLE nimble needs to be enabled)
- idf.py add-dependency esp-idf-lib/aht (install component)
- idf.py build flash monitor (all at once or 3 separated commands)

## SpinMobile
- Install Xcode
- It should work right away to compile, dependencies are downloaded automatically through SPM
