# Tracked bot wiring (Tracker, bot2)

Board: **Unexpected Maker TinyS3[D]** (ESP32-S3). **Source of truth:** `include/tracked_pins.h` and `docs/PINMAP.md`.

Power: one **1S 800 mAh LiPo** into a boost converter; the 5 V output feeds the TinyS3 **5 V** pin and the DRV8833 **VM**. Tracks are two **130-size** DC motors on a **DRV8833**.

---

## Quick pin map (silk labels)

| Silk | GPIO | Function | Notes |
|------|------|----------|-------|
| **IO4** | 4 | **Solar tilt servo** | Signal, LEDC |
| **IO3** | 3 | **IR RX** | TSOP / 38 kHz demod (fleet peer ID); strap pin, input only |
| **IO5** | 5 | **Side IR R** | Digital, active LOW, pull-up |
| **IO21** | 21 | DRV8833 **AIN1** | Left track |
| **IO0** | 0 | DRV8833 **AIN2** | Left track; **BOOT** strap |
| **RX / IO44** | 44 | DRV8833 **BIN1** | Right track; UART0 RX |
| **TX / IO43** | 43 | DRV8833 **BIN2** | Right track; UART0 TX |
| **IO6** | 6 | **Side IR L** | Digital, active LOW, pull-up |
| **IO7 / D7** | 7 | **Speaker** | Passive piezo (LEDC) |
| **SDA / IO8** | 8 | I2C **SDA** | OLED, MPU, BH1750, ToF, ADS1115, MAX17048 |
| **SCL / IO9** | 9 | I2C **SCL** | Same bus |
| **IO34** | 34 | **IR TX** | 38 kHz fleet ID beacon |
| *(onboard)* | 17 | RGB power | Firmware drives this |
| *(onboard)* | 18 | RGB data | NeoPixel — do not reuse |
| **VBUS** | 33 | 5 V sense | Digital HIGH when USB 5 V present |

Onboard ESP ADC taps are **not** used for pack voltage. Battery / solar / photodiode analog is on the **ADS1115**; cell voltage and state of charge also come from the onboard **MAX17048** (I2C).

---

## DRV8833

| TinyS3 | DRV8833 |
|--------|---------|
| **IO21** | AIN1 (left) |
| **IO0** | AIN2 (left) |
| **IO44** | BIN1 (right) |
| **IO43** | BIN2 (right) |
| **3.3 V** | **nSLEEP** and logic VCC. Firmware does **not** drive nSLEEP |
| Boost **5 V** | **VM** (motor power) |
| **GND** | GND, common with the battery and all boards |

PWM on IN1/IN2 at **30 kHz**, 8-bit. Dead-band floor **150/255** (do not drop it). Right track is inverted in firmware (`TRACK_INVERT_RIGHT 1`).

**IO0**, **IO43**, and **IO44** are strap / UART0 pins. Firmware keeps motor PWM at 0 when stopped, but they can glitch at reset or during the ROM boot log — do not rely on the tracks being still in the first moments of boot.

---

## I2C (SDA IO8, SCL IO9, 3.3 V)

| Device | Addr | Notes |
|--------|------|-------|
| SSD1306 128×64 | **0x3C** | OLED HUD |
| **MPU-6050** | **0x68** | AD0 to GND. `/bot2/imu` |
| **BH1750** ALS | **0x23** | ADDR to GND. `/bot2/als/illuminance` |
| VL53L0X (optional) | **0x29** | `ToF --` until it ACKs |
| **ADS1115** | 0x48–0x4B (auto-probed) | See ADS1115 channels |
| **MAX17048** fuel gauge | **0x36** | Onboard LiPo gauge |

The ADS1115 and MAX17048 are each probed once at boot. If one is missing, firmware logs one line and carries on.

---

## ADS1115 channels

Gain ±4.096 V, single-shot, one channel per loop.

| Ch | Signal | Front end | Topic |
|----|--------|-----------|-------|
| A0 | **Left** BPW34 | 4.7k to GND | `/bot2/light/left_v` |
| A1 | **Right** BPW34 | 4.7k to GND | `/bot2/light/right_v` |
| A2 | **Main battery** (1S LiPo) | 100k / 100k (×2.0) | `/bot2/power/battery_v` |
| A3 | **Solar panel** (6 V) | 100k from panel+ to pin, 47k pin to GND (×3.128) | `/bot2/power/solar_v` |

---

## Fuel gauge (onboard MAX17048, I2C 0x36)

Raw I2C reads, about 2.5 Hz: VCELL `0x02`, SOC `0x04`. Needs the LiPo on the board JST.

| Value | Topic |
|-------|-------|
| Cell voltage (V) | `/bot2/battery/voltage` |
| State of charge (%) | `/bot2/battery/percent` |

`/bot2/battery/motor_voltage` is unused (−1). The OLED shows gauge voltage when present.

---

## IR

| Role | Pin | Notes |
|------|-----|--------|
| IR comm **TX** | **IO34** | IR LED + series resistor, 38 kHz fleet ID |
| IR comm **RX** | **IO3** | TSOP OUT to GPIO, VCC 3.3 V |
| Side proximity **L** | **IO6** | LOW = close (~5 cm) |
| Side proximity **R** | **IO5** | Same |

Fleet IR id **2**. Peer IDs on `/bot2/ir/peer`. Side sensors on `/bot2/side/left` and `/bot2/side/right`.

---

## Solar tilt servo

Signal **IO4**, **3.3 V** power, GND common. Small slow servo only.

- `/bot2/solar/servo_deg` (Float32), clamped **45–125** (90 = flat).
- Boot sweep 45 → 125 → 90, then PWM released after each move.

## Speaker

Passive piezo on **IO7**.

## Status

Onboard NeoPixel: dim blue = ROS idle, green = moving, amber = OTA.

---

## micro-ROS and flashing

Wi-Fi to the NUC micro-ROS agent, UDP **8888**, namespace `/bot2`. Brain is `tracked-brain`. Publisher list: `metas/colcon_tracked.meta`.

First flash over USB (`tracked_s3_usb`), then OTA with `tracked_s3_ota` (hostname `tracked-bot`). Power-cycle after a flash.
