# Tracked bot wiring

Board: **Unexpected Maker TinyC6** (ESP32-C6). **Source of truth:** `include/tracked_pins.h`.

LiPo **400 mAh** on the TinyC6 **JST-PH** (onboard charger). **500 mAh** is a separate charge/boost pack for 5 V motors. Do **not** use GPIO 0/1 (32 kHz crystal).

The old **NodeMCU-32S** sketch is still `nodemcu-32s` (PS4 / Classic BT only). Fleet firmware is TinyC6 — C6 has **BLE only**, so no PS4 pad.

MPU-6050 is on the same I2C as the OLED. ToF can stay unplugged (`ToF --` until it ACKs).

---

## Quick pin map

| GPIO | Function | Notes |
|------|----------|--------|
| **2** | DRV8833 AIN1 | Left track |
| **3** | DRV8833 AIN2 | Left |
| **20** | DRV8833 BIN1 | Right track |
| **21** | DRV8833 BIN2 | Right |
| **16** | DRV8833 **nSLEEP** | HIGH = run; firmware pulls LOW when stopped |
| **6** | I2C SDA | OLED + **MPU-6050** + ToF |
| **7** | I2C SCL | Same bus |
| **8** | **IR TX** | 38 kHz fleet ID beacon (C6 strap pin — idle LOW at boot) |
| **16** | DRV8833 **nSLEEP** | Also board silk **TX** — motors, not IR |
| **17** | **IR RX** | TSOP / demod; board silk **RX** |
| **18** | **Side IR L** (reserved) | Digital proximity, active LOW, pull-up |
| **19** | **Side IR R** (reserved) | Same |
| **4** | VBAT sense | **400 mAh** onboard divider — do not reuse |
| **5** | **500 mAh** pack divider | Last C6 ADC. Mid-point ≤ **3.3 V**. ≥47 k resistors. |
| **10** | VBUS / boost 5 V sense | USB **or** boost 5 V into the TinyC6 5 V pin |
| **11** | **Solar tilt servo** (reserved) | Signal only. Power from TinyC6 **3.3 V**. |
| **22** | RGB power | Onboard — firmware drives this |
| **23** | RGB data | Onboard NeoPixel — do not reuse |

---

## DRV8833

| TinyC6 | DRV8833 |
|------:|---------|
| GPIO **2** | AIN1 (left) |
| GPIO **3** | AIN2 |
| GPIO **20** | BIN1 (right) |
| GPIO **21** | BIN2 |
| GPIO **16** | **nSLEEP** |
| **3.3 V** | VCC (logic) | Stays up from the 400 mAh even if boost 5 V is dead |
| **GND** | GND | Common with both packs |
| Boost **5 V** | **VM** (motor power) | Same 5 V rail as TinyC6 5 V in |

PWM on IN1/IN2 at **30 kHz**, 8-bit. Dead-band floor **150/255** (do not drop it).

**nSLEEP not wired to GPIO16 yet:** jumper the DRV8833 **nSLEEP** pin to **3.3 V** on the chip so the bridge stays awake until you connect GPIO16.

When stopped, firmware pulls nSLEEP **LOW** (once wired). GPIO10 is a 5 V-present flag for the OLED/brain only — it does not block `/cmd_vel`.

---

## I2C (OLED / MPU / ToF)

| Device | Addr | TinyC6 |
|--------|------|--------|
| SSD1306 128×64 | **0x3C** | SDA **6**, SCL **7**, 3.3 V |
| **MPU-6050** | **0x68** | Same bus. AD0 → GND. INT unused. 3.3 V only. |
| **BH1750** ALS | **0x23** | Same bus. ADDR → GND (or 3.3 V for **0x5C**). 3.3 V only. |
| VL53L0X (later) | **0x29** | Same bus, 3.3 V, XSHUT floating |

OLED HUD shows `IMU` when the MPU ACKs, and lux on the ToF line when the BH1750 ACKs (`120lx`). `/bot2/imu` and `/bot2/als/illuminance`. If yaw looks flipped, set `IMU_GYRO_Z_SIGN` to `-1` in `tracked_pins.h`.

Firmware shows `ToF --` until the VL53L0X ACKs. VL53L1X needs a different driver — say if that is what you fitted.

---

## IR (reserved — wire later)

Bot-to-bot ID (all fleet members, different behaviour when they meet) and side bumpers. **3.3 V logic.** TX stays off until we add the ID protocol.

| Role | GPIO | Notes |
|------|------|--------|
| IR comm **TX** | **8** | IR LED + resistor (transistor optional). **Not GPIO14** — that pin does not exist on TinyC6. **Not GPIO16** — that is nSLEEP / board TX. |
| IR comm **RX** | **17** | TSOP-style 38 kHz demod, OUT → GPIO, VCC 3.3 V (board **RX** pin) |
| Side proximity **L** | **18** | Digital, **LOW = hit**. Firmware pull-up. |
| Side proximity **R** | **19** | Same |

Either side LOW still sets `/bot2/ir/detected`. **GPIO17** TSOP decodes fleet peer ID on `/bot2/ir/peer` (0=rover, 1=mini, 2=tracked). **GPIO8** TX broadcasts bot2's ID frame.

**GPIO16** is the motor driver **nSLEEP** line — even though the header says "TX", do not wire the IR LED there.

Do **not** hang a TSOP on GPIO **8 / 9** — C6 strapping pins; a LOW at reset can trap boot. TX LED on GPIO8 is OK (firmware holds it off until the beacon starts).

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

GPIO **10** is a **digital** 5 V-present flag (not ADC — analog on this pin panics the C6). GPIO **5** is the **500 mAh pack** divider (ADC). OLED: `E 3.91 M 3.72` plus `5V` or `bat`.

TinyC6 **VBAT_SENSE** (GPIO **4**) is a **1:1 resistor divider** on the 400 mAh (pack/2 at the pin). Firmware uses ADC **11 dB attenuation** so the pin can read ~0–3.1 V (a full 4.2 V cell is ~2.1 V at GPIO4). Default 0 dB only goes to ~1.1 V and reports a stuck ~2.2 V after ×2. `/bot2/battery/voltage` = pin × `VBAT_DIVIDER` (2.0). DMM-check `VBAT_CAL` if a few tenths off.

Do not pull track current through the TinyC6 BAT JST.

---

## Solar tilt servo (3.3 V)

Signal **GPIO 11**. **VCC = TinyC6 3.3 V**, GND common. That way the panel can still hunt after the motor pack dies.

Use a **small** analog servo and **slow** steps. The C6 3.3 V rail is shared with the MCU — a stalling 9 g servo can brown out the board. Firmware does **not** drive the servo yet (no hunt loop); pin is reserved, left idle.

BH1750 on the panel (or nearby) is the light sensor for that hunt later.

## Status

Onboard NeoPixel: dim blue = ROS idle, green = moving, amber = OTA. RGB power is cut when the LED is off.

---

## micro-ROS

Same as before: WiFi → dockerhost UDP **8888**, namespace `/bot2`. Brain is `tracked-brain`. First flash is **USB** (`tracked_wifi_ota_usb`); after that OTA to `tracked-bot.local` with the bot **stopped**.
