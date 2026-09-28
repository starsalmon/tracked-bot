#include "tracked_speaker.h"

namespace {

constexpr uint8_t kDutyBits = 8;
constexpr uint8_t kDutyOn = (1u << kDutyBits) / 3;
constexpr uint16_t kNoteGapMs = 5;
constexpr float kVol = 0.45f;

struct Note {
  uint16_t hz;
  uint16_t ms;
};

struct MelodyDef {
  Note notes[8];
  uint8_t count;
};

#define NOTE(hz, ms) \
  { static_cast<uint16_t>(hz), static_cast<uint16_t>(ms) }

enum : uint16_t {
  E4 = 330,
  G4 = 392,
  B4 = 494,
  E5 = 659,
  D5 = 587,
  Fs5 = 740,
  A5 = 880,
};

static const MelodyDef kMelodies[] = {
    {},
    // startup — short rising wake
    {{NOTE(E4, 50), NOTE(G4, 50), NOTE(B4, 62), NOTE(E5, 78)}, 4},
    // fleet_ping — bleep bloop ping when another bot's fleet IR ID is seen
    {{NOTE(D5, 36), NOTE(Fs5, 36), NOTE(A5, 58)}, 3},
};

#undef NOTE

}  // namespace

bool TrackedSpeaker::begin(int pin, int8_t ledc_channel) {
  _pin = pin;
  _ch = ledc_channel;
  _ok = false;
  if (_pin < 0) {
    return false;
  }
  ledcDetach(static_cast<uint8_t>(_pin));
  if (!ledcAttachChannel(static_cast<uint8_t>(_pin), 1000, kDutyBits, _ch)) {
    return false;
  }
  ledcWrite(static_cast<uint8_t>(_pin), 0);
  _ok = true;
  return true;
}

void TrackedSpeaker::silence_pwm() {
  if (!_ok) {
    return;
  }
  ledcWriteTone(static_cast<uint8_t>(_pin), 0);
  ledcWrite(static_cast<uint8_t>(_pin), 0);
}

void TrackedSpeaker::start_melody(uint8_t melody_id) {
  if (!_ok || melody_id == 0 || melody_id >= sizeof(kMelodies) / sizeof(kMelodies[0])) {
    return;
  }
  const MelodyDef& def = kMelodies[melody_id];
  if (def.count == 0) {
    return;
  }
  _melody = reinterpret_cast<const TrackedSpeaker::Note*>(def.notes);
  _melody_len = def.count;
  _note_idx = 0;
  _in_gap = false;
  _note_until_ms = 0;
}

void TrackedSpeaker::play(uint8_t melody_id) { start_melody(melody_id); }

void TrackedSpeaker::tick(uint32_t now_ms) {
  if (!_ok || _melody == nullptr) {
    return;
  }
  if (_note_until_ms != 0 && now_ms < _note_until_ms) {
    return;
  }
  if (_in_gap) {
    _in_gap = false;
    silence_pwm();
    _note_idx++;
    if (_note_idx >= _melody_len) {
      _melody = nullptr;
      return;
    }
    _note_until_ms = now_ms + kNoteGapMs;
    if (now_ms < _note_until_ms) {
      return;
    }
  }
  if (_note_idx >= _melody_len) {
    _melody = nullptr;
    silence_pwm();
    return;
  }
  const Note& n = _melody[_note_idx];
  if (n.hz == 0) {
    silence_pwm();
  } else {
    ledcWriteTone(static_cast<uint8_t>(_pin), n.hz);
    ledcWrite(static_cast<uint8_t>(_pin), static_cast<uint32_t>(kDutyOn * kVol));
  }
  _note_until_ms = now_ms + n.ms;
  _in_gap = true;
}
