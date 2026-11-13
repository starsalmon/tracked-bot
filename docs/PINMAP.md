# Tracker pin map

Unexpected Maker **TinyS3[D]** (ESP32-S3) tracked robot: DRV8833 dual 130 motors, fleet micro-ROS client `/bot2`, IR id 2.

## Boards

| Board | Role | Hostname / IP |
|-------|------|----------------|
| UM TinyS3[D] | Motion, sensors, IR, OLED, micro-ROS | `tracked-bot` / DHCP (NUC agent 192.168.3.172:8888) |

## UM TinyS3[D]

| Board pin (silk label) | GPIO | Function | Connects to (device + device pin) | Notes |
|------------------------|------|----------|-----------------------------------|-------|
| IO4 | 4 | Solar tilt servo PWM | Hobby servo signal | 3.3 V rail power; GND common; 45–125°, centre 90 |
| IO3 | 3 | Fleet IR RX | TSOP demod OUT | VCC 3.3 V, GND; 38 kHz; JTAG strap — input only |
| IO5 | 5 | Side IR R | IR LED + detector module DOUT | Active LOW, ~5 cm; firmware pull-up |
| IO21 | 21 | Left motor IN1 | DRV8833 AIN1 | 30 kHz PWM |
| IO0 | 0 | Left motor IN2 | DRV8833 AIN2 | BOOT strap; held LOW at boot |
| RX (IO44) | 44 | Right motor IN1 | DRV8833 BIN1 | UART0 RX; right track inverted in firmware |
| TX (IO43) | 43 | Right motor IN2 | DRV8833 BIN2 | UART0 TX; boot log may toggle |
| IO6 | 6 | Side IR L | IR proximity DOUT | Active LOW, pull-up |
| IO7 / D7 | 7 | Speaker | Passive piezo + | GND; LEDC tone |
| SDA (IO8) | 8 | I2C SDA | Bus devices SDA | 3.3 V, external pull-ups if needed |
| SCL (IO9) | 9 | I2C SCL | Bus devices SCL | Same bus |
| IO34 | 34 | Fleet IR TX | IR LED anode (+ resistor) | Cathode GND; 38 kHz RMT/beacon |
| 5V | — | Motor + logic boost rail in | Boost converter +5 V out | Also to TinyS3 5V pin |
| 3V3 | — | Sensor / servo logic | DRV8833 VCC, nSLEEP, I2C, IR modules | Tie DRV8833 nSLEEP HIGH |
| GND | — | Common ground | Battery (−), DRV8833, peripherals | Single return |
| VBUS | 33 | USB / 5 V present (digital) | Onboard sense only | HIGH = 5 V on USB pin; not ADC |
| NEOPIXEL_POWER | 17 | RGB LED supply enable | Onboard LED | Driven HIGH in firmware |
| NEOPIXEL | 18 | RGB LED data | Onboard WS2812 | Status colours only |
| BAT / JST | — | 1S LiPo | Cell +/− via JST | Onboard MAX17048 |
| USB-C | — | Flash / debug | Host PC | Native CDC |

## I2C devices

| Bus | Address | Device |
|-----|---------|--------|
| SDA 8 / SCL 9 | 0x3C | SSD1306 128×64 OLED (optional at boot) |
| SDA 8 / SCL 9 | 0x68 | MPU-6050 IMU (AD0 → GND) |
| SDA 8 / SCL 9 | 0x23 | BH1750 ambient light (ADDR → GND) |
| SDA 8 / SCL 9 | 0x29 | VL53L0X ToF (optional) |
| SDA 8 / SCL 9 | 0x48–0x4B | ADS1115 ADC (first ACK wins) |
| SDA 8 / SCL 9 | 0x36 | MAX17048 fuel gauge (onboard) |

## Build steps

1. Solder or socket the **TinyS3[D]**; leave USB accessible for first flash.
2. Wire **GND** and **3V3** between the board, **DRV8833** logic, and all 3.3 V sensors.
3. Connect **DRV8833 VM** and the board **5V** to the boost **+5 V** output; battery (−) common with all GND.
4. Tie **DRV8833 nSLEEP** to **3.3 V** (firmware does not drive it).
5. Motor phases: **IO21 / IO0** → left **AIN1/AIN2**; **IO44 / IO43** → right **BIN1/BIN2**.
6. **I2C**: **SDA IO8**, **SCL IO9** to OLED, MPU-6050, BH1750, optional ToF, ADS1115; 3.3 V to each module.
7. **ADS1115**: A0/A1 BPW34 with **4.7 kΩ** to GND; A2 pack divider **100 kΩ / 100 kΩ**; A3 solar **100 kΩ** from panel+ to A3, **47 kΩ** A3 to GND.
8. **Fleet IR TX IO34**: IR LED with suitable series resistor (e.g. 100–330 Ω) to 3.3 V or transistor drive; **RX IO3**: TSOP **OUT** to GPIO, **VCC** 3.3 V, **GND** common.
9. **Side IR IO6 / IO5**: digital outputs of modules to GPIO; 3.3 V power; active LOW when object near.
10. **Speaker IO7**: piezo between GPIO and GND (or transistor buffer).
11. **Solar servo IO4**: signal only; power from 3.3 V; do not stall under load.
12. Install **1S LiPo** on JST for fuel gauge and boost source.
13. Flash `tracked_s3_usb` once, then `tracked_s3_ota` for updates (bot stopped).

## Free pins (TinyS3[D])

| Silk | GPIO | Notes |
|------|------|-------|
| IO1 | 1 | Unassigned in firmware |
| IO2 | 2 | Unassigned in firmware |
| IO35 | 35 | SPI MOSI strap — avoid if using flash/PSRAM |
| IO36 | 36 | SPI SCK strap |
| IO37 | 37 | SPI MISO strap |
| BATTERY / VBAT | 10 | Onboard cell sense only; not used in tracker firmware |
