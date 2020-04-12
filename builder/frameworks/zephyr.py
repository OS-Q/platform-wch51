"""
The Zephyr Project is a scalable real-time operating system (RTOS) supporting multiple
hardware architectures, optimized for resource constrained devices, and built with
safety and security in mind.

https://github.com/zephyrproject-rtos/zephyr
"""

from os.path import join

from SCons.Script import Import, SConscript

Import("env")

SConscript(
    join(env.PioPlatform().get_package_dir("framework-zephyr"), "scripts",
         "platformio", "platformio-build.py"), exports="env")
