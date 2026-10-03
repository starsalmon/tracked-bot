#pragma once

// Unexpected Maker TinyC6 (ESP32-C6) — DRV8833 tracks.
// Source of truth: WIRING.md.
// GPIO 0 = solar servo signal, GPIO 1 = fleet IR RX (32 kHz crystal pins — now assigned).

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 6  // TinyC6 SDA — OLED + MPU + BH1750 + ToF (+ future ADS1115)
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 7  // TinyC6 SCL
#endif

// Future ADS1115 on this I2C bus (no driver yet): A2 = battery, A3 = solar, A0/A1 = photodiodes.

#ifndef MPU6050_ADDR
#define MPU6050_ADDR 0x68
#endif
#ifndef IMU_GYRO_Z_SIGN
#define IMU_GYRO_Z_SIGN (1.0f)
#endif

// Bot-to-bot IR ID. TinyC6 has no GPIO14 on the header (not bonded out).
#ifndef IR_TX_PIN
#define IR_TX_PIN 18
#endif
#ifndef IR_RX_PIN
#define IR_RX_PIN 1  // TSOP / demod (LOW = carrier seen)
#endif
#ifndef IR_TX_HZ
#define IR_TX_HZ 38000
#endif
#ifndef IR_TX_MODULATE
#define IR_TX_MODULATE 1
#endif
#ifndef IR_TX_DEFAULT_ON
#define IR_TX_DEFAULT_ON 1
#endif
#ifndef FLEET_IR_ID
#define FLEET_IR_ID 2
#endif

// Side IR proximity (digital, active LOW). Pull-ups on; unused = HIGH.
#ifndef IR_SIDE_L_PIN
#define IR_SIDE_L_PIN 15
#endif
#ifndef IR_SIDE_R_PIN
#define IR_SIDE_R_PIN 11
#endif

#ifndef MOTOR_A_IN1
#define MOTOR_A_IN1 17  // left
#endif
#ifndef MOTOR_A_IN2
#define MOTOR_A_IN2 16
#endif
#ifndef MOTOR_B_IN1
#define MOTOR_B_IN1 8  // right (C6 strap — PWM idle LOW at boot)
#endif
#ifndef MOTOR_B_IN2
#define MOTOR_B_IN2 9  // right (C6 strap)
#endif
// DRV8833 nSLEEP — tie HIGH to 3.3 V in hardware; firmware does not drive a GPIO for it.

#ifndef TRACK_PWM_HZ
#define TRACK_PWM_HZ 30000
#endif
#ifndef TRACK_PWM_BITS
#define TRACK_PWM_BITS 8
#endif

// Bench dead-band: map |cmd|>0 onto 150–255 duty (do not drop this floor).
#ifndef TRACK_PWM_MIN
#define TRACK_PWM_MIN 150
#endif
#ifndef TRACK_PWM_MAX
#define TRACK_PWM_MAX 255
#endif

#ifndef TRACK_INVERT_LEFT
#define TRACK_INVERT_LEFT 0
#endif
#ifndef TRACK_INVERT_RIGHT
#define TRACK_INVERT_RIGHT 0
#endif

#ifndef TRACK_ACCEL_PER_S
#define TRACK_ACCEL_PER_S 0.50f
#endif
#ifndef TRACK_BRAKE_PER_S
#define TRACK_BRAKE_PER_S 0.85f
#endif

#ifndef CMD_TIMEOUT_MS
#define CMD_TIMEOUT_MS 400
#endif
#ifndef DRIVE_ARM_MS
#define DRIVE_ARM_MS 1500
#endif

#ifndef OLED_ENABLED
#define OLED_ENABLED 1
#endif
#ifndef OLED_WIDTH
#define OLED_WIDTH 128
#endif
#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif
#ifndef OLED_ADDR
#define OLED_ADDR 0x3C
#endif

// Onboard NeoPixel (GPIO23 data, GPIO22 power).
#ifndef RGB_PWR_PIN
#define RGB_PWR_PIN 22
#endif
#ifndef RGB_DATA_PIN
#define RGB_DATA_PIN 23
#endif

#ifndef VBUS_SENSE_PIN
#define VBUS_SENSE_PIN 10  // TinyC6 VBUS — digital HIGH when 5 V (USB or boost) is in. Not ADC.
#endif

// Solar panel tilt — 3.3 V rail so it still works when the 500 mAh / boost 5 V is dead.
// PWM not attached yet (no hunt loop). Small slow servo only; stall current can brown out the C6.
#ifndef SOLAR_SERVO_PIN
#define SOLAR_SERVO_PIN 0
#endif

// Passive piezo — SPEAKER_LEDC_CH is an LEDC slot, not a GPIO.
#ifndef SPEAKER_PIN
#define SPEAKER_PIN 5
#endif
#ifndef SPEAKER_LEDC_CH
#define SPEAKER_LEDC_CH 5
#endif
#ifndef SPEAKER_ENABLED
#define SPEAKER_ENABLED 1
#endif

#ifndef BH1750_ADDR
#define BH1750_ADDR 0x23  // ADDR pin to GND; 0x5C if ADDR to 3.3 V
#endif

#ifndef TOF_I2C_ADDR
#define TOF_I2C_ADDR 0x29
#endif
#ifndef TOF_STOP_M
#define TOF_STOP_M 0.12f
#endif
