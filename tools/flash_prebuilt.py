#!/usr/bin/env python3
"""Flash the bundled ESP32-S3 v0.2 images without installing ESP-IDF."""
import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="COM5, /dev/ttyACM0, etc.")
    parser.add_argument("--baud", type=int, default=460800)
    args = parser.parse_args()
    directory = Path(__file__).resolve().parents[1] / "prebuilt"
    images = [("0x0", "bootloader.bin"), ("0x8000", "partitions.bin"),
              ("0x10000", "firmware.bin")]
    command = [sys.executable, "-m", "esptool", "--chip", "esp32s3",
               "--port", args.port, "--baud", str(args.baud), "write_flash",
               "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB"]
    for offset, name in images:
        image = directory / name
        if not image.is_file():
            parser.error(f"Missing bundled image: {image}")
        command.extend([offset, str(image)])
    # esptool verifies the chip and written bytes. Do not erase all flash/NVS.
    return subprocess.call(command)


if __name__ == "__main__":
    raise SystemExit(main())
