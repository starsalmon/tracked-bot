#pragma once
// Unexpected Maker TinyS3[D] (ESP32-S3) — DRV8833 tracks.
// Source of truth for docs/PINMAP.md and WIRING.md.

// --- Header GPIO (silk labels IO#) ---
#define SOLAR_SERVO_PIN 4   // IO4 — solar panel tilt servo
#define IR_RX_PIN       3   // IO3 — fleet IR demod (TSOP), JTAG strap, input only
#define IR_SIDE_R_PIN   5   // IO5 — side proximity, active LOW
#define MOTOR_A_IN1     21  // IO21 — DRV8833 AIN1, left track
#define MOTOR_A_IN2     0   // IO0 — DRV8833 AIN2, left track (BOOT strap)
#define MOTOR_B_IN1     44  // RX / IO44 — DRV8833 BIN1, right track (UART0 RX)
#define MOTOR_B_IN2     43  // TX / IO43 — DRV8833 BIN2, right track (UART0 TX)
#define IR_SIDE_L_PIN   6   // IO6 — side proximity, active LOW
#define SPEAKER_PIN     7   // IO7 / D7 — passive piezo
#define I2C_SDA_PIN     8   // SDA / IO8 — I2C bus
#define I2C_SCL_PIN     9   // SCL / IO9 — I2C bus
#define IR_TX_PIN       34  // IO34 — fleet IR LED, 38 kHz

// --- On-board (not broken out on header) ---
#define RGB_PWR_PIN     17  // NEOPIXEL_POWER
#define RGB_DATA_PIN    18  // NEOPIXEL
#define VBUS_SENSE_PIN  33  // VBUS — digital HIGH when USB 5 V present (not ADC)

// ADS1115 on I2C: A0 = left BPW34, A1 = right BPW34, A2 = main battery, A3 = solar panel.
// Address auto-probed 0x48..0x4B at boot. BPW34s: 4.7k to ground, raw pin V. A2 1S LiPo 100k/100k (x2.0);
// A3 6 V panel: 100k from panel+ to the ADS pin, 47k from the pin to ground (x3.128).
#define ADS_CH_LIGHT_L 0
#define ADS_CH_LIGHT_R 1
#define ADS_CH_BATT    2
#define ADS_CH_SOLAR   3
#define ADS_BATT_DIVIDER 2.0f
#define ADS_SOLAR_DIVIDER 3.128f

#define MPU6050_ADDR 0x68
#define IMU_GYRO_Z_SIGN (1.0f)

#define IR_TX_HZ 38000
#define IR_TX_MODULATE 1
#define IR_TX_DEFAULT_ON 1
#ifndef FLEET_IR_ID
#define FLEET_IR_ID 2
#endif

// DRV8833 nSLEEP — tie HIGH to 3.3 V in hardware; firmware does not drive a GPIO for it.

#define TRACK_PWM_HZ 30000
#define TRACK_PWM_BITS 8
#define TRACK_PWM_MIN 150
#define TRACK_PWM_MAX 255
#define TRACK_INVERT_LEFT 0
#define TRACK_INVERT_RIGHT 1
#define TRACK_ACCEL_PER_S 0.50f
#define TRACK_BRAKE_PER_S 0.85f
#define CMD_TIMEOUT_MS 400
#define DRIVE_ARM_MS 1500

#define OLED_ENABLED 1
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

// Solar panel tilt — 3.3 V rail so it still works when the motor boost is off.
// Small slow servo only; stall current can brown out the board.
#define SOLAR_SERVO_MIN_DEG 45
#define SOLAR_SERVO_MAX_DEG 125
#define SOLAR_SERVO_CENTER_DEG 90

// Passive piezo — SPEAKER_LEDC_CH is an LEDC slot, not a GPIO.
#define SPEAKER_LEDC_CH 5
#define SPEAKER_ENABLED 1

#define BH1750_ADDR 0x23
#define TOF_I2C_ADDR 0x29
#define TOF_STOP_M 0.12f
