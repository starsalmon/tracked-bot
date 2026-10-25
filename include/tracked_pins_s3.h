#pragma once
// UM TinyS3 port of tracked-bot: same physical header position as the TinyC6 wiring.
// Force-included before tracked_pins.h (whose #ifndef defaults then don't apply).
// pos = TinyPICO-footprint header pin (1-11 right column from VBAT, 12-23 left column from RX).
#define SOLAR_SERVO_PIN 4    // pos 8  (C6 IO0)
#define IR_RX_PIN       3    // pos 7  (C6 IO1)  S3 strap (JTAG src) — input, harmless
#define IR_SIDE_R_PIN   5    // pos 9  (C6 IO11)
#define MOTOR_A_IN1     21   // pos 10 (C6 IO8)  left
#define MOTOR_A_IN2     0    // pos 11 (C6 IO9)  left — S3 BOOT strap, pulled HIGH at reset (same glitch as C6)
#define MOTOR_B_IN1     44   // pos 12 (C6 IO17 / RX) right — UART0 RX
#define MOTOR_B_IN2     43   // pos 13 (C6 IO16 / TX) right — UART0 TX: ROM boot log toggles it
#define IR_SIDE_L_PIN   6    // pos 16 (C6 IO15)
#define SPEAKER_PIN     7    // pos 17 (C6 IO5)
#define I2C_SDA_PIN     8    // pos 18 (C6 IO6)
#define I2C_SCL_PIN     9    // pos 19 (C6 IO7)
#define IR_TX_PIN       34   // pos 20 (C6 IO18)
// On-board (not header):
#define RGB_PWR_PIN     17
#define RGB_DATA_PIN    18
#define VBUS_SENSE_PIN  33
