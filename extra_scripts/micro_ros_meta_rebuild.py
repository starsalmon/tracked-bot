#!/usr/bin/env python3
"""Force micro-ROS library rebuild when colcon meta changes."""
Import("env")
import hashlib
import os

meta = env.GetProjectConfig().get("env:" + env["PIOENV"], "board_microros_user_meta", "")
if not meta:
    Return()

meta_path = os.path.join(env["PROJECT_DIR"], meta)
if not os.path.isfile(meta_path):
    Return()

with open(meta_path, "rb") as f:
    digest = hashlib.sha256(f.read()).hexdigest()

stamp_dir = os.path.join(env["PROJECT_DIR"], ".pio", "build", env["PIOENV"])
stamp_path = os.path.join(stamp_dir, "microros_meta.sha256")
os.makedirs(stamp_dir, exist_ok=True)

prev = None
if os.path.isfile(stamp_path):
    with open(stamp_path, "r", encoding="utf-8") as f:
        prev = f.read().strip()

if prev != digest:
    build_dir = os.path.join(
        env["PROJECT_DIR"], ".pio", "libdeps", env["PIOENV"], "micro_ros_platformio", "build"
    )
    if os.path.isdir(build_dir):
        print("micro-ROS meta changed — wiping", build_dir)
        import shutil

        shutil.rmtree(build_dir)
    with open(stamp_path, "w", encoding="utf-8") as f:
        f.write(digest)
