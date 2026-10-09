#!/usr/bin/env python3
"""Disable the extra GAP PPCP characteristic in an existing ESP-IDF sdkconfig."""
import argparse
from pathlib import Path
import re

KEYS = (
    "CONFIG_BT_NIMBLE_SVC_GAP_PPCP_MIN_CONN_INTERVAL",
    "CONFIG_BT_NIMBLE_SVC_GAP_PPCP_MAX_CONN_INTERVAL",
    "CONFIG_BT_NIMBLE_SVC_GAP_PPCP_SLAVE_LATENCY",
    "CONFIG_BT_NIMBLE_SVC_GAP_PPCP_SUPERVISION_TMO",
)


def update(path):
    raw = path.read_bytes()
    text = raw.decode("utf-8")
    if not re.search(r'^CONFIG_IDF_TARGET="esp32s3"\s*$', text, re.M):
        raise ValueError("Expected an ESP32-S3 sdkconfig; file left unchanged")
    for key in KEYS:
        pattern = rf"^{re.escape(key)}=[^\r\n]*"
        if len(re.findall(pattern, text, re.M)) != 1:
            raise ValueError(f"Expected one {key} entry; file left unchanged")
        text = re.sub(pattern, f"{key}=0", text, flags=re.M)
    changed = text.encode("utf-8")
    if changed == raw:
        print(f"Already configured: {path}")
        return
    backup = path.with_name(path.name + ".before-r1-v022")
    # Never overwrite an existing backup. Existing unrelated settings survive.
    if not backup.exists():
        backup.write_bytes(raw)
    path.write_bytes(changed)
    print(f"Updated four PPCP values: {path}; backup: {backup}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sdkconfig", type=Path)
    args = parser.parse_args()
    try:
        update(args.sdkconfig)
    except (OSError, UnicodeError, ValueError) as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    main()
