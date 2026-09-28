Import("env")

import os
import shutil
from pathlib import Path


def _archive_looks_xtensa(archive):
    try:
        with open(archive, "rb") as f:
            blob = f.read(256 * 1024)
        return b"xtensa-esp32-elf" in blob
    except OSError:
        return False


def _reuse_or_drop_stale_microros():
    """USB env has its own libdeps folder. A leftover Xtensa libmicroros.a
    links as 'skipping incompatible' on C6. Reuse the OTA RISC-V build."""
    project = Path(env["PROJECT_DIR"])
    pioenv = env["PIOENV"]
    libdeps = project / ".pio" / "libdeps"
    this_uros = libdeps / pioenv / "micro_ros_platformio"
    this_a = this_uros / "libmicroros" / "libmicroros.a"

    sibling = None
    if pioenv.endswith("_usb"):
        sibling = libdeps / pioenv[: -len("_usb")] / "micro_ros_platformio"

    if sibling and (sibling / "libmicroros" / "libmicroros.a").is_file() and not _archive_looks_xtensa(
        sibling / "libmicroros" / "libmicroros.a"
    ):
        if this_uros.is_symlink() or this_uros.exists():
            if this_uros.is_symlink() and this_uros.resolve() == sibling.resolve():
                print("micro-ROS: USB env reusing", sibling.parent.name)
                return
            if this_uros.is_symlink() or this_uros.is_dir():
                if this_uros.is_symlink():
                    this_uros.unlink()
                else:
                    shutil.rmtree(this_uros)
        this_uros.parent.mkdir(parents=True, exist_ok=True)
        os.symlink(sibling, this_uros, target_is_directory=True)
        print("micro-ROS: linked", pioenv, "→", sibling.parent.name)
        return

    if this_a.is_file() and _archive_looks_xtensa(this_a):
        print("micro-ROS: dropping Xtensa libmicroros.a (need RISC-V for C6)")
        shutil.rmtree(this_uros / "libmicroros", ignore_errors=True)
        shutil.rmtree(this_uros / "build", ignore_errors=True)


_reuse_or_drop_stale_microros()


def _keep(chunk):
    chunk = str(chunk).strip().strip(";")
    if not chunk or chunk == ";":
        return False
    # Arduino-ESP32 3.x injects CHIP/Matter -D...=<path.h> which cmake then
    # feeds to /bin/sh as a redirect. Drop it; micro-ROS does not need it.
    if "CHIP_ADDRESS_RESOLVE" in chunk:
        return False
    if chunk.startswith("-D") and "<" in chunk and ">" in chunk:
        return False
    return True


def _flatten_env_flags(name):
    try:
        flat = [str(x) for x in env.Flatten(env.get(name, []))]
    except Exception:
        flat = []
        for item in env.get(name, []):
            if isinstance(item, (list, tuple)):
                flat.extend(str(x) for x in item)
            elif item is not None:
                flat.append(str(item))
    clean = []
    for part in flat:
        part = str(part).strip()
        if not part:
            continue
        for chunk in part.replace(";", " ").split():
            if _keep(chunk):
                clean.append(chunk)
    env.Replace(**{name: clean})


_flatten_env_flags("CFLAGS")
_flatten_env_flags("CCFLAGS")
_flatten_env_flags("CXXFLAGS")
_flatten_env_flags("ASFLAGS")


def _sanitize_joined(flags):
    parts = []
    for chunk in str(flags).replace(";", " ").split():
        if _keep(chunk):
            parts.append(chunk)
    return " ".join(parts)


def _patch_file(path, marker, old, new):
    if not path.is_file():
        return False
    text = path.read_text(encoding="utf-8")
    if marker in text:
        return True
    if old not in text:
        print("WARN: micro-ROS sanitize — pattern not found in", path)
        return False
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print("Patched", path.relative_to(Path(env["PROJECT_DIR"])))
    return True


lib_root = Path(env["PROJECT_DIR"]) / ".pio" / "libdeps" / env["PIOENV"] / "micro_ros_platformio"

_patch_file(
    lib_root / "microros_utils" / "library_builder.py",
    "TRACKED_BOT_CMAKE_FLAG_SANITIZE",
    "        cmake_toolchain = cmake_toolchain.format(C_COMPILER=cc, CXX_COMPILER=cxx, AR_COMPILER=ar, C_FLAGS=cflags, CXX_FLAGS=cxxflags)",
    """        # TRACKED_BOT_CMAKE_FLAG_SANITIZE
        def _sanitize_cmake_flags(flags):
            parts = []
            for chunk in str(flags).replace(";", " ").split():
                chunk = chunk.strip().strip(";")
                if not chunk:
                    continue
                if "CHIP_ADDRESS_RESOLVE" in chunk:
                    continue
                if "<" in chunk and ">" in chunk:
                    continue
                parts.append(chunk)
            return " ".join(parts)
        cflags = _sanitize_cmake_flags(cflags)
        cxxflags = _sanitize_cmake_flags(cxxflags)
        cmake_toolchain = cmake_toolchain.format(C_COMPILER=cc, CXX_COMPILER=cxx, AR_COMPILER=ar, C_FLAGS=cflags, CXX_FLAGS=cxxflags)""",
)

extra_script = lib_root / "extra_script.py"
_patch_file(
    extra_script,
    "TRACKED_BOT_EXTRA_FLAG_SANITIZE",
    """    cmake_toolchain = library_builder.CMakeToolchain(
        main_path + "/platformio_toolchain.cmake",
        env['CC'],
        env['CXX'],
        env['AR'],
        "{} {} -DCLOCK_MONOTONIC=0 -D'__attribute__(x)='".format(' '.join(env['CFLAGS']), ' '.join(env['CCFLAGS'])),
        "{} {} -fno-rtti -DCLOCK_MONOTONIC=0 -D'__attribute__(x)='".format(' '.join(env['CXXFLAGS']), ' '.join(env['CCFLAGS']))
    )""",
    """    def _sanitize_microros_flags(s):  # TRACKED_BOT_EXTRA_FLAG_SANITIZE
        parts = []
        for chunk in str(s).replace(";", " ").split():
            chunk = chunk.strip().strip(";")
            if not chunk:
                continue
            if "CHIP_ADDRESS_RESOLVE" in chunk:
                continue
            if chunk.startswith("-D") and "<" in chunk and ">" in chunk:
                continue
            parts.append(chunk)
        return " ".join(parts)
    cmake_toolchain = library_builder.CMakeToolchain(
        main_path + "/platformio_toolchain.cmake",
        env['CC'],
        env['CXX'],
        env['AR'],
        _sanitize_microros_flags("{} {} -DCLOCK_MONOTONIC=0 -D'__attribute__(x)='".format(' '.join(env['CFLAGS']), ' '.join(env['CCFLAGS']))),
        _sanitize_microros_flags("{} {} -fno-rtti -DCLOCK_MONOTONIC=0 -D'__attribute__(x)='".format(' '.join(env['CXXFLAGS']), ' '.join(env['CCFLAGS'])))
    )""",
)

# If a previous failed colcon left a poisoned toolchain file, strip it now
# (library extra_script overwrites it, but a leftover can confuse retries).
toolchain = lib_root / "platformio_toolchain.cmake"
if toolchain.is_file():
    raw = toolchain.read_text(encoding="utf-8", errors="replace")
    if "CHIP_ADDRESS_RESOLVE" in raw or "; -D" in raw:
        print("Removing poisoned platformio_toolchain.cmake")
        toolchain.unlink()
