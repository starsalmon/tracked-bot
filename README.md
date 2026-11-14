# Tracked bot — dockerhost swarm client

**Unexpected Maker TinyS3[D]** (ESP32-S3) + **DRV8833** tracked drive. Thin client: dockerhost `tracked-brain` sends `/bot2/cmd_vel`; PWM ramp, 400 ms timeout, ToF forward brake, and nSLEEP stay on the ESP.

One **1S 800 mAh LiPo** feeds a boost converter; **5 V** runs the tracks (DRV8833 VM) and the board **5 V** pin. ESP, sensors, and solar tilt servo use **3.3 V**. H-bridge outputs idle low when stopped.

MPU-6050, BH1750, OLED, ToF, ADS1115, and MAX17048 share I2C **SDA IO8 / SCL IO9**. Fleet IR TX/RX, side IR, and speaker are on dedicated GPIOs.

## Docs

| Doc | Contents |
|-----|----------|
| [`docs/PINMAP.md`](docs/PINMAP.md) | How-to-build pin map (TinyS3 silk labels) |
| [`WIRING.md`](WIRING.md) | Harness, I2C devices, IR, power — includes **motor lines on strap/UART pins** (IO0 BOOT, IO43/IO44 UART0) |
| [`docs/TODO.md`](docs/TODO.md) | Future plans (Tracker + fleet-wide) |

## Fleet

| | |
|--|--|
| ROS namespace | `/bot2/…` |
| Agent | dockerhost UDP **8888** |
| Brain | `tracked-brain` → `explore_then_chase.py` with `BOT_NS=bot2` |
| Fleet IR id | **2** |

No Go button — **tracked-brain wanders as soon as the bot is on WiFi and ROS is up** (and Allow drive is on). Slow crawl without ToF (`WANDER_BLIND_CRUISE`). ESP hard-brakes forward if ToF is under 12 cm.

OLED HUD: namespace, WiFi, ROS, IMU, battery, ToF, lux, cmd_vel.

## Flash

**First time (USB)** — new board or after a partition change. Same firmware as OTA; the USB env reuses the OTA micro-ROS library (do not rebuild it from scratch):

```bash
cd tracked-bot
pio run -e tracked_s3_usb -t upload
```

**After that (OTA) — bot must be stopped:**

```bash
pio run -e tracked_s3_ota -t upload
```

Hostname: `tracked-bot.local`. WiFi + agent: `platformio_private.ini`. Flash mode is **DIO** (QIO can watchdog-loop on this board).

## Envs

| Env | Use |
|-----|-----|
| `tracked_s3_ota` | **Default** — fleet + OTA |
| `tracked_s3_usb` | First USB flash of that firmware |
| `tracked_s3` | USB serial upload, no OTA server in firmware |

On the NUC: `./build_flash.sh build` (defaults to `tracked_s3_ota`).
