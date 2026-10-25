# Tracked bot wiring

Board: **Unexpected Maker TinyC6** (ESP32-C6). **Source of truth:** `include/tracked_pins.h`.

LiPo **400 mAh** on the TinyC6 **JST-PH** (onboard charger). **500 mAh** is a separate charge/boost pack for 5 V motors. **GPIO 0** (solar servo) and **GPIO 1** (fleet IR RX) use the crystal pins — assigned in this rewire.

The old **NodeMCU-32S** sketch is still `nodemcu-32s` (PS4 / Classic BT only). Fleet firmware is TinyC6 — C6 has **BLE only**, so no PS4 pad.

MPU-6050 is on the same I2C as the OLED. ToF can stay unplugged (`ToF --` until it ACKs).

---

## Quick pin map

| GPIO | Function | Notes |
|------|----------|--------|
| **0** | **Solar tilt servo** | Signal. Power from TinyC6 **3.3 V** |
| **1** | **IR RX** | TSOP / 38 kHz demod (fleet peer ID) |
| **5** | **Speaker** | Passive piezo (LEDC) |
| **6** | I2C SDA | OLED + **MPU-6050** + BH1750 + ToF + **ADS1115** |
| **7** | I2C SCL | Same bus |
| **8** | DRV8833 **BIN1** | Right track (C6 strap — idle LOW at boot) |
| **9** | DRV8833 **BIN2** | Right |
| **10** | VBUS / boost 5 V sense | USB **or** boost 5 V into the TinyC6 5 V pin |
| **11** | **Side IR R** | Digital proximity, active LOW, pull-up |
| **15** | **Side IR L** | Same |
| **17** | DRV8833 **AIN1** | Left track |
| **16** | DRV8833 **AIN2** | Left (board silk **TX**) |
| **18** | **IR TX** | 38 kHz fleet ID beacon |
| **22** | RGB power | Onboard — firmware drives this |
| **23** | RGB data | Onboard NeoPixel — do not reuse |

Onboard ESP **ADC** voltage taps (GPIO **4** VBAT, old GPIO **5** pack divider, etc.) are **not** used in this harness. Pack/battery/solar/photodiode analog is on an **ADS1115** on I2C (see below).

---

## DRV8833

| TinyC6 | DRV8833 |
|------:|---------|
| GPIO **17** | AIN1 (left) |
| GPIO **16** | AIN2 |
| GPIO **8** | BIN1 (right) |
| GPIO **9** | BIN2 |
| **3.3 V** | **nSLEEP** | Jumper **nSLEEP → 3.3 V** — bridge always awake; firmware does **not** toggle nSLEEP |
| **3.3 V** | VCC (logic) | Stays up from the 400 mAh even if boost 5 V is dead |
| **GND** | GND | Common with both packs |
| Boost **5 V** | **VM** (motor power) | Same 5 V rail as TinyC6 5 V in |

PWM on IN1/IN2 at **30 kHz**, 8-bit. Dead-band floor **150/255** (do not drop it).

GPIO **8** / **9** are C6 **strapping** pins — firmware keeps motor PWM at **0** when stopped so boot is not trapped.

GPIO10 is a 5 V-present flag for the OLED/brain only — it does not block `/cmd_vel`.

---

## I2C (OLED / MPU / ToF / ADS1115)

| Device | Addr | TinyC6 |
|--------|------|--------|
| SSD1306 128×64 | **0x3C** | SDA **6**, SCL **7**, 3.3 V |
| **MPU-6050** | **0x68** | Same bus. AD0 → GND. INT unused. 3.3 V only. |
| **BH1750** ALS | **0x23** | Same bus. ADDR → GND (or 3.3 V for **0x5C**). 3.3 V only. |
| VL53L0X (later) | **0x29** | Same bus, 3.3 V, XSHUT floating |
| ADS1115 | 0x48–0x4B (auto-probed) | Same bus — see ADS1115 channels |

OLED HUD shows `IMU` when the MPU ACKs, and lux on the ToF line when the BH1750 ACKs (`120lx`). `/bot2/imu` and `/bot2/als/illuminance`. If yaw looks flipped, set `IMU_GYRO_Z_SIGN` to `-1` in `tracked_pins.h`.

Firmware shows `ToF --` until the VL53L0X ACKs. VL53L1X needs a different driver — say if that is what you fitted.

---

## IR

Bot-to-bot ID (all fleet members, different behaviour when they meet) and side bumpers. **3.3 V logic.**

| Role | GPIO | Notes |
|------|------|--------|
| IR comm **TX** | **18** | IR LED + resistor (transistor optional). **Not GPIO14** — not bonded on TinyC6. **Not GPIO16** — that is left motor AIN2 / board **TX** silk. |
| IR comm **RX** | **1** | TSOP-style 38 kHz demod, OUT → GPIO, VCC 3.3 V |
| Side proximity **L** | **15** | Digital, **LOW = hit**. Firmware pull-up. |
| Side proximity **R** | **11** | Same |

Either side LOW still sets `/bot2/ir/detected`. **GPIO1** TSOP decodes fleet peer ID on `/bot2/ir/peer` (0=rover, 1=mini, 2=tracked). **GPIO18** TX broadcasts bot2's ID frame.

Do **not** hang a TSOP on GPIO **8 / 9** — strapping pins; motors use those lines.

**12 / 13** are USB — leave them.

---

## Power (two packs)

| Pack | Where it plugs | What it runs |
|------|----------------|--------------|
| **~400 mAh** LiPo | TinyC6 **BAT** JST | ESP, OLED, IMU, BH1750, ToF, IR, **solar tilt servo** (3.3 V) |
| **~500 mAh** + charge/boost | Boost module input | Boost **5 V** → TinyC6 **5 V** pin **and** DRV8833 **VM** (tracks) |

While boost 5 V is present, the TinyC6 charger keeps the **400 mAh** topped up from that 5 V (same as USB charging). If the 500 mAh / boost goes flat, the ESP **keeps running** on the 400 mAh: WiFi, ROS, sensors, solar tilt. Tracks stop — there is no motor 5 V.

**GND common** across TinyC6, boost board, DRV8833, both packs, servo.

```
500 mAh  →  charge/boost  →  5 V  →  TinyC6 5V  +  DRV8833 VM
400 mAh  →  TinyC6 BAT    →  3.3 V →  ESP + I2C + solar servo
                 ↑ charged from 5 V whenever boost (or USB) is in
```

GPIO **10** is a **digital** 5 V-present flag (not ADC — analog on this pin panics the C6). OLED may show `5V` or `bat` from that flag; pack voltages come from the ADS1115.

Do not pull track current through the TinyC6 BAT JST.

---

## Solar tilt servo (3.3 V)

Signal **GPIO 0**. **VCC = TinyC6 3.3 V**, GND common. That way the panel can still hunt after the motor pack dies.

Use a **small** analog servo and **slow** steps. The C6 3.3 V rail is shared with the MCU — a stalling 9 g servo can brown out the board. Firmware moves it to 90 (flat) at boot and takes clamped 45–125 commands (no sun-seeking yet).

BH1750 on the panel (or nearby) is the light sensor for that hunt later.

## Speaker

Passive piezo on **GPIO 5** (LEDC). Keep clear of the old pack-divider net on that pin.

## Status

Onboard NeoPixel: dim blue = ROS idle, green = moving, amber = OTA. RGB power is cut when the LED is off.

---

## micro-ROS

Same as before: WiFi → dockerhost UDP **8888**, namespace `/bot2`. Brain is `tracked-brain`. First flash is **USB** (`tracked_wifi_ota_usb`); after that OTA to `tracked-bot.local` with the bot **stopped**.

## ADS1115 channels

| Ch | Signal | Front end | Topic |
|----|--------|-----------|-------|
| A0 | Main battery (1S 800 mAh LiPo) | 100k top / 100k bottom (x2.0) | `/bot2/power/battery_v` (real V) |
| A1 | Solar panel (6 V) | 100k top / 47k bottom (x3.128) | `/bot2/power/solar_v` (real V) |
| A2 | Left BPW34 | 4.7k load to GND | `/bot2/light/left_v` (pin V) |
| A3 | Right BPW34 | 4.7k load to GND | `/bot2/light/right_v` (pin V) |

Gain +-4.096 V. If the ADS1115 is missing, firmware logs once at boot and publishes none of these.

## Solar tilt servo limits

45 = panel 45 deg left, 90 = flat (boot position), 125 = 45 deg right. `/bot2/solar/servo_deg` (Float32) is clamped to 45–125.

## Side IR

Binary IR proximity, trips at ~5 cm. `/bot2/side/left`, `/bot2/side/right` (Bool, true = object close).
