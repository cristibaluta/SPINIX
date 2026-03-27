Spinix is an OSS bike computer that sits near the wheel and tracks your rides without manual intervention. It uses a MagnicLight frictionless generator to charge its battery.

# Components
- **SpinTX**: the main module that sits near the back wheel and records every move
- **SpinRX**: an optional module that sits on the handlebars and displays info from SpinTX
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
- Save settings to SpinTX (wheel diameter)
