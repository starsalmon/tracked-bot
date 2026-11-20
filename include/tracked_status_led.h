#pragma once

#include <Arduino.h>

// Onboard WS2812 (Unexpected Maker TinyS3[D]): data GPIO 18, rail GPIO 17 (NEOPIXEL_POWER).

struct TrackedStatusLedInput {
  bool wifi_connected = false;
  bool uros_transport = false;       // Wi-Fi up and XRCE transport configured
  bool agent_connected = false;
  bool agent_reconnecting = false;  // reconnecting after a prior ROS session
};

class TrackedStatusLed {
 public:
  void begin(int data_pin, int pwr_pin);
  void set_input(const TrackedStatusLedInput& in) { _in = in; }
  void set_ota_active(bool active) { _ota_active = active; }
  void tick(uint32_t now_ms);

 private:
  enum class Phase : uint8_t {
    kBoot,
    kWifiConnecting,
    kWifiNoAgent,
    kAgentOk,
    kAgentLost,
  };

  static constexpr uint32_t kBootMs = 350;
  static constexpr uint32_t kNetworkGraceMs = 45000;
  static constexpr uint8_t kBrightness = 20;

  int _data_pin = -1;
  int _pwr_pin = -1;
  TrackedStatusLedInput _in;
  uint32_t _boot_until = 0;
  uint32_t _network_grace_until = 0;
  bool _ready = false;
  bool _ota_active = false;
  bool _ota_rendered = false;
  Phase _last_phase = Phase::kBoot;
  bool _last_on = false;
  uint32_t _last_show_ms = 0;

  Phase _phase(uint32_t now_ms) const;
  void _render(Phase phase, uint32_t now_ms);
  void _show(uint8_t r, uint8_t g, uint8_t b);
  void _blank();
};