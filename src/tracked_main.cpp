#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <limits>
#include <micro_ros_platformio.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <geometry_msgs/msg/twist.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <sensor_msgs/msg/illuminance.h>
#include <sensor_msgs/msg/imu.h>
#include <sensor_msgs/msg/range.h>
#include <std_msgs/msg/bool.h>
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/u_int8.h>

#include "fleet_ir_proto.h"
#include "fleet_ir_ping.h"
#include "fleet_ir_rx.h"
#include "drive_tracks.h"
#include "ir_tx.h"
#include "tracked_als.h"
#include "tracked_imu.h"
#include "tracked_ota.h"
#include "tracked_pins.h"
#include "tracked_speaker.h"
#include "tracked_tof.h"

#if defined(TRACKED_PS4)
#include <PS4Controller.h>
#include "DifferentialSteering.h"
#include "tracked_stick.h"
#endif
#ifndef BOT_NAMESPACE
#define BOT_NAMESPACE "bot2"
#endif
#ifndef BOT_NODE_NAME
#define BOT_NODE_NAME "tracked_bot"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID "changeme"
#define WIFI_PSK "changeme"
#define AGENT_IP "192.168.1.1"
#endif
#ifndef AGENT_PORT
#define AGENT_PORT 8888
#endif
#ifndef BOT_OTA_HOSTNAME
#define BOT_OTA_HOSTNAME "tracked-bot"
#endif

namespace {

constexpr uint32_t LOOP_MS = 20;
constexpr uint32_t AGENT_WAIT_MS = 60000;
constexpr uint32_t OLED_PERIOD_MS = 250;
constexpr uint32_t IMU_PERIOD_MS = 20;

IrTx ir_beacon;
FleetIrRx fleet_peer_rx;
FleetIrPing fleet_ir_ping;
#if SPEAKER_ENABLED
TrackedSpeaker speaker;
#endif

void ir_pins_begin() {
  pinMode(IR_RX_PIN, INPUT_PULLUP);
  pinMode(IR_SIDE_L_PIN, INPUT_PULLUP);
  pinMode(IR_SIDE_R_PIN, INPUT_PULLUP);
  if (ir_beacon.begin(IR_TX_PIN, IR_TX_HZ)) {
    ir_beacon.set_enabled(IR_TX_DEFAULT_ON);
    Serial.printf("IR TX fleet id %d on GPIO %d\n", FLEET_IR_ID, IR_TX_PIN);
  } else {
    Serial.println("WARN: IR TX PWM init failed");
  }
}

bool comm_ir_active() { return digitalRead(IR_RX_PIN) == LOW; }

// TSOP sees our own GPIO8 beacon — ignore RX while we are in a TX frame.
bool fleet_rx_carrier() {
  if (ir_beacon.in_tx_frame()) {
    return false;
  }
  return comm_ir_active();
}

bool side_ir_hit() {
  return digitalRead(IR_SIDE_L_PIN) == LOW || digitalRead(IR_SIDE_R_PIN) == LOW;
}

DriveTracks drive;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool oled_ok = false;
TrackedTof tof;
Mpu6050Imu imu;
bool imu_ok = false;
TrackedAls als;
#if defined(TRACKED_PS4)
DifferentialSteering diff_steer;
#endif
bool ps4_owns = false;

float last_lin = 0.0f;
float last_ang = 0.0f;
uint32_t last_cmd_ms = 0;
uint32_t drive_arm_at_ms = 0;
float vbat_v = -1.0f;
float pack2_v = -1.0f;
bool boost_5v = false;

rcl_publisher_t imu_pub;
rcl_publisher_t ir_detected_pub;
rcl_publisher_t ir_peer_pub;
rcl_publisher_t stall_pub;
rcl_publisher_t battery_pub;
rcl_publisher_t pack2_pub;
rcl_publisher_t boost_pub;
rcl_publisher_t range_pub;
rcl_publisher_t als_pub;
rcl_subscription_t cmd_sub;
sensor_msgs__msg__Imu imu_msg;
std_msgs__msg__Bool ir_detected_msg;
std_msgs__msg__UInt8 ir_peer_msg;
std_msgs__msg__Bool stall_msg;
std_msgs__msg__Float32 battery_msg;
std_msgs__msg__Float32 pack2_msg;
std_msgs__msg__Bool boost_msg;
sensor_msgs__msg__Range range_msg;
sensor_msgs__msg__Illuminance als_msg;
geometry_msgs__msg__Twist cmd_msg;

rcl_allocator_t allocator;
rclc_support_t support;
rcl_node_t node;
rclc_executor_t executor;
bool ros_ok = false;
uint8_t ros_init_level = 0;

#define ROS_LOG_FAIL(name, rc) \
  Serial.printf("ROS fail %s: %d\n", (name), static_cast<int>(rc))

void status_rgb(uint8_t r, uint8_t g, uint8_t b) {
  pinMode(RGB_PWR_PIN, OUTPUT);
  if (r == 0 && g == 0 && b == 0) {
    rgbLedWrite(RGB_DATA_PIN, 0, 0, 0);
    digitalWrite(RGB_PWR_PIN, LOW);
    return;
  }
  digitalWrite(RGB_PWR_PIN, HIGH);
  rgbLedWrite(RGB_DATA_PIN, r, g, b);
}

void apply_drive() {
  if (ps4_owns) {
    return;
  }
  if (drive_arm_at_ms == 0 || (millis() - drive_arm_at_ms) < DRIVE_ARM_MS) {
    drive.stop();
    return;
  }
  if (millis() - last_cmd_ms > CMD_TIMEOUT_MS) {
    drive.set_twist(0.0f, 0.0f);
    return;
  }
  float lin = last_lin;
  if (tof.ok() && tof.range_m() > 0.0f && tof.range_m() < TOF_STOP_M && lin > 0.02f) {
    lin = 0.0f;
  }
  drive.set_twist(lin, last_ang);
}

void adc_begin_pin(int pin) {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // ~0–3.1 V at the pin (not 0 dB ~1.1 V)
  (void)analogReadMilliVolts(pin);  // attach ADC channel first
  analogSetPinAttenuation(pin, ADC_11db);
}

float read_adc_scaled(int pin, float divider, float cal) {
  analogSetPinAttenuation(pin, ADC_11db);
  uint32_t mv_sum = 0;
  constexpr int kN = 8;
  for (int i = 0; i < kN; i++) {
    mv_sum += analogReadMilliVolts(pin);
  }
  const float vadc = (mv_sum / static_cast<float>(kN)) * 0.001f;
  if (vadc < 0.05f) {
    return -1.0f;
  }
  return vadc * divider * cal;
}

float read_vbat() {
  return read_adc_scaled(VBAT_PIN, VBAT_DIVIDER, VBAT_CAL);
}

float read_pack2() {
  return read_adc_scaled(PACK2_PIN, PACK2_DIVIDER, PACK2_CAL);
}

bool read_boost_5v() {
  // GPIO10 is VBUS sense, not an ADC pin on C6 — analogRead here panics.
  return digitalRead(VBUS_SENSE_PIN) == HIGH;
}

bool tick_ps4() {
#if !defined(TRACKED_PS4)
  ps4_owns = false;
  return false;
#else
  if (!PS4.isConnected()) {
    if (ps4_owns) {
      drive.set_twist(0.0f, 0.0f);
    }
    ps4_owns = false;
    return false;
  }
  ps4_owns = true;
  int y = 0;
  if (PS4.R2()) {
    y = map(PS4.R2Value(), 0, 255, 0, 127);
  }
  if (PS4.L2()) {
    y = map(PS4.L2Value(), 0, 255, 0, -127);
  }
  int left = 0;
  int right = 0;
  tracked_stick_to_motors(diff_steer, PS4.LStickX(), y, left, right);
  drive.set_wheel_speeds(left / 127.0f, right / 127.0f);
  return true;
#endif
}

void on_cmd(const void* msgin) {
  const auto* msg = static_cast<const geometry_msgs__msg__Twist*>(msgin);
  last_lin = static_cast<float>(msg->linear.x);
  last_ang = static_cast<float>(msg->angular.z);
  last_cmd_ms = millis();
}

void destroy_entities();

bool create_entities() {
  allocator = rcl_get_default_allocator();
  rcl_ret_t rc = rclc_support_init(&support, 0, nullptr, &allocator);
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("rclc_support_init", rc);
    return false;
  }
  ros_init_level = 1;

  rc = rclc_node_init_default(&node, BOT_NODE_NAME, BOT_NAMESPACE, &support);
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("rclc_node_init_default", rc);
    destroy_entities();
    return false;
  }
  ros_init_level = 2;

  rc = rclc_publisher_init_default(&imu_pub, &node,
                                   ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu), "imu");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub imu", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(
      &ir_detected_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), "ir/detected");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub ir/detected", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(
      &ir_peer_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt8), "ir/peer");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub ir/peer", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(&stall_pub, &node,
                                   ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), "stall");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub stall", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(
      &battery_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), "battery/voltage");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub battery/voltage", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(
      &pack2_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), "battery/motor_voltage");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub battery/motor_voltage", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(
      &boost_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), "power/boost_5v");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub power/boost_5v", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(
      &range_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range), "sonar/range");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub sonar/range", rc);
    destroy_entities();
    return false;
  }
  rc = rclc_publisher_init_default(
      &als_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Illuminance), "als/illuminance");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("pub als/illuminance", rc);
    destroy_entities();
    return false;
  }
  ros_init_level = 3;

  rc = rclc_subscription_init_default(
      &cmd_sub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel");
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("sub cmd_vel", rc);
    destroy_entities();
    return false;
  }
  ros_init_level = 4;

  rc = rclc_executor_init(&executor, &support.context, 1, &allocator);
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("rclc_executor_init", rc);
    destroy_entities();
    return false;
  }
  ros_init_level = 5;

  rc = rclc_executor_add_subscription(&executor, &cmd_sub, &cmd_msg, &on_cmd, ON_NEW_DATA);
  if (rc != RCL_RET_OK) {
    ROS_LOG_FAIL("executor add cmd_vel", rc);
    destroy_entities();
    return false;
  }
  ros_init_level = 6;

  imu_msg.orientation_covariance[0] = -1.0;
  static char imu_frame[] = "imu_link";
  imu_msg.header.frame_id.data = imu_frame;
  imu_msg.header.frame_id.size = sizeof(imu_frame) - 1;

  static char range_frame[] = "tof_link";
  range_msg.header.frame_id.data = range_frame;
  range_msg.header.frame_id.size = sizeof(range_frame) - 1;
  range_msg.radiation_type = sensor_msgs__msg__Range__INFRARED;
  range_msg.field_of_view = 0.44f;
  range_msg.min_range = 0.03f;
  range_msg.max_range = 2.0f;

  static char als_frame[] = "als_link";
  als_msg.header.frame_id.data = als_frame;
  als_msg.header.frame_id.size = sizeof(als_frame) - 1;
  als_msg.variance = 0.0;

  ros_ok = true;
  return true;
}

void destroy_entities() {
  if (ros_init_level >= 6) {
    rclc_executor_fini(&executor);
  }
  if (ros_init_level >= 4) {
    rcl_subscription_fini(&cmd_sub, &node);
  }
  if (ros_init_level >= 3) {
    rcl_publisher_fini(&stall_pub, &node);
    rcl_publisher_fini(&ir_peer_pub, &node);
    rcl_publisher_fini(&ir_detected_pub, &node);
    rcl_publisher_fini(&imu_pub, &node);
    rcl_publisher_fini(&battery_pub, &node);
    rcl_publisher_fini(&pack2_pub, &node);
    rcl_publisher_fini(&boost_pub, &node);
    rcl_publisher_fini(&range_pub, &node);
    rcl_publisher_fini(&als_pub, &node);
  }
  if (ros_init_level >= 2) {
    rcl_node_fini(&node);
  }
  if (ros_init_level >= 1) {
    rclc_support_fini(&support);
  }
  ros_init_level = 0;
  ros_ok = false;
  drive_arm_at_ms = 0;
}

bool wait_for_agent(uint32_t timeout_ms) {
  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
#if defined(ENABLE_OTA)
    tracked_ota_tick();
#endif
    if (rmw_uros_ping_agent(50, 1) == RMW_RET_OK) {
      return true;
    }
    Serial.println("waiting for agent...");
    tick_ps4();
    drive.tick();
    delay(LOOP_MS);
  }
  return false;
}

bool connect_ros() {
  if (!wait_for_agent(AGENT_WAIT_MS)) {
    Serial.println("agent not reachable — check AGENT_IP, UDP 8888, same subnet");
    return false;
  }
  if (!create_entities()) {
    Serial.println("micro-ROS entity init failed — retrying in 2s");
    destroy_entities();
    delay(2000);
    return false;
  }
  if (!ps4_owns) {
    drive.stop();
  }
  drive_arm_at_ms = millis();
  Serial.printf("micro-ROS ready ns=/%s — drive locked %u ms\n", BOT_NAMESPACE, DRIVE_ARM_MS);
  return true;
}

void oled_draw(uint32_t now) {
#if OLED_ENABLED
  if (!oled_ok) {
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("/"));
  display.print(BOT_NAMESPACE);
  if (ps4_owns) {
    display.print(F("  PS4"));
  } else if (ros_ok) {
    display.print(F("  AUTO"));
  } else {
    display.print(F("  WIFI"));
  }
  if (drive.moving()) {
    display.print(F(" *"));
  }

  display.setCursor(0, 10);
  display.print(WiFi.status() == WL_CONNECTED ? F("WiFi ") : F("WiFi-- "));
  display.print(WiFi.localIP());

  display.setCursor(0, 20);
  display.print(boost_5v ? F("5V ") : F("bat "));
  if (vbat_v > 0.5f) {
    display.printf("E%.2f", vbat_v);
  } else {
    display.print(F("E--"));
  }
  display.print(' ');
  if (pack2_v > 0.5f) {
    display.printf("M%.2f", pack2_v);
  } else {
    display.print(F("M--"));
  }

  display.setCursor(0, 30);
  if (tof.ok() && tof.range_m() > 0.0f) {
    display.printf("ToF %.2fm", tof.range_m());
  } else if (tof.present()) {
    display.print(F("ToF ?"));
  } else {
    display.print(F("ToF --"));
  }
  if (als.ok() && als.lux() >= 0.0f) {
    display.printf(" %dlx", static_cast<int>(als.lux() + 0.5f));
  }

  display.setCursor(0, 40);
  display.printf("v %.2f  w %.2f", last_lin, last_ang);

  display.setCursor(0, 50);
  if (ps4_owns) {
    display.print(F("pad"));
  } else {
    const bool cmd = (now - last_cmd_ms) <= CMD_TIMEOUT_MS;
    display.print(cmd ? F("cmd") : F("idle"));
  }
  display.print(F("  "));
  display.print(BOT_OTA_HOSTNAME);
  display.display();
#endif
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("*** FIRMWARE: tracked_wifi TinyC6 (ROS /cmd_vel + OTA) ***");

  pinMode(RGB_PWR_PIN, OUTPUT);
  digitalWrite(RGB_PWR_PIN, HIGH);
  status_rgb(0, 0, 32);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#if OLED_ENABLED
  oled_ok = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (oled_ok) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("tracked-bot"));
    display.println(F("booting..."));
    display.display();
  } else {
    Serial.println("SSD1306 not detected");
  }
#endif

  pinMode(VBUS_SENSE_PIN, INPUT);
  pinMode(SOLAR_SERVO_PIN, INPUT);
  adc_begin_pin(VBAT_PIN);
  adc_begin_pin(PACK2_PIN);
  imu_ok = imu.begin(MPU6050_ADDR);
  if (!imu_ok) {
    Serial.println("WARN: MPU6050 not found on I2C 6/7");
  } else {
    Serial.println("MPU6050 OK");
  }
  tof.begin();
  if (!als.begin(BH1750_ADDR)) {
    Serial.println("WARN: BH1750 not found on I2C 6/7");
  } else {
    Serial.printf("BH1750 OK  %.0f lx\n", static_cast<double>(als.lux()));
  }

  if (!drive.begin()) {
    Serial.println("WARN: DRV8833 PWM init failed");
  }
  ir_pins_begin();  // after motor LEDC — C6 has only 6 PWM channels
#if SPEAKER_ENABLED
  if (speaker.begin(SPEAKER_PIN, SPEAKER_LEDC_CH)) {
    speaker.play(TrackedSpeaker::kMelodyStartup);
    Serial.printf("Speaker GPIO %d ch %d\n", SPEAKER_PIN, SPEAKER_LEDC_CH);
    const uint32_t tune_end = millis() + 280;
    while (millis() < tune_end) {
      speaker.tick(millis());
      delay(1);
    }
  } else {
    Serial.printf("WARN: speaker PWM failed GPIO %d ch %d\n", SPEAKER_PIN, SPEAKER_LEDC_CH);
  }
#endif
#if defined(TRACKED_PS4)
#ifndef TRACKED_PS4_MAC
#define TRACKED_PS4_MAC "AC:3E:B1:0A:5B:3C"
#endif
  diff_steer.begin(32);

  // Classic BT must be up *before* WiFi (legacy NodeMCU-32S only).
  Serial.printf("PS4.begin %s (BT before WiFi)\n", TRACKED_PS4_MAC);
  if (!PS4.begin(TRACKED_PS4_MAC)) {
    Serial.println("PS4.begin failed — pad will not connect");
  }
#endif

#if defined(ENABLE_OTA)
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(BOT_OTA_HOSTNAME);
#endif
  IPAddress agent_ip;
  if (!agent_ip.fromString(AGENT_IP)) {
    Serial.printf("WARN: AGENT_IP parse failed: '%s'\n", AGENT_IP);
    agent_ip = IPAddress(192, 168, 2, 31);
  }
  set_microros_wifi_transports(WIFI_SSID, WIFI_PSK, agent_ip, AGENT_PORT);
#if defined(ENABLE_OTA)
  WiFi.setSleep(WIFI_PS_NONE);
#else
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
#endif
  delay(2000);
  Serial.printf("WiFi → agent %s:%d\n", AGENT_IP, AGENT_PORT);
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());
#if defined(ENABLE_OTA)
  tracked_ota_begin();
#endif
  Serial.printf("micro-ROS ns=/%s node=%s\n", BOT_NAMESPACE, BOT_NODE_NAME);
}

void loop() {
  static uint32_t last_ping = 0;
  static uint32_t last_oled = 0;
  static uint32_t last_ir = 0;
  static uint32_t last_tof = 0;
  static uint32_t last_vbat = 0;
  static uint32_t last_imu = 0;
  static uint32_t last_als = 0;
  const uint32_t now = millis();

#if SPEAKER_ENABLED
  speaker.tick(now);
#endif

#if defined(ENABLE_OTA)
  tracked_ota_tick();
  if (tracked_ota_busy()) {
    drive.stop();
    status_rgb(32, 16, 0);
    delay(10);
    return;
  }
#endif

  tick_ps4();

  if (now - last_tof >= 50) {
    last_tof = now;
    tof.poll();
  }
  if (als.ok() && now - last_als >= 200) {
    last_als = now;
    als.poll();
  }
  if (now - last_vbat >= 400) {
    last_vbat = now;
    vbat_v = read_vbat();
    pack2_v = read_pack2();
    boost_5v = read_boost_5v();
  }

  fleet_ir_service([&]() { ir_beacon.tick(); },
                   [&]() { fleet_peer_rx.poll(fleet_rx_carrier()); });

  if (!ros_ok) {
    connect_ros();
    if (!ps4_owns) {
      drive.stop();
    }
    drive.tick();
    oled_draw(now);
    delay(LOOP_MS);
    return;
  }

  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(25));

  if (now - last_ping > 1000) {
    last_ping = now;
    static uint8_t agent_miss = 0;
    if (rmw_uros_ping_agent(100, 1) != RMW_RET_OK) {
      if (++agent_miss >= 3) {
        Serial.println("agent lost");
        agent_miss = 0;
        destroy_entities();
        if (!ps4_owns) {
          drive.stop();
        }
        return;
      }
    } else {
      agent_miss = 0;
    }
  }

  if (imu_ok && now - last_imu >= IMU_PERIOD_MS) {
    last_imu = now;
    ImuSample sample;
    if (imu.read(sample)) {
      imu_msg.header.stamp.sec = static_cast<int32_t>(now / 1000);
      imu_msg.header.stamp.nanosec = static_cast<uint32_t>((now % 1000) * 1000000UL);
      imu_msg.linear_acceleration.x = sample.ax;
      imu_msg.linear_acceleration.y = sample.ay;
      imu_msg.linear_acceleration.z = sample.az;
      imu_msg.angular_velocity.x = sample.gx;
      imu_msg.angular_velocity.y = sample.gy;
      imu_msg.angular_velocity.z = sample.gz;
      rcl_publish(&imu_pub, &imu_msg, nullptr);
    }
  }

  apply_drive();
  drive.tick();
  if (drive.moving()) {
    status_rgb(0, 48, 0);
  } else if (ros_ok) {
    status_rgb(0, 0, 16);
  } else {
    status_rgb(16, 8, 0);
  }

  if (now - last_ir > 100) {
    last_ir = now;
    const bool peer_seen = fleet_peer_rx.detected();
    const uint8_t peer_id = peer_seen ? fleet_peer_rx.peer_id() : FLEET_IR_PEER_NONE;
    ir_detected_msg.data = fleet_rx_carrier() || side_ir_hit() || peer_seen;
    ir_peer_msg.data = peer_id;
#if SPEAKER_ENABLED
    if (fleet_ir_ping.should_ping(peer_seen, peer_id, FLEET_IR_ID, now)) {
      speaker.play(TrackedSpeaker::kMelodyFleetPing);
    }
#endif
    stall_msg.data = false;
    rcl_publish(&ir_detected_pub, &ir_detected_msg, nullptr);
    rcl_publish(&ir_peer_pub, &ir_peer_msg, nullptr);
    rcl_publish(&stall_pub, &stall_msg, nullptr);
    battery_msg.data = vbat_v;
    rcl_publish(&battery_pub, &battery_msg, nullptr);
    pack2_msg.data = pack2_v;
    rcl_publish(&pack2_pub, &pack2_msg, nullptr);
    boost_msg.data = boost_5v;
    rcl_publish(&boost_pub, &boost_msg, nullptr);
    if (tof.ok() && tof.range_m() > 0.0f) {
      range_msg.range = tof.range_m();
      rcl_publish(&range_pub, &range_msg, nullptr);
    }
    if (als.ok() && als.lux() >= 0.0f) {
      als_msg.illuminance = static_cast<double>(als.lux());
      als_msg.header.stamp.sec = static_cast<int32_t>(now / 1000);
      als_msg.header.stamp.nanosec = static_cast<uint32_t>((now % 1000) * 1000000UL);
      rcl_publish(&als_pub, &als_msg, nullptr);
    }
  }

  if (now - last_oled >= OLED_PERIOD_MS) {
    last_oled = now;
    oled_draw(now);
  }

  delay(LOOP_MS);
}
