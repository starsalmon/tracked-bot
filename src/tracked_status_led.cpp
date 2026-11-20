#include "tracked_status_led.h"

#include <Adafruit_NeoPixel.h>

namespace {

Adafruit_NeoPixel* g_px = nullptr;

}  // namespace

void TrackedStatusLed::begin(int data_pin, int pwr_pin) {
  _data_pin = data_pin;
  _pwr_pin = pwr_pin;
  if (_data_pin < 0 || _pwr_pin < 0) {
    return;
  }
  pinMode(_pwr_pin, OUTPUT);
  digitalWrite(_pwr_pin, HIGH);
  static Adafruit_NeoPixel pixel(1, _data_pin, NEO_GRB + NEO_KHZ800);
  g_px = &pixel;
  g_px->begin();
  g_px->setBrightness(kBrightness);
  g_px->clear();
  g_px->show();
  _show(36, 36, 36);
  const uint32_t now = millis();
  _boot_until = now + kBootMs;
  _network_grace_until = now + kNetworkGraceMs;
  _ready = true;
}

TrackedStatusLed::Phase TrackedStatusLed::_phase(uint32_t now_ms) const {
  if (now_ms < _boot_until) {
    return Phase::kBoot;
  }
  const bool net_grace = now_ms < _network_grace_until;
  if (!_in.wifi_connected || !_in.uros_transport) {
    return Phase::kWifiConnecting;
  }
  if (_in.agent_connected) {
    return Phase::kAgentOk;
  }
  if (_in.agent_reconnecting && !net_grace) {
    return Phase::kAgentLost;
  }
  if (net_grace) {
    return Phase::kWifiConnecting;
  }
  return Phase::kWifiNoAgent;
}

void TrackedStatusLed::_blank() {
  if (_pwr_pin >= 0) {
    digitalWrite(_pwr_pin, LOW);
  }
  if (g_px) {
    g_px->clear();
    g_px->show();
  }
}

void TrackedStatusLed::_show(uint8_t r, uint8_t g, uint8_t b) {
  if (!g_px || _pwr_pin < 0) {
    return;
  }
  if (r == 0 && g == 0 && b == 0) {
    _blank();
    return;
  }
  digitalWrite(_pwr_pin, HIGH);
  g_px->setPixelColor(0, r, g, b);
  g_px->show();
}

void TrackedStatusLed::_render(Phase phase, uint32_t now_ms) {
  bool on = true;
  if (phase == Phase::kWifiConnecting || phase == Phase::kAgentLost) {
    on = ((now_ms / 400) % 2) == 0;
  } else if (phase == Phase::kWifiNoAgent) {
    on = ((now_ms / 1200) % 2) == 0;
  }

  uint8_t r = 0, g = 0, b = 0;
  switch (phase) {
    case Phase::kBoot:
      r = g = b = 36;
      break;
    case Phase::kWifiConnecting:
      if (on) {
        r = 160;
        g = 70;
      }
      break;
    case Phase::kWifiNoAgent:
      if (on) {
        b = 90;
      }
      break;
    case Phase::kAgentOk:
      g = 32;
      break;
    case Phase::kAgentLost:
      if (on) {
        r = 120;
      }
      break;
  }
  _show(r, g, b);
}

void TrackedStatusLed::tick(uint32_t now_ms) {
  if (!_ready || _data_pin < 0) {
    return;
  }

  if (_ota_active) {
    if (!_ota_rendered || (now_ms - _last_show_ms) >= 40) {
      _ota_rendered = true;
      _last_show_ms = now_ms;
      _show(32, 16, 0);
    }
    return;
  }
  if (_ota_rendered) {
    _ota_rendered = false;
    _last_phase = Phase::kBoot;
    _last_on = false;
    _last_show_ms = 0;
  }

  const Phase phase = _phase(now_ms);
  const bool fast = (phase == Phase::kWifiConnecting || phase == Phase::kAgentLost);
  const bool slow = (phase == Phase::kWifiNoAgent);
  const bool on =
      (!fast && !slow) ? true : (((now_ms / (fast ? 400 : 1200)) % 2) == 0);

  if (phase == _last_phase && on == _last_on && (now_ms - _last_show_ms) < 40) {
    return;
  }

  _last_phase = phase;
  _last_on = on;
  _last_show_ms = now_ms;
  _render(phase, now_ms);
}