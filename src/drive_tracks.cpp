#include "drive_tracks.h"

#include "tracked_pins.h"

#include <math.h>

namespace {
constexpr float kEps = 0.02f;
}  // namespace

bool DriveTracks::begin() {
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);

  // 2 track motors × IN1/IN2 = 4 PWM pins (shared LEDC timer at TRACK_PWM_HZ).
  if (!ledcAttach(MOTOR_A_IN1, TRACK_PWM_HZ, TRACK_PWM_BITS) ||
      !ledcAttach(MOTOR_A_IN2, TRACK_PWM_HZ, TRACK_PWM_BITS) ||
      !ledcAttach(MOTOR_B_IN1, TRACK_PWM_HZ, TRACK_PWM_BITS) ||
      !ledcAttach(MOTOR_B_IN2, TRACK_PWM_HZ, TRACK_PWM_BITS)) {
    return false;
  }

  _ok = true;
  _last_ms = millis();
  stop();
  return true;
}

void DriveTracks::sleep_bridge(bool sleep) {
  (void)sleep;  // nSLEEP tied HIGH in hardware — no GPIO
}

void DriveTracks::stop() {
  _tgt_l = _tgt_r = _cur_l = _cur_r = 0.0f;
  if (!_ok) {
    return;
  }
  ledcWrite(MOTOR_A_IN1, 0);
  ledcWrite(MOTOR_A_IN2, 0);
  ledcWrite(MOTOR_B_IN1, 0);
  ledcWrite(MOTOR_B_IN2, 0);
}

bool DriveTracks::moving() const {
  return fabsf(_cur_l) > kEps || fabsf(_cur_r) > kEps;
}

void DriveTracks::set_twist(float linear, float angular) {
  float left = linear - angular;
  float right = linear + angular;
  const float peak = fmaxf(fabsf(left), fabsf(right));
  if (peak > 1.0f) {
    left /= peak;
    right /= peak;
  }
  set_targets(left, right);
}

void DriveTracks::set_wheel_speeds(float left, float right) {
  set_targets(left, right);
}

void DriveTracks::set_targets(float left, float right) {
#if TRACK_INVERT_LEFT
  left = -left;
#endif
#if TRACK_INVERT_RIGHT
  right = -right;
#endif
  if (left > 1.0f) left = 1.0f;
  if (left < -1.0f) left = -1.0f;
  if (right > 1.0f) right = 1.0f;
  if (right < -1.0f) right = -1.0f;
  _tgt_l = left;
  _tgt_r = right;
}

uint8_t DriveTracks::speed_to_duty(float speed) const {
  speed = fabsf(speed);
  if (speed <= kEps) {
    return 0;
  }
  const long duty = map(static_cast<long>(speed * 128.0f), 2, 128, TRACK_PWM_MIN, TRACK_PWM_MAX);
  if (duty < 0) {
    return 0;
  }
  if (duty > TRACK_PWM_MAX) {
    return TRACK_PWM_MAX;
  }
  return static_cast<uint8_t>(duty);
}

void DriveTracks::write_side(int in1_pin, int in2_pin, float speed) {
  if (speed > kEps) {
    ledcWrite(in1_pin, speed_to_duty(speed));
    ledcWrite(in2_pin, 0);
  } else if (speed < -kEps) {
    ledcWrite(in1_pin, 0);
    ledcWrite(in2_pin, speed_to_duty(speed));
  } else {
    ledcWrite(in1_pin, 0);
    ledcWrite(in2_pin, 0);
  }
}

float DriveTracks::ramp_toward(float cur, float tgt, float dt) const {
  if (cur * tgt < -kEps * kEps) {
    tgt = 0.0f;  // bleed through zero before reverse
  }
  const float rate = (fabsf(tgt) < fabsf(cur)) ? TRACK_BRAKE_PER_S : TRACK_ACCEL_PER_S;
  const float step = rate * dt;
  float delta = tgt - cur;
  if (delta > step) delta = step;
  if (delta < -step) delta = -step;
  return cur + delta;
}

void DriveTracks::tick() {
  if (!_ok) {
    return;
  }
  const uint32_t now = millis();
  float dt = static_cast<float>(now - _last_ms) * 0.001f;
  _last_ms = now;
  if (dt <= 0.0f || dt > 0.12f) {
    dt = 0.02f;
  }
  _cur_l = ramp_toward(_cur_l, _tgt_l, dt);
  _cur_r = ramp_toward(_cur_r, _tgt_r, dt);
  write_side(MOTOR_A_IN1, MOTOR_A_IN2, _cur_l);
  write_side(MOTOR_B_IN1, MOTOR_B_IN2, _cur_r);
}
