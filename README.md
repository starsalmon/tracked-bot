# Robot - pioarduino / NodeMCU-32S

Converted from the original ESP32 Arduino project.

## Hardware

- NodeMCU-32S / classic ESP32
- DRV8833 dual motor driver
- PS4 controller
- MPU6050 (currently disabled)
- SSD1306 128x32 OLED (currently disabled)

## Current pin assignment

### DRV8833

| ESP32 | DRV8833 |
|---:|---|
| GPIO 27 | AIN1 |
| GPIO 26 | AIN2 |
| GPIO 32 | BIN1 |
| GPIO 33 | BIN2 |

The DRV8833 is driven using PWM directly on IN1/IN2. There are no separate motor-enable pins.

Make sure the DRV8833 nSLEEP input is held HIGH according to the breakout board's wiring. If your particular board exposes nSLEEP without a pull-up, it will need to be connected to 3.3 V or assigned a GPIO.

## Optional hardware switches

In `src/main.cpp`:

```cpp
#define MPU_ENABLED 0
#define OLED_ENABLED 0
```

Set either to `1` when the corresponding hardware is connected.

The libraries remain in `platformio.ini` and the code remains in the project. With both set to `0`, the ESP32 will not attempt to initialise either device.

## PS4

The original controller pairing address is retained:

`48:b0:2d:37:2d:4c`

If the controller was paired to a different ESP32 Bluetooth address, this may need to be changed.

## Important conversion notes

1. The old L293D-style `enableA/enableB` PWM arrangement has been replaced by DRV8833 IN1/IN2 PWM.
2. The old ESP32 `ledcSetup()` / `ledcAttachPin()` API has been replaced with the current pin-based LEDC API (`ledcAttach()` / `ledcWrite()`).
3. The original differential-steering condition was inverted, meaning the calculation normally did not run. It has been corrected to the intended valid-range test.
4. The original motor dead-band compensation (minimum PWM 150/255) has been retained.
5. A controller disconnect now immediately stops the motors.
