# Tracked bot wiring (Tracker, bot2)

Board: **Unexpected Maker TinyS3[D]** (ESP32-S3). **Source of truth:** `include/tracked_pins_s3.h` (force-included for env `tracked_s3_usb` / `tracked_s3_ota`, `-DTRACKED_PINS_S3`). Defaults for everything else are in `include/tracked_pins.h`. The pin map keeps the same physical header positions as the older TinyC6 harness (header position noted below).

Power: one **1S 800 mAh LiPo** into a boost converter; the 5 V output feeds the TinyS3 **5 V** pin and the DRV8833 **VM**. Tracks are two **130-size** DC motors on a **DRV8833**.

The old NodeMCU-32S sketch is still `nodemcu-32s` (PS4 / Classic BT only). Fleet firmware for Tracker is TinyS3[D] over Wi-Fi + micro-ROS.

---

## Quick pin map (TinyS3[D])

| GPIO | Header pos | Function | Notes |
|------|-----------|----------|-------|
| **4** | 8 | **Solar tilt servo** | Signal, LEDC channel 7 |
| **3** | 7 | **IR RX** | TSOP / 38 kHz demod (fleet peer ID); S3 strap pin, used as input |
| **5** | 9 | **Side IR R** | Digital, active LOW, pull-up |
| **21** | 10 | DRV8833 **AIN1** | Left track |
| **0** | 11 | DRV8833 **AIN2** | Left track. S3 **BOOT** strap, pulled high at reset |
| **44** | 12 | DRV8833 **BIN1** | Right track. UART0 **RX** |
| **43** | 13 | DRV8833 **BIN2** | Right track. UART0 **TX** (ROM boot log toggles it) |
| **6** | 16 | **Side IR L** | Digital, active LOW, pull-up |
| **7** | 17 | **Speaker** | Passive piezo (LEDC channel 5) |
| **8** | 18 | I2C **SDA** | OLED, MPU-6050, BH1750, ToF, ADS1115, MAX17048 |
| **9** | 19 | I2C **SCL** | Same bus |
| **34** | 20 | **IR TX** | 38 kHz fleet ID beacon |
| 17 | onboard | RGB power | Firmware drives this |
| 18 | onboard | RGB data | Onboard NeoPixel, do not reuse |
| 33 | onboard | VBUS sense | Digital, HIGH when USB 5 V present |

Onboard ESP ADC voltage taps are **not** used. Battery / solar / photodiode analog is on the **ADS1115**, and battery volts / state of charge come from the **MAX17048** (both on I2C, see below).

---

## DRV8833

| TinyS3 | DRV8833 |
|------:|---------|
| GPIO **21** | AIN1 (left) |
| GPIO **0** | AIN2 (left) |
| GPIO **44** | BIN1 (right) |
| GPIO **43** | BIN2 (right) |
| **3.3 V** | **nSLEEP** and logic VCC. Firmware does **not** drive nSLEEP |
| Boost **5 V** | **VM** (motor power) |
| **GND** | GND, common with the battery and all boards |

PWM on IN1/IN2 at **30 kHz**, 8-bit. Dead-band floor **150/255** (do not drop it). Right track is inverted in firmware (`TRACK_INVERT_RIGHT 1`).

GPIO **0**, **43**, **44** are strap / UART0 pins. Firmware keeps motor PWM at 0 when stopped, but they glitch at reset or boot-log time, so do not rely on the tracks being still in the first moments of boot.

---

## I2C (SDA GPIO 8, SCL GPIO 9, 3.3 V)

| Device | Addr | Notes |
|--------|------|-------|
| SSD1306 128x64 | **0x3C** | OLED HUD |
| **MPU-6050** | **0x68** | AD0 to GND. `/bot2/imu` |
| **BH1750** ALS | **0x23** | ADDR to GND. `/bot2/als/illuminance` |
| VL53L0X (optional) | **0x29** | `ToF --` until it ACKs |
| **ADS1115** | 0x48-0x4B (auto-probed) | See ADS1115 channels |
| **MAX17048** fuel gauge | **0x36** | LiPo voltage + state of charge, see Fuel gauge |

The ADS1115 and MAX17048 are each probed once at boot. If one is missing, firmware logs one line and carries on, with no retries and no blocking.

---

## ADS1115 channels

Gain +-4.096 V, single-shot, one channel per loop.

| Ch | Signal | Front end | Topic |
|----|--------|-----------|-------|
| A0 | **Left** BPW34 | 4.7k to GND | `/bot2/light/left_v` (pin V) |
| A1 | **Right** BPW34 | 4.7k to GND | `/bot2/light/right_v` (pin V) |
| A2 | **Main battery** (1S LiPo) | 100k top / 100k bottom (x2.0) | `/bot2/power/battery_v` (real V) |
| A3 | **Solar panel** (6 V) | 100k from panel + to the ADS pin, 47k from the ADS pin to GND (x3.128) | `/bot2/power/solar_v` (real V) |

Solar divider: panel + terminal, then 100k, then the ADS A3 pin, then 47k to ground. At a 6 V panel the pin sees 6 x 47 / 147 = 1.92 V. Channel numbers are in `tracked_pins.h` (`ADS_CH_*`). If the ADS1115 is missing, firmware logs once at boot and publishes none of these topics.

---

## Fuel gauge (onboard MAX17048, I2C 0x36)

The TinyS3[D] has the gauge onboard (no battery divider, GPIO 10 unused). Raw I2C reads, about 2.5 Hz: VCELL register `0x02` (78.125 uV/LSB) and SOC register `0x04` (high byte = %, low byte = 1/256 %). It needs the LiPo connected to the board.

| Value | Topic |
|-------|-------|
| Cell voltage (V) | `/bot2/battery/voltage` (-1 if the gauge is absent or not reading) |
| State of charge (%) | `/bot2/battery/percent` (not published if the gauge is absent) |

`/bot2/battery/motor_voltage` is an unused placeholder and always publishes -1. The OLED also shows the gauge voltage.

---

## IR

| Role | GPIO | Notes |
|------|------|--------|
| IR comm **TX** | **34** | IR LED + resistor, 38 kHz fleet ID |
| IR comm **RX** | **3** | TSOP-style demod, OUT to GPIO, VCC 3.3 V. LOW = carrier seen |
| Side proximity **L** | **6** | Digital, **LOW = object close** (about 5 cm). Firmware pull-up |
| Side proximity **R** | **5** | Same |

Side IR is published as binary Bool on `/bot2/side/left` and `/bot2/side/right` (true = close). TSOP decodes the fleet peer ID on `/bot2/ir/peer` (0 = rover, 1 = mini, 2 = tracked). Either side LOW also sets `/bot2/ir/detected`. The explore brain uses the side sensors as very-close steer-away only.

---

## Solar tilt servo

Signal **GPIO 4**, powered from the 3.3 V rail, GND common. Use a small, slow servo. A stalling 9 g servo can brown out the board.

- Commands on `/bot2/solar/servo_deg` (Float32) are clamped to **45-125** (90 = flat).
- Boot: moves to 45, sweeps smoothly to 125, then to 90, then releases.
- The PWM is **released 600 ms after every move** (no idle holding current). A command within 0.5 deg of the current angle is ignored.
- Each received command is echoed on `/bot2/solar/servo_now` (Float32) with the resulting angle.
- Tip: `ros2 topic pub --once -w 0` can drop the message before the subscriber connects, so use `-w 1` or publish a few times.

## Speaker

Passive piezo on **GPIO 7** (LEDC channel 5).

## Status

Onboard NeoPixel: dim blue = ROS idle, green = moving, amber = OTA.

---

## micro-ROS and flashing

Wi-Fi to the NUC micro-ROS agent, UDP **8888**, namespace `/bot2`. Brain is `tracked-brain` (`/data/swarm/robot-fleet`). 18 publishers and 2 subscriptions are configured (`metas/colcon_tracked.meta`).

First flash over USB (`tracked_s3_usb`), then OTA with `tracked_s3_ota` (hostname `tracked-bot`; mDNS does not cross the subnets, so use the board IP, run espota from the NUC). Power-cycle all boards after a flash.
