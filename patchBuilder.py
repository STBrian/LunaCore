#!/usr/bin/env python3
"""
patchBuilder.py - Compiles .patch files containing ARM/THUMB assembly into standard IPS patch files.
Also supports merging multiple IPS files into a single unified IPS file.
"""

import sys
import os
import re
import argparse
import subprocess
import tempfile
import shutil

# Try importing keystone as fallback assembler
try:
    import keystone
    HAS_KEYSTONE = True
except ImportError:
    HAS_KEYSTONE = False


def find_devkit_binaries():
    """Find arm-none-eabi-as and arm-none-eabi-objcopy from devkitARM."""
    devkitarm = os.environ.get("DEVKITARM", "/opt/devkitpro/devkitARM")
    as_bin = os.path.join(devkitarm, "bin", "arm-none-eabi-as")
    objcopy_bin = os.path.join(devkitarm, "bin", "arm-none-eabi-objcopy")

    if os.path.isfile(as_bin) and os.path.isfile(objcopy_bin):
        return as_bin, objcopy_bin

    # Try PATH
    as_which = shutil.which("arm-none-eabi-as")
    objcopy_which = shutil.which("arm-none-eabi-objcopy")
    if as_which and objcopy_which:
        return as_which, objcopy_which

    return None, None


def assemble_instructions(mode: str, instructions: list[str]) -> bytes:
    """
    Assembles a list of instruction strings into machine code bytes for ARM or THUMB mode.
    Prefers devkitARM toolchain; falls back to keystone module if devkitARM is unavailable.
    """
    mode = mode.upper()
    if mode not in ("ARM", "THUMB"):
        raise ValueError(f"Unsupported architecture mode: '{mode}'. Must be ARM or THUMB.")

    clean_code = "\n".join(instructions).strip()
    if not clean_code:
        return b""

    as_bin, objcopy_bin = find_devkit_binaries()
    if as_bin and objcopy_bin:
        with tempfile.TemporaryDirectory() as tmpdir:
            s_path = os.path.join(tmpdir, "patch.s")
            o_path = os.path.join(tmpdir, "patch.o")
            bin_path = os.path.join(tmpdir, "patch.bin")

            directive = ".arm" if mode == "ARM" else ".thumb"
            asm_content = f".syntax unified\n{directive}\n{clean_code}\n"

            with open(s_path, "w", encoding="utf-8") as f:
                f.write(asm_content)

            as_cmd = [as_bin, "-mcpu=mpcore", s_path, "-o", o_path]
            res = subprocess.run(as_cmd, capture_output=True, text=True)
            if res.returncode != 0:
                raise RuntimeError(f"Assembler error ({as_bin}):\n{res.stderr.strip()}")

            objcopy_cmd = [objcopy_bin, "-O", "binary", o_path, bin_path]
            res = subprocess.run(objcopy_cmd, capture_output=True, text=True)
            if res.returncode != 0:
                raise RuntimeError(f"Objcopy error ({objcopy_bin}):\n{res.stderr.strip()}")

            with open(bin_path, "rb") as f:
                return f.read()

    # Fallback to Keystone if devkitARM is not found
    if HAS_KEYSTONE:
        ks_mode = keystone.KS_MODE_ARM if mode == "ARM" else keystone.KS_MODE_THUMB
        ks = keystone.Ks(keystone.KS_ARCH_ARM, ks_mode)
        try:
            encoding, _ = ks.asm(clean_code)
            return bytes(encoding)
        except keystone.KsError as e:
            raise RuntimeError(f"Keystone assembler error: {e}")

    raise RuntimeError(
        "Could not find devkitARM tools (arm-none-eabi-as/objcopy) and python module 'keystone' is not available."
    )


def parse_patch_file(patch_path: str, default_base_offset: int = 0x100000) -> list[tuple[int, bytes]]:
    """
    Parses a .patch file and compiles each INS_PATCH block.
    Returns a list of (file_offset, compiled_bytes).
    """
    if not os.path.isfile(patch_path):
        raise FileNotFoundError(f"Patch file not found: '{patch_path}'")

    with open(patch_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    records: list[tuple[int, bytes]] = []
    in_block = False
    current_mode = None
    target_offset = None
    block_instructions: list[str] = []
    block_start_line = 0

    for line_num, raw_line in enumerate(lines, 1):
        line = raw_line.strip()

        # Ignore empty lines and full-line comments starting with '//'
        if not line or line.startswith("//"):
            continue

        if not in_block:
            # Check for start of block: INS_PATCH [ARM|THUMB]
            match = re.match(r"^INS_PATCH(?:\s+([A-Za-z0-9_]+))?$", line, re.IGNORECASE)
            if match:
                mode_str = match.group(1)
                if not mode_str:
                    raise ValueError(f"Line {line_num}: INS_PATCH requires mode specification (ARM or THUMB).")
                current_mode = mode_str.upper()
                if current_mode not in ("ARM", "THUMB"):
                    raise ValueError(f"Line {line_num}: Invalid mode '{current_mode}'. Must be ARM or THUMB.")

                in_block = True
                target_offset = None
                block_instructions = []
                block_start_line = line_num
                continue
            else:
                raise ValueError(f"Line {line_num}: Unexpected content outside INS_PATCH block: '{line}'")
        else:
            # Check for end of block: END_INS_PATCH
            if line.upper() == "END_INS_PATCH":
                if target_offset is None:
                    raise ValueError(f"Block starting at line {block_start_line} ended without 'AT' address directive.")
                if not block_instructions:
                    raise ValueError(f"Block starting at line {block_start_line} has no instructions.")

                compiled = assemble_instructions(current_mode, block_instructions)
                if not compiled:
                    raise ValueError(f"Block starting at line {block_start_line} produced 0 bytes.")

                records.append((target_offset, compiled))
                in_block = False
                current_mode = None
                target_offset = None
                block_instructions = []
                continue

            # Check for AT directive: AT [PC] <address>
            at_match = re.match(r"^AT(?:\s+(PC))?\s+(0x[0-9a-fA-F]+|\d+)$", line, re.IGNORECASE)
            if at_match:
                if target_offset is not None:
                    raise ValueError(f"Line {line_num}: Duplicate AT directive in block starting at line {block_start_line}.")

                is_pc = (at_match.group(1) is not None)
                addr_str = at_match.group(2)
                addr_val = int(addr_str, 16) if addr_str.lower().startswith("0x") else int(addr_str, 10)

                if is_pc:
                    if addr_val < default_base_offset:
                        raise ValueError(
                            f"Line {line_num}: Address 0x{addr_val:X} is less than base offset 0x{default_base_offset:X}. "
                            f"Cannot compute non-negative file offset when 'PC' is specified."
                        )
                    file_offset = addr_val - default_base_offset
                else:
                    file_offset = addr_val

                if file_offset < 0:
                    raise ValueError(f"Line {line_num}: Computed negative file offset: {file_offset}")
                if file_offset > 0xFFFFFF:
                    raise ValueError(f"Line {line_num}: Offset 0x{file_offset:X} exceeds 24-bit IPS offset limit (0xFFFFFF).")

                target_offset = file_offset
                continue

            # Instruction line: strip inline comments if present
            comment_idx = line.find("//")
            if comment_idx != -1:
                line = line[:comment_idx].strip()
            if line:
                block_instructions.append(line)

    if in_block:
        raise ValueError(f"Unclosed INS_PATCH block starting at line {block_start_line} (missing END_INS_PATCH).")

    return records


def write_ips(records: list[tuple[int, bytes]], out_path: str):
    """
    Writes a standard IPS patch file.
    records is a list of (offset, data).
    """
    out_dir = os.path.dirname(os.path.abspath(out_path))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(out_path, "wb") as f:
        # Header: "PATCH"
        f.write(b"PATCH")

        for offset, data in records:
            chunk_offset = offset
            # IPS record max size is 0xFFFF (65535 bytes)
            for i in range(0, len(data), 0xFFFF):
                chunk = data[i : i + 0xFFFF]
                if chunk_offset == 0x454F46:  # ASCII "EOF"
                    raise ValueError("Patch offset 0x454F46 conflicts with IPS EOF marker.")
                f.write(chunk_offset.to_bytes(3, byteorder="big"))
                f.write(len(chunk).to_bytes(2, byteorder="big"))
                f.write(chunk)
                chunk_offset += len(chunk)

        # Footer: "EOF"
        f.write(b"EOF")


def read_ips(ips_path: str) -> list[tuple[int, bytes]]:
    """
    Reads a standard IPS patch file and returns a list of (offset, data) records.
    """
    if not os.path.isfile(ips_path):
        raise FileNotFoundError(f"IPS file not found: '{ips_path}'")

    with open(ips_path, "rb") as f:
        content = f.read()

    if len(content) < 8 or content[:5] != b"PATCH":
        raise ValueError(f"Invalid IPS file (missing 'PATCH' header): '{ips_path}'")

    records: list[tuple[int, bytes]] = []
    idx = 5
    content_len = len(content)

    while idx < content_len:
        if content[idx : idx + 3] == b"EOF":
            idx += 3
            break

        if idx + 5 > content_len:
            raise ValueError(f"Truncated IPS record in '{ips_path}' at byte position {idx}.")

        offset = int.from_bytes(content[idx : idx + 3], byteorder="big")
        size = int.from_bytes(content[idx + 3 : idx + 5], byteorder="big")
        idx += 5

        if size > 0:
            if idx + size > content_len:
                raise ValueError(f"Truncated standard record in '{ips_path}' at offset 0x{offset:X}.")
            data = content[idx : idx + size]
            idx += size
            records.append((offset, data))
        else:
            # RLE record: 2-byte count + 1-byte value
            if idx + 3 > content_len:
                raise ValueError(f"Truncated RLE record in '{ips_path}' at offset 0x{offset:X}.")
            rle_count = int.from_bytes(content[idx : idx + 2], byteorder="big")
            rle_val = content[idx + 2 : idx + 3]
            idx += 3
            records.append((offset, rle_val * rle_count))

    return records


def merge_ips_files(ips_paths: list[str], out_path: str):
    """
    Merges multiple IPS files into a single unified IPS file sorted by offset.
    """
    all_records: list[tuple[int, bytes]] = []
    for path in ips_paths:
        if not os.path.exists(path):
            raise FileNotFoundError(f"IPS file not found: '{path}'")
        recs = read_ips(path)
        all_records.extend(recs)

    # Sort records by offset
    all_records.sort(key=lambda r: r[0])

    # Check for overlapping patches
    for i in range(len(all_records) - 1):
        off1, data1 = all_records[i]
        off2, data2 = all_records[i + 1]
        end1 = off1 + len(data1)
        if end1 > off2:
            print(
                f"Warning: Overlapping patches detected between [0x{off1:X}..0x{end1:X}] "
                f"and [0x{off2:X}..0x{off2+len(data2):X}].",
                file=sys.stderr
            )

    write_ips(all_records, out_path)
    print(f"Successfully merged {len(ips_paths)} IPS file(s) ({len(all_records)} record(s)) into '{out_path}'.")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compile .patch files with ARM/THUMB assembly into .ips patches and merge them."
    )
    parser.add_argument(
        "patch_file",
        nargs="?",
        help="Input .patch file to compile."
    )
    parser.add_argument(
        "-o", "--output",
        help="Output .ips file path. If omitted, defaults to './out/<basename>.ips'."
    )
    parser.add_argument(
        "--base-offset",
        type=lambda x: int(x, 16) if x.lower().startswith("0x") else int(x, 10),
        default=0x100000,
        help="Base runtime address to subtract when 'PC' is specified (default: 0x100000)."
    )
    parser.add_argument(
        "--merge",
        nargs="+",
        metavar="IPS_FILE",
        help="Merge multiple .ips files into a single output .ips file."
    )
    parser.add_argument(
        "--merge-dir",
        metavar="DIR",
        help="Merge all .ips files found in DIR into a single output .ips file."
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Handle merge modes
    if args.merge or args.merge_dir:
        ips_files = []
        if args.merge:
            ips_files.extend(args.merge)
        if args.merge_dir:
            if not os.path.isdir(args.merge_dir):
                print(f"Error: Directory not found: '{args.merge_dir}'", file=sys.stderr)
                sys.exit(1)
            for fname in sorted(os.listdir(args.merge_dir)):
                if fname.lower().endswith(".ips"):
                    ips_files.append(os.path.join(args.merge_dir, fname))

        if not ips_files:
            print("Error: No IPS files specified or found for merging.", file=sys.stderr)
            sys.exit(1)

        out_path = args.output if args.output else "./dist/patches.ips"
        try:
            merge_ips_files(ips_files, out_path)
        except Exception as e:
            print(f"Error merging IPS files: {e}", file=sys.stderr)
            sys.exit(1)
        return

    # Handle patch compilation mode
    if not args.patch_file:
        print("Error: No input .patch file specified. Run with -h for help.", file=sys.stderr)
        sys.exit(1)

    # Determine output path
    if args.output:
        out_path = args.output
    else:
        base_name = os.path.splitext(os.path.basename(args.patch_file))[0]
        out_path = os.path.join("./out", f"{base_name}.ips")

    try:
        records = parse_patch_file(args.patch_file, default_base_offset=args.base_offset)
        if not records:
            print(f"Warning: No patch records found in '{args.patch_file}'.", file=sys.stderr)
        write_ips(records, out_path)
        total_bytes = sum(len(d) for _, d in records)
        print(f"Compiled '{args.patch_file}' -> '{out_path}' ({len(records)} record(s), {total_bytes} byte(s)).")
    except Exception as e:
        print(f"Error compiling patch '{args.patch_file}': {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
