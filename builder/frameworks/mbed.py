"""
mbed

The mbed framework The mbed SDK has been designed to provide enough
hardware abstraction to be intuitive and concise, yet powerful enough to
build complex projects. It is built on the low-level ARM CMSIS APIs,
allowing you to code down to the metal if needed. In addition to RTOS,
USB and Networking libraries, a cookbook of hundreds of reusable
peripheral and module libraries have been built on top of the SDK by
the mbed Developer Community.

http://mbed.org/
"""

from os.path import join

from SCons.Script import Import, SConscript

Import("env")

# https://github.com/platformio/builder-framework-mbed.git
SConscript(
    join(env.PioPlatform().get_package_dir("framework-N02"), "platformio","platformio-build.py"))
