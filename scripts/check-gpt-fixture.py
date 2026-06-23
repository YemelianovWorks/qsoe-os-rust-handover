#!/usr/bin/env python3
"""Generate and validate a GPT fixture using host_tools/mkgpt.py."""

from __future__ import annotations

import pathlib
import struct
import subprocess
import sys
import uuid
import zlib


ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "build" / "fixtures" / "gpt"
IMG = FIXTURE / "nvme-fixture.img"
REPORT = FIXTURE / "mkgpt.log"

LBA_SIZE = 512
IMAGE_SIZE = 192 * 1024 * 1024
PART_MIBS = [16] * 8
FIRST_USABLE = 34
N_ENTRIES = 128
ENTRY_SIZE = 128
LINUX_GUID = uuid.UUID("0fc63daf-8483-4772-8e79-3d69d8477de4")
FSQRV_GUID = uuid.UUID("51525611-322e-4017-bae8-e4d9c9d4e979")


def die(message: str) -> None:
    print(f"check-gpt-fixture.py: {message}", file=sys.stderr)
    sys.exit(1)


def read_at(path: pathlib.Path, offset: int, size: int) -> bytes:
    with path.open("rb") as f:
        f.seek(offset)
        data = f.read(size)
    if len(data) != size:
        die(f"{path} truncated at offset {offset}")
    return data


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def utf16_name(raw: bytes) -> str:
    return raw.decode("utf-16-le").rstrip("\x00")


def expect(condition: bool, message: str) -> None:
    if not condition:
        die(message)


def main() -> None:
    FIXTURE.mkdir(parents=True, exist_ok=True)
    with IMG.open("wb") as f:
        f.truncate(IMAGE_SIZE)

    cmd = [
        sys.executable,
        str(ROOT / "host_tools" / "mkgpt.py"),
        "--fsqrv",
        "8",
        str(IMG),
        *[str(x) for x in PART_MIBS],
    ]
    result = subprocess.run(cmd, check=True, text=True, capture_output=True)
    REPORT.write_text(result.stdout, encoding="utf-8")

    pmbr = read_at(IMG, 0, LBA_SIZE)
    expect(pmbr[510:512] == b"\x55\xaa", "protective MBR signature missing")
    expect(pmbr[446 + 4] == 0xEE, "protective MBR partition type is not 0xEE")

    hdr = bytearray(read_at(IMG, LBA_SIZE, LBA_SIZE))
    expect(hdr[0:8] == b"EFI PART", "GPT signature missing")
    header_size = struct.unpack_from("<I", hdr, 12)[0]
    header_crc = struct.unpack_from("<I", hdr, 16)[0]
    expect(header_size == 92, f"unexpected header size {header_size}")

    hdr_for_crc = bytearray(hdr[:header_size])
    struct.pack_into("<I", hdr_for_crc, 16, 0)
    expect(crc32(hdr_for_crc) == header_crc, "header CRC mismatch")

    current_lba = struct.unpack_from("<Q", hdr, 24)[0]
    backup_lba = struct.unpack_from("<Q", hdr, 32)[0]
    first_usable = struct.unpack_from("<Q", hdr, 40)[0]
    entries_lba = struct.unpack_from("<Q", hdr, 72)[0]
    n_entries = struct.unpack_from("<I", hdr, 80)[0]
    entry_size = struct.unpack_from("<I", hdr, 84)[0]
    entries_crc = struct.unpack_from("<I", hdr, 88)[0]

    expect(current_lba == 1, f"unexpected current_lba {current_lba}")
    expect(backup_lba == IMAGE_SIZE // LBA_SIZE - 1, f"unexpected backup_lba {backup_lba}")
    expect(first_usable == FIRST_USABLE, f"unexpected first_usable {first_usable}")
    expect(entries_lba == 2, f"unexpected entries_lba {entries_lba}")
    expect(n_entries == N_ENTRIES, f"unexpected n_entries {n_entries}")
    expect(entry_size == ENTRY_SIZE, f"unexpected entry_size {entry_size}")

    entries = read_at(IMG, entries_lba * LBA_SIZE, n_entries * entry_size)
    expect(crc32(entries) == entries_crc, "partition entry CRC mismatch")

    for i, mib in enumerate(PART_MIBS):
        off = i * entry_size
        type_guid = uuid.UUID(bytes_le=entries[off : off + 16])
        first = struct.unpack_from("<Q", entries, off + 32)[0]
        last = struct.unpack_from("<Q", entries, off + 40)[0]
        name = utf16_name(entries[off + 56 : off + 128])

        expected_first = FIRST_USABLE + i * (mib * 1024 * 1024 // LBA_SIZE)
        expected_last = expected_first + (mib * 1024 * 1024 // LBA_SIZE) - 1
        expect(first == expected_first, f"p{i + 1} first LBA {first} != {expected_first}")
        expect(last == expected_last, f"p{i + 1} last LBA {last} != {expected_last}")

        if i == 7:
            expect(type_guid == FSQRV_GUID, f"p8 type GUID {type_guid} is not fsqrv")
            expect(name == "fsqrv", f"p8 name {name!r} is not fsqrv")
        else:
            expect(type_guid == LINUX_GUID, f"p{i + 1} type GUID {type_guid} is not linux-data")
            expect(name == f"qsoe-test-p{i + 1}", f"p{i + 1} name {name!r} mismatch")

    print("check-gpt-fixture.py: ok")
    print(f"  image: {IMG}")
    print(f"  log:   {REPORT}")


if __name__ == "__main__":
    main()
