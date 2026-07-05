Import("env")
import os
import struct
import subprocess

# ---------------------------------------------------------------------------
# Pre-build: inject git info as compiler defines
# ---------------------------------------------------------------------------

def _git(args, cwd):
    try:
        return subprocess.check_output(
            ["git"] + args, stderr=subprocess.DEVNULL, cwd=cwd
        ).decode().strip()
    except Exception:
        return ""


project_dir = env["PROJECT_DIR"]
branch      = _git(["rev-parse", "--abbrev-ref", "HEAD"], project_dir) or "unknown"
describe    = _git(["describe", "--tags", "--always", "--dirty"], project_dir) or "unknown"


def _flag(name, value):
    return (name, '\\"' + value + '\\"')


env.Append(CPPDEFINES=[
    _flag("BUILD_GIT_BRANCH", branch),
    _flag("BUILD_GIT_DESCRIBE", describe),
])
print(f"[build_info] branch={branch}  describe={describe}")


# ---------------------------------------------------------------------------
# Custom target: pio run -t version -e <env>
#
# Scans firmware.hex (Intel HEX) for the magic bytes "TRID2020" and decodes
# the embedded BuildInfoRecord. No external tools required.
#
# Struct layout (must match BuildInfoRecord in main.cpp):
#   char[8]   magic     "TRID2020"
#   uint16_t  major
#   uint16_t  minor
#   char[32]  branch
#   char[64]  describe
#   char[24]  built
# ---------------------------------------------------------------------------

_STRUCT_FMT   = "<8sHH32s64s24s"
_STRUCT_MAGIC = b"TRID2020"


def _parse_intel_hex(hex_path):
    """Return a bytearray containing the full flash image from an Intel HEX file."""
    segments = []  # list of (base_address, bytearray)
    base = 0

    with open(hex_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line.startswith(":"):
                continue
            raw      = bytes.fromhex(line[1:])
            count    = raw[0]
            addr     = (raw[1] << 8) | raw[2]
            rec_type = raw[3]

            if rec_type == 0:    # data
                segments.append((base + addr, bytearray(raw[4:4 + count])))
            elif rec_type == 2:  # extended segment address
                base = ((raw[4] << 8) | raw[5]) << 4
            elif rec_type == 4:  # extended linear address
                base = ((raw[4] << 8) | raw[5]) << 16
            elif rec_type == 1:  # EOF
                break

    if not segments:
        return bytearray()

    max_addr = max(addr + len(data) for addr, data in segments)
    mem = bytearray(b"\xff" * max_addr)
    for addr, data in segments:
        mem[addr:addr + len(data)] = data

    return mem


def _show_version(source, target, env):
    hex_path = os.path.join(env.subst("$BUILD_DIR"), "firmware.hex")

    if not os.path.exists(hex_path):
        print(f"[version] {hex_path} not found — run 'pio run -e <env>' first.")
        return

    mem = _parse_intel_hex(hex_path)
    pos = mem.find(_STRUCT_MAGIC)

    if pos == -1:
        print("[version] Magic 'TRID2020' not found in firmware.hex.")
        return

    expected = struct.calcsize(_STRUCT_FMT)
    if pos + expected > len(mem):
        print("[version] Struct extends past end of image — truncated?")
        return

    magic, major, minor, branch_b, describe_b, built_b = struct.unpack_from(_STRUCT_FMT, mem, pos)

    def _s(b):
        return b.rstrip(b"\x00").decode("ascii", errors="replace")

    print()
    print("=== Firmware Build Info ===")
    print(f"  Version:  {major}.{minor}")
    print(f"  Branch:   {_s(branch_b)}")
    print(f"  Describe: {_s(describe_b)}")
    print(f"  Built:    {_s(built_b)}")
    print(f"  HEX:      {hex_path}")
    print("===========================")
    print()


env.AddCustomTarget(
    "version",
    None,
    _show_version,
    title="Firmware version",
    description="Decode embedded build info from firmware.hex (build first)",
    always_build=True,
)
