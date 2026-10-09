#!/usr/bin/env python3
"""USB serial trace collector; does not require an iPhone app or a BLE sniffer."""
import argparse
import datetime as dt
import sys
import threading
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="COM5, /dev/ttyACM0, etc.")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        import serial
    except ImportError:
        parser.error("Install pyserial: python -m pip install pyserial")
    # Refuse accidental overwriting of an earlier experiment's trace.
    with args.output.open("x", encoding="utf-8", buffering=1) as output:
        with serial.Serial(args.port, args.baud, timeout=0.2) as port:
            write_lock = threading.Lock()
            stopped = threading.Event()

            def record(direction, text):
                timestamp = dt.datetime.now().astimezone().isoformat(timespec="milliseconds")
                line = f"{timestamp} {direction} {text.rstrip()}\n"
                with write_lock:
                    output.write(line)
                    sys.stdout.write(line)
                    sys.stdout.flush()

            def commands():
                try:
                    for line in sys.stdin:
                        if stopped.is_set():
                            break
                        text = line.rstrip("\r\n")
                        if not text:
                            continue
                        record("HOST>", text)
                        port.write((text + "\n").encode("ascii", errors="strict"))
                except (OSError, UnicodeError, serial.SerialException) as error:
                    if not stopped.is_set():
                        record("HOST!", str(error))

            threading.Thread(target=commands, daemon=True).start()
            record("HOST!", "capture started; restart board to include boot, Ctrl+C to stop")
            pending = bytearray()
            try:
                while True:
                    pending.extend(port.read(port.in_waiting or 1))
                    while b"\n" in pending:
                        line, _, rest = pending.partition(b"\n")
                        pending = bytearray(rest)
                        record("ESP>", line.decode("utf-8", errors="replace"))
                    if len(pending) > 8192:
                        record("ESP>", pending.decode("utf-8", errors="replace"))
                        pending.clear()
            except KeyboardInterrupt:
                stopped.set()
                if pending:
                    record("ESP>", pending.decode("utf-8", errors="replace"))
                record("HOST!", "capture stopped")
            finally:
                stopped.set()


if __name__ == "__main__":
    main()
