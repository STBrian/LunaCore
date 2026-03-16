import os, sys, subprocess, argparse

os.chdir('./LunaCoreRuntime')

parser = argparse.ArgumentParser(
    prog="Helper builder",
    description="Builds LunaCoreRuntime"
)
parser.add_argument("-rb", "--rebuild", action="store_true", help="Rebuild project")
parser.add_argument("-c", "--clean", action="store_true", help="Clean project")
parser.add_argument("-r", "--release", action="store_true", help="Build in release mode")
parser.add_argument("-e", "--experimental", action="store_true", help="Build experimental features")
parser.add_argument("--legacyhooks", action="store_true", help="Build legacy hooks")
args = parser.parse_args()

if args.rebuild:
    subprocess.run(["make", "clean"], check=True)
if args.clean:
    subprocess.run(["make", "clean"], check=True)
    sys.exit(0)

cflags_extra = ""
if not args.release:
    print("Building in debug mode")
    cflags_extra += "-DDEBUG "
if args.experimental:
    cflags_extra += "-DEXPERIMENTAL "
if args.legacyhooks:
    cflags_extra += "-DLEGACY_HOOKS "
os.environ["CFLAGS_EXTRA"] = cflags_extra

cmd_args = ["make"]
try:
    subprocess.run(cmd_args, check=True)
except subprocess.CalledProcessError as e:
    print(e, file=sys.stderr)
    sys.exit(1)
except KeyboardInterrupt:
    print("Build cancelled by user", file=sys.stderr)
    sys.exit(1)