
import sys
from os.path import basename, isdir, isfile, join

from SCons.Script import DefaultEnvironment

from platformio.proc import exec_command

env = DefaultEnvironment()
platform = env.PioPlatform()
board_config = env.BoardConfig()

FRAMEWORK_DIR = platform.get_package_dir("framework-wch51")
assert isdir(FRAMEWORK_DIR)


env.Append(
    CFLAGS=[
        "--model-small"
    ],

    CPPPATH=[
        join(FRAMEWORK_DIR,"inc"),
        join(FRAMEWORK_DIR,"include"),
        "$PROJECTSRC_DIR",
    ]
)

env.BuildSources(
    join("$BUILD_DIR", "native"),
    join(FRAMEWORK_DIR,"src")
)
