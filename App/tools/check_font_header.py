#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Check cn_font.bin against the generated v3 SPI Flash layout."""

import re
import struct
from pathlib import Path


def parse_define(header_text, name):
    match = re.search(r"^#define\s+" + re.escape(name) + r"\s+([0-9A-Fa-fxu]+)", header_text, re.MULTILINE)
    if not match:
        raise ValueError("missing define: " + name)
    token = match.group(1).rstrip("u")
    return int(token, 0)


script_dir = Path(__file__).resolve().parent
repo_root = script_dir.parent.parent
header_path = repo_root / "App" / "cn_font_data.h"
bin_path = repo_root / "docs" / "font" / "cn_font.bin"

header_text = header_path.read_text(encoding="utf-8")
bitmap_size = parse_define(header_text, "CN_FONT_BITMAP_SIZE")
char_count = parse_define(header_text, "CN_FONT_CHAR_COUNT")
version = parse_define(header_text, "CN_FONT_VERSION")
version_offset = parse_define(header_text, "CN_FONT_VERSION_OFFSET")

data = bin_path.read_bytes()
print("File size: %d bytes" % len(data))
print("CN_FONT_VERSION: %d" % version)
print("CN_FONT_VERSION_OFFSET: %d" % version_offset)

if len(data) != version_offset + 1:
    raise SystemExit("ERROR: bin length does not match CN_FONT_VERSION_OFFSET + 1")
if data[version_offset] != version:
    raise SystemExit("ERROR: version byte mismatch")

first_entry = struct.unpack_from("<I", data, bitmap_size)[0]
last_entry = struct.unpack_from("<I", data, bitmap_size + (char_count - 1) * 4)[0]
first_unicode = first_entry >> 16
last_unicode = last_entry >> 16
print("First Unicode: U+%04X" % first_unicode)
print("Last Unicode: U+%04X" % last_unicode)

if first_unicode != 0x4E00 or last_unicode != 0x9F99:
    raise SystemExit("ERROR: v3 sorted index boundary mismatch")

prev_unicode = -1
for entry_index in range(char_count):
    entry = struct.unpack_from("<I", data, bitmap_size + entry_index * 4)[0]
    unicode_val = entry >> 16
    if unicode_val < prev_unicode:
        raise SystemExit("ERROR: Unicode index is not sorted at entry %d" % entry_index)
    prev_unicode = unicode_val

print("OK: cn_font.bin matches v3 sorted-index layout")
