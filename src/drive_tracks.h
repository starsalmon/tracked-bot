#pragma once

#include <Arduino.h>

/** DRV8833 dual H-bridge — time-based wheel ramp, ROS twist mix. */
class DriveTracks {
 public:
  bool begin();
  void stop();
  void set_twist(float linear, float angular);
  void set_wheel_speeds(float left, float right);
  void tick();
  bool moving() const;
  void sleep_bridge(bool sleep);

 private:
  void set_targets(float left, float right);
  void write_side(int in1_ch, int in2_ch, float speed);
  uint8_t speed_to_duty(float speed) const;
  float ramp_toward(float cur, float tgt, float dt) const;

  bool _ok = false;
  uint32_t _last_ms = 0;
  float _tgt_l = 0.0f;
  float _tgt_r = 0.0f;
  float _cur_l = 0.0f;
  float _cur_r = 0.0f;
};
