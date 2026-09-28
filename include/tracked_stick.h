#pragma once

#include "DifferentialSteering.h"

/** Shape DualShock stick + triggers into track speeds (−127…127).

  Old bug: values outside a hard box were *dropped* (hard-left −128 → stop).
  Now they are mapped/clamped.

  Parked (no real throttle): left stick spins in place; tiny trigger noise ignored.
  Rolling: steering is scaled down so a twitchy stick doesn’t yank the gearbox.
*/
inline int tracked_stick_deadzone(int v, int dz) {
  return (v > dz || v < -dz) ? v : 0;
}

inline void tracked_stick_to_motors(DifferentialSteering& steer, int raw_x, int raw_y, int& left,
                                    int& right) {
  // int8 stick: −128…127. Fold into compute range instead of rejecting.
  int x = constrain(raw_x, -128, 127);
  if (x == -128) {
    x = -127;
  }
  int y = constrain(raw_y, -127, 127);

  x = tracked_stick_deadzone(x, 8);
  y = tracked_stick_deadzone(y, 10);

  if (y == 0) {
    // Rotate in place — ignore leftover throttle; DiffSteer pivot at Y=0 is 100%.
  } else {
    // More throttle → less steer authority (straight crawl, not twitch).
    const float t = static_cast<float>(abs(y)) / 127.0f;
    const float steer_scale = 1.0f - 0.55f * t;
    x = static_cast<int>(x * steer_scale);
  }

  steer.computeMotors(x, y);
  left = steer.computedLeftMotor();
  right = steer.computedRightMotor();
}
