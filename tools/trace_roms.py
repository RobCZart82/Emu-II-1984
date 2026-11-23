#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Run a bounded offline Z80 trace with a verified local main/scanner pair."""
import argparse
from pathlib import Path
import subprocess
import sys

from inspect_inputs import load_rom_pair


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', required=True, type=Path, help='built z80_rom_trace executable')
    parser.add_argument('--main-rom', required=True, type=Path)
    parser.add_argument('--scanner-rom', required=True, type=Path)
    parser.add_argument('--max-steps', type=int, default=256,
                        help='step limit per CPU (1–1000000, default: 256)')
    args = parser.parse_args()
    if not 1 <= args.max_steps <= 1000000:
        parser.error('--max-steps must be between 1 and 1000000')
    try:
        (main_rom, _), (scanner_rom, _) = load_rom_pair(args.main_rom, args.scanner_rom)
        return subprocess.run([str(args.runner.resolve()), '--max-steps', str(args.max_steps)], input=main_rom + scanner_rom,
                              timeout=10).returncode
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
