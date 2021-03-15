
import sys
from os.path import basename, isdir, isfile, join

from SCons.Script import DefaultEnvironment

from platformio.proc import exec_command

env = DefaultEnvironment()
platform = env.PioPlatform()
board_config = env.BoardConfig()

FRAMEWORK_DIR = platform.get_package_dir("E13A")
assert isdir(FRAMEWORK_DIR)


def get_core_files():
    command = [
        env.subst("$CC"), "-m%s" % board_config.get("build.cpu"),
        "-D%s" % board_config.get("build.mcu")[0:5].upper(),
        "-I.", "-I", "%s" % env.subst("$PROJECTSRC_DIR"),
        "-Wp-MM", "-E", "ch554.h"
    ]

    result = exec_command(
        command,
        cwd=join(FRAMEWORK_DIR,"include"),
        env=env['ENV']
    )

    if result['returncode'] != 0:
        sys.stderr.write(
            "Error: Could not parse library files for the target.\n")
        sys.stderr.write(result['err'])
        env.Exit(1)

    src_files = []
    includes = result['out']
    for inc in includes.split(" "):
        if "_" not in inc or ".h" not in inc or "conf" in inc:
            continue
        src_files.append(basename(inc).replace(".h", ".c").strip())

    return src_files


env.Append(
    CFLAGS=[
        "--model-small",
        "-DFREQ_SYS=$(F_CPU)",
        "-mmcs51",
        "-V"
    ],

    CPPPATH=[
        join(FRAMEWORK_DIR,"include"),
        "$PROJECTSRC_DIR",
    ]
)

env.BuildSources(
    join("$BUILD_DIR", "native"),
    join(FRAMEWORK_DIR,"include"),
    src_filter=["-<*>"] + [" +<%s>" % f for f in get_core_files()]
)
