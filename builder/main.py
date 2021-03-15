import sys
from os.path import join
from platform import system

from SCons.Script import (AlwaysBuild, COMMAND_LINE_TARGETS, Default,
                            DefaultEnvironment)

def __getSize(size_type, env):
    return str(env.BoardConfig().get("build", {
        # defaults
        "size_heap": 1024,
        "size_iram": 256,
        "size_xram": 65536,
        "size_code": 65536,
    })[size_type])

env = DefaultEnvironment()
board_config = env.BoardConfig()

env.Replace(
    AR="sdar",
    AS="sdas8051",
    CC="sdcc",
    LD="sdld",
    RANLIB="sdranlib",
    OBJCOPY="sdobjcopy",
    WCHISP="wchisptool -g -f",
    OBJSUFFIX=".rel",
    LIBSUFFIX=".lib",
    SIZETOOL=join(env.PioPlatform().get_dir(), "builder", "size.py"),
    CFLAGS=[
        "-m%s" % board_config.get("build.cpu")
    ],
    CPPDEFINES=[
        "F_CPU=$BOARD_F_CPU",
        "HEAP_SIZE=" + __getSize("size_heap", env)
    ],
    LINKFLAGS=[
        "-m%s" % board_config.get("build.cpu"),
        "--nostdlib",
        "--code-size", board_config.get("build.size_code"),
        "--iram-size", board_config.get("build.size_iram"),
        "--xram-size", board_config.get("build.size_xram"),
        "--out-fmt-ihx"
    ],

    LIBPATH=[
        join(env.PioPlatform().get_package_dir("toolchain-sdcc"),
            "%s" % "lib" if system() == "Windows" else join("share", "sdcc", "lib"),
            board_config.get("build.cpu"))
    ],
    SIZEPROGREGEXP=r"^(?:HOME|GSINIT|GSFINAL|CODE|INITIALIZER)\s+([0-9]+).*",
    SIZEDATAREGEXP=r"^(?:DATA|INITIALIZED)\s+([0-9]+).*",
    SIZEEEPROMREGEXP=r"^(?:EEPROM)\s+([0-9]+).*",
    SIZECHECKCMD="$SIZETOOL -A $SOURCES",
    SIZEPRINTCMD='$SIZETOOL -d $SOURCES',

    PROGNAME="firmware",
    PROGSUFFIX=".ihx"
)

# Allow user to override via pre:script
if env.get("PROGNAME", "program") == "program":
    env.Replace(PROGNAME="firmware")

def _ldflags_for_ihx(env, ldflags):
    ldflags = ["--out-fmt-ihx" if f == "--out-fmt-elf" else f for f in ldflags]
    return ldflags

env.Append(
    ASFLAGS=env.get("CFLAGS", [])[:]
)

#
# Target: Build executable and linkable firmware
#

target_elf = None
if "nobuild" in COMMAND_LINE_TARGETS:
    target_elf = join("$BUILD_DIR", "${PROGNAME}.elf")
    target_firm = join("$BUILD_DIR", "${PROGNAME}.ihx")
else:
    target_elf = env.BuildProgram()
    target_firm = env.Command(
        join("$BUILD_DIR", "${PROGNAME}.ihx"),
        env['PIOBUILDFILES'],
        env['LINKCOM'].replace("$LINKFLAGS", "$ldflags_for_ihx")
    )
    env.Depends(target_firm, target_elf)

AlwaysBuild(env.Alias("nobuild", target_firm))
target_buildprog = env.Alias("buildprog", target_firm, target_firm)

#
# Target: Print binary size
#
target_size = env.Alias(
    "size", target_firm,
    env.VerboseAction("$SIZEPRINTCMD", "Calculating size $SOURCE"))
AlwaysBuild(target_size)


#
# Setup default targets
#

Default([target_buildprog, target_size])
