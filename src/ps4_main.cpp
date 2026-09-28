/*
 * Robot controller
 *
 * NodeMCU-32S / ESP32
 * DRV8833 dual motor driver
 * PS4 controller via Bluetooth
 *
 * Compatible with Arduino-ESP32 2.x, which is required by the
 * PS4Controller library version being used.
 */

#include <Arduino.h>
#include <Wire.h>

#include <PS4Controller.h>
#include <MPU6050_light.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "DifferentialSteering.h"
#include "tracked_stick.h"

#include <esp_system.h>

// -----------------------------------------------------------------------------
// Optional hardware
// -----------------------------------------------------------------------------

#define MPU_ENABLED 0
#define OLED_ENABLED 1

// -----------------------------------------------------------------------------
// OLED
// -----------------------------------------------------------------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

// -----------------------------------------------------------------------------
// MPU6050
// -----------------------------------------------------------------------------

#if MPU_ENABLED
MPU6050 mpu(Wire);
#endif

unsigned long timer = 0;

int16_t ax = 0;
int16_t ay = 0;
int16_t az_raw = 0;

int16_t gx = 0;
int16_t gy = 0;
int16_t gz = 0;

int16_t lockangle = 0;
int adjangle = 0;
int az = 0;

// -----------------------------------------------------------------------------
// Differential steering
// -----------------------------------------------------------------------------

int fPivYLimit = 32;
DifferentialSteering DiffSteer;

bool assist = false;
String mode = "angles";

// -----------------------------------------------------------------------------
// Pins
// -----------------------------------------------------------------------------

const int onboardled = 2;

// DRV8833 Motor A / left motor
const int motorAPin1 = 27;
const int motorAPin2 = 26;

// DRV8833 Motor B / right motor
const int motorBPin1 = 32;
const int motorBPin2 = 33;

// -----------------------------------------------------------------------------
// PWM
// -----------------------------------------------------------------------------

const int pwmFreq = 30000;
const int pwmResolution = 8;

// Arduino-ESP32 2.x requires explicit LEDC channels.
const int motorAPin1Channel = 0;
const int motorAPin2Channel = 1;
const int motorBPin1Channel = 2;
const int motorBPin2Channel = 3;

// Motor dead-band compensation
const int pwmMin = 150;
const int pwmMax = 255;

// -----------------------------------------------------------------------------
// Tuning
// -----------------------------------------------------------------------------

int adj_map = 3;
int adj_out = 20;

int lowLimit = -115;
int highLimit = 115;

int l_speed = 0;
int r_speed = 0;

int LStickXvalue = 0;
int prevXValue = 0;
int XValue = 0;
int YValue = 0;

// -----------------------------------------------------------------------------
// OLED helpers
// -----------------------------------------------------------------------------

void oledClear()
{
#if OLED_ENABLED
    display.clearDisplay();
#endif
}

void oledShow()
{
#if OLED_ENABLED
    display.display();
#endif
}

void oledReady()
{
#if OLED_ENABLED
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(F("Ready."));
    display.display();
#endif
}

// -----------------------------------------------------------------------------
// DRV8833 motor control
// -----------------------------------------------------------------------------

int speedToDuty(int speed)
{
    speed = abs(speed);

    if (speed <= 1)
    {
        return 0;
    }

    return constrain(
        map(speed, 2, 128, pwmMin, pwmMax),
        0,
        255
    );
}

void stopMotorA()
{
    ledcWrite(motorAPin1Channel, 0);
    ledcWrite(motorAPin2Channel, 0);
}

void stopMotorB()
{
    ledcWrite(motorBPin1Channel, 0);
    ledcWrite(motorBPin2Channel, 0);
}

void stopMotors()
{
    stopMotorA();
    stopMotorB();
}

void driveMotorA(int speed)
{
    speed = constrain(speed, -128, 128);

    if (speed > 1)
    {
        ledcWrite(
            motorAPin1Channel,
            speedToDuty(speed)
        );

        ledcWrite(
            motorAPin2Channel,
            0
        );
    }
    else if (speed < -1)
    {
        ledcWrite(
            motorAPin1Channel,
            0
        );

        ledcWrite(
            motorAPin2Channel,
            speedToDuty(speed)
        );
    }
    else
    {
        stopMotorA();
    }
}

void driveMotorB(int speed)
{
    speed = constrain(speed, -128, 128);

    if (speed > 1)
    {
        ledcWrite(
            motorBPin1Channel,
            speedToDuty(speed)
        );

        ledcWrite(
            motorBPin2Channel,
            0
        );
    }
    else if (speed < -1)
    {
        ledcWrite(
            motorBPin1Channel,
            0
        );

        ledcWrite(
            motorBPin2Channel,
            speedToDuty(speed)
        );
    }
    else
    {
        stopMotorB();
    }
}

void drive(int leftSpeed, int rightSpeed)
{
    driveMotorA(leftSpeed);
    driveMotorB(rightSpeed);
}

void printBluetoothMac()
{
    uint8_t mac[6];

    esp_read_mac(mac, ESP_MAC_BT);

    Serial.printf(
        "ESP32 Bluetooth MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );
}


// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(200);

    printBluetoothMac();

    PS4.begin("AC:3E:B1:0A:5B:3C");
    
    Serial.println(
        F("PS4 controller interface started.")
    );    

    Serial.println();
    Serial.println(F("Robot starting..."));
    Serial.println(F("Platform: NodeMCU-32S / Arduino-ESP32 2.x"));
    Serial.println(F("Motor driver: DRV8833"));

    // I2C
    Wire.begin();

    // -------------------------------------------------------------------------
    // OLED
    // -------------------------------------------------------------------------

#if OLED_ENABLED

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 not detected."));
    }
    else
    {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println(F("OLED OK"));
        display.display();
    }

#else

    Serial.println(
        F("SSD1306 support compiled in, hardware disabled.")
    );

#endif

    // -------------------------------------------------------------------------
    // MPU6050
    // -------------------------------------------------------------------------

#if MPU_ENABLED

    byte status = mpu.begin();

    Serial.print(F("MPU6050 status: "));
    Serial.println(status);

#if OLED_ENABLED

    display.clearDisplay();
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println(F("MPU6050:"));

    display.setCursor(60, 0);
    display.println(status);

    display.setCursor(0, 16);
    display.println(F("Calculating..."));

    display.setCursor(0, 24);
    display.println(F("keep still!"));

    display.display();

#endif

    if (status == 0)
    {
        Serial.println(
            F("Calculating offsets, do not move MPU6050")
        );

        delay(1000);

        mpu.calcOffsets();

        Serial.println(
            F("MPU6050 calibration complete.")
        );
    }
    else
    {
        Serial.println(
            F("MPU6050 not detected; gyro assist disabled.")
        );

        assist = false;
    }

#else

    Serial.println(
        F("MPU6050 support compiled in, hardware disabled.")
    );

#endif

    // -------------------------------------------------------------------------
    // GPIO
    // -------------------------------------------------------------------------

    pinMode(onboardled, OUTPUT);
    digitalWrite(onboardled, LOW);

    pinMode(motorAPin1, OUTPUT);
    pinMode(motorAPin2, OUTPUT);
    pinMode(motorBPin1, OUTPUT);
    pinMode(motorBPin2, OUTPUT);

    // -------------------------------------------------------------------------
    // PWM
    //
    // Arduino-ESP32 2.x API:
    //   ledcSetup(channel, frequency, resolution)
    //   ledcAttachPin(pin, channel)
    //   ledcWrite(channel, duty)
    // -------------------------------------------------------------------------

    bool pwmOK = true;

    pwmOK &= (
        ledcSetup(
            motorAPin1Channel,
            pwmFreq,
            pwmResolution
        ) > 0
    );

    pwmOK &= (
        ledcSetup(
            motorAPin2Channel,
            pwmFreq,
            pwmResolution
        ) > 0
    );

    pwmOK &= (
        ledcSetup(
            motorBPin1Channel,
            pwmFreq,
            pwmResolution
        ) > 0
    );

    pwmOK &= (
        ledcSetup(
            motorBPin2Channel,
            pwmFreq,
            pwmResolution
        ) > 0
    );

    ledcAttachPin(
        motorAPin1,
        motorAPin1Channel
    );

    ledcAttachPin(
        motorAPin2,
        motorAPin2Channel
    );

    ledcAttachPin(
        motorBPin1,
        motorBPin1Channel
    );

    ledcAttachPin(
        motorBPin2,
        motorBPin2Channel
    );

    if (!pwmOK)
    {
        Serial.println(
            F("WARNING: one or more PWM channels failed.")
        );
    }
    else
    {
        Serial.println(
            F("DRV8833 PWM configured.")
        );
    }

    stopMotors();

    // -------------------------------------------------------------------------
    // PS4 controller
    // -------------------------------------------------------------------------

    // To use a fixed controller Bluetooth address instead, use:
    //
    // PS4.begin("48:b0:2d:37:2d:4c");

    //PS4.begin("E0:8C:FE:F9:C2:36");

    //Serial.println(
    //    F("PS4 controller interface started.")
    //);

    DiffSteer.begin(fPivYLimit);

    oledReady();

    Serial.println(F("Ready."));
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------

void loop()
{
    // Fail safe if the PS4 controller is disconnected.
    if (!PS4.isConnected())
    {
        stopMotors();
        digitalWrite(onboardled, LOW);
        return;
    }

    // -------------------------------------------------------------------------
    // Buttons
    // -------------------------------------------------------------------------

    if (PS4.Share())
    {
        Serial.println(F("Share Button"));

        assist = !assist;

        delay(100);
    }

    digitalWrite(
        onboardled,
        assist ? HIGH : LOW
    );

    if (PS4.Options())
    {
        Serial.println(F("Options Button"));

        if (mode == "angles")
        {
            mode = "mspeed";
        }
        else if (mode == "mspeed")
        {
            mode = "valadjust";
        }
        else
        {
            mode = "angles";
        }

        delay(100);
    }

    if (PS4.Triangle())
    {
        Serial.printf(
            "Battery Level: %d\n",
            PS4.Battery()
        );

        if (PS4.Charging())
        {
            Serial.println(
                F("The controller is charging")
            );
        }

        delay(100);
    }

    // -------------------------------------------------------------------------
    // Steering input
    // -------------------------------------------------------------------------

    LStickXvalue = PS4.LStickX();

    int y = 0;
    if (PS4.R2()) {
        y = map(PS4.R2Value(), 0, 255, 0, 127);
    }
    if (PS4.L2()) {
        y = map(PS4.L2Value(), 0, 255, 0, -127);
    }

    tracked_stick_to_motors(DiffSteer, LStickXvalue, y, l_speed, r_speed);
    XValue = tracked_stick_deadzone((LStickXvalue <= -128) ? -127 : LStickXvalue, 8);
    YValue = y;

    // -------------------------------------------------------------------------
    // Gyro assist
    // -------------------------------------------------------------------------

#if MPU_ENABLED

    if ((!XValue) && assist)
    {
        if (prevXValue)
        {
            mpu.update();
            lockangle = (int16_t)mpu.getAngleZ();
        }

        mpu.update();

        gz = (int16_t)mpu.getGyroZ();
        az = (int)mpu.getAngleZ();

        adjangle = map(
            az - lockangle,
            -adj_map,
            adj_map,
            -adj_out,
            adj_out
        );

        adjangle = constrain(
            adjangle,
            -20,
            20
        );

        l_speed = constrain(
            l_speed + adjangle,
            -128,
            128
        );

        r_speed = constrain(
            r_speed - adjangle,
            -128,
            128
        );
    }
    else
    {
        adjangle = 0;
    }

#else

    adjangle = 0;
    az = 0;
    gz = 0;

#endif

    // -------------------------------------------------------------------------
    // OLED display
    // -------------------------------------------------------------------------

#if OLED_ENABLED

    if (mode == "angles")
    {
        display.clearDisplay();
        display.setTextSize(1);

        display.setCursor(0, 0);
        display.println(F("Angle Z: "));

        display.setCursor(60, 0);
        display.println(az);

        display.setCursor(0, 12);
        display.println(F("adjangle: "));

        display.setCursor(60, 12);
        display.println(adjangle);

        display.setCursor(0, 24);
        display.println(F("Lock A: "));

        display.setCursor(60, 24);
        display.println(lockangle);

        display.display();
    }
    else if (mode == "mspeed")
    {
        display.clearDisplay();
        display.setTextSize(1);

        display.setCursor(0, 0);
        display.println(F("L speed: "));

        display.setCursor(60, 0);
        display.println(l_speed);

        display.setCursor(0, 12);
        display.println(F("R speed: "));

        display.setCursor(60, 12);
        display.println(r_speed);

        display.setCursor(0, 24);
        display.println(F("adj: "));

        display.setCursor(60, 24);
        display.println(adjangle);

        display.display();
    }
    else if (mode == "valadjust")
    {
        if (PS4.Up())
        {
            Serial.println(F("Up Button"));
            adj_map++;
            delay(100);
        }
        else if (PS4.Down())
        {
            Serial.println(F("Down Button"));
            adj_map--;
            delay(100);
        }
        else if (PS4.Left())
        {
            Serial.println(F("Left Button"));
            adj_out--;
            delay(100);
        }
        else if (PS4.Right())
        {
            Serial.println(F("Right Button"));
            adj_out++;
            delay(100);
        }

        display.clearDisplay();
        display.setTextSize(1);

        display.setCursor(0, 0);
        display.println(F("Adj map: "));

        display.setCursor(60, 0);
        display.println(adj_map);

        display.setCursor(0, 12);
        display.println(F("Adj out: "));

        display.setCursor(60, 12);
        display.println(adj_out);

        display.setCursor(0, 24);
        display.println(F("adj: "));

        display.setCursor(60, 24);
        display.println(adjangle);

        display.display();
    }

#else

    // Adjustment controls remain available without the OLED.
    if (mode == "valadjust")
    {
        if (PS4.Up())
        {
            adj_map++;

            Serial.printf(
                "adj_map = %d\n",
                adj_map
            );

            delay(100);
        }
        else if (PS4.Down())
        {
            adj_map--;

            Serial.printf(
                "adj_map = %d\n",
                adj_map
            );

            delay(100);
        }
        else if (PS4.Left())
        {
            adj_out--;

            Serial.printf(
                "adj_out = %d\n",
                adj_out
            );

            delay(100);
        }
        else if (PS4.Right())
        {
            adj_out++;

            Serial.printf(
                "adj_out = %d\n",
                adj_out
            );

            delay(100);
        }
    }

#endif

    prevXValue = XValue;

    // -------------------------------------------------------------------------
    // Drive motors
    // -------------------------------------------------------------------------

    drive(
        l_speed,
        r_speed
    );
}
