import os, sys, subprocess, argparse
from pathlib import Path

SRC_FOLDER = "LunaCoreRuntime"

os.chdir('./'+SRC_FOLDER)

parser = argparse.ArgumentParser(
    prog="Helper builder",
    description="Builds LunaCoreRuntime"
)
parser.add_argument("-rb", "--rebuild", action="store_true", help="Rebuild project")
parser.add_argument("-c", "--clean", action="store_true", help="Clean project")
parser.add_argument("-r", "--release", action="store_true", help="Build in release mode")
parser.add_argument("-e", "--experimental", action="store_true", help="Build experimental features")
parser.add_argument("--legacyhooks", action="store_true", help="Build legacy hooks")
parser.add_argument("--enable-jit", action="store_true", help="Build with luajit")
args = parser.parse_args()

if args.rebuild:
    subprocess.run(["make", "clean"], check=True)
if args.clean:
    subprocess.run(["make", "clean"], check=True)
    sys.exit(0)

make_envvars = os.environ.copy()
cflags_extra = ""
if not args.release:
    print("Building in debug mode")
    cflags_extra += "-DDEBUG "
if args.experimental:
    cflags_extra += "-DEXPERIMENTAL "
if args.legacyhooks:
    cflags_extra += "-DLEGACY_HOOKS "
if args.enable_jit:
    make_envvars["BUILD_JIT"] = "1"
    make_envvars["TARGET_PREFIX"] = "-jit"
os.environ["CFLAGS_EXTRA"] = cflags_extra

cmd_args = ["make"]
try:
    subprocess.run(cmd_args, check=True, env=make_envvars)
except subprocess.CalledProcessError as e:
    print(e, file=sys.stderr)
    sys.exit(1)
except KeyboardInterrupt:
    print("Build cancelled by user", file=sys.stderr)
    sys.exit(1)