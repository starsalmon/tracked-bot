# Tracked bot — dockerhost swarm client

**Unexpected Maker TinyC6** (ESP32-C6) + **DRV8833** tracks. Thin client: dockerhost `tracked-brain` sends `/bot2/cmd_vel`; PWM ramp, 400 ms timeout, ToF forward brake, and nSLEEP stay on the ESP.

TinyC6 is the space/power board: **400 mAh** on the BAT JST (ESP + sensors + solar tilt), **500 mAh** through a boost 5 V rail (motors). H-bridge off when stopped. **No PS4** on this chip (C6 is BLE-only). The old NodeMCU-32S + DualShock env is still `nodemcu-32s` if you need a pad on the bench.

MPU-6050 and BH1750 share I2C **6/7** with the OLED. ToF is optional until you plug the VL53L0X in. IR comm (**14/17**) and side IR (**18/19**) are reserved — see `WIRING.md`.

## Fleet

| | |
|--|--|
| ROS namespace | `/bot2/…` |
| Agent | dockerhost UDP **8888** |
| Brain | `tracked-brain` → `explore_then_chase.py` with `BOT_NS=bot2` |
| Topics | `/bot2/cmd_vel` (sub), `/bot2/imu`, `/bot2/als/illuminance`, `/bot2/sonar/range`, `/bot2/battery/voltage` (400 mAh), `/bot2/battery/motor_voltage` (500 mAh), `/bot2/power/boost_5v`, `/bot2/ir/detected`, `/bot2/stall` |

No Go button — **tracked-brain wanders as soon as the bot is on WiFi and ROS is up** (and Allow drive is on). Slow crawl without ToF (`WANDER_BLIND_CRUISE`). ESP hard-brakes forward if ToF is under 12 cm.

OLED HUD: namespace, WiFi, ROS, IMU, battery, ToF, lux, cmd_vel.

Pins: `WIRING.md`.

## Flash

**First time (USB)** — new TinyC6, or after a partition change. Same firmware as OTA; the USB env reuses that micro-ROS library (do not rebuild it from scratch):

```bash
cd tracked-bot
pio run -e tracked_wifi_ota_usb -t upload
```

**After that (OTA) — bot must be stopped:**

```bash
pio run -e tracked_wifi_ota -t upload
```

Hostname: `tracked-bot.local`. WiFi + agent: `platformio_private.ini`.

## Envs

| Env | Use |
|-----|-----|
| `tracked_wifi_ota` | **Default** — TinyC6 fleet + OTA |
| `tracked_wifi_ota_usb` | First USB flash of that firmware |
| `tracked_wifi` | USB, no OTA server in firmware |
| `nodemcu-32s` | Legacy NodeMCU-32S + PS4, no ROS |
