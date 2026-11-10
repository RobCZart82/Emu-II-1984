# SPDX-License-Identifier: AGPL-3.0-only
import csv
import io
import subprocess
import sys

runner = sys.argv[1]

# LD A,0, OUT to the main CPU's low page selector, then unmodelled IN.
main_rom = bytes([0x3e, 0, 0xd3, 0x6a, 0xdb, 0x42]).ljust(8192, b'\0')
# The same port is unmodelled on the scanner. HALT must never be reached.
scanner_rom = bytes([0xd3, 0x6a, 0x76]).ljust(8192, b'\0')
pair = main_rom + scanner_rom
command = [runner]
run = subprocess.run(command, input=pair, capture_output=True, check=True)
rows = list(csv.DictReader(io.StringIO(run.stdout.decode('ascii'))))
expected = [
    ['main', '1', '0', '2', '7', 'step', ''],
    ['main', '2', '2', '4', '18', 'step', ''],
    ['main', '3', '4', '', '28', 'unmodelled_in', '66'],
    ['scanner', '1', '0', '', '10', 'unmodelled_out', '65386'],
]
assert [list(row.values()) for row in rows] == expected, rows
for payload in (pair[:-1], pair + b'\0'):
    invalid = subprocess.run(command, input=payload, capture_output=True)
    assert invalid.returncode == 1 and not invalid.stdout

# A NOP-only ROM reaches the cap. HALT ends the other trace immediately.
bounded = subprocess.run(command, input=bytes(8192) + b'\x76' + bytes(8191),
                         capture_output=True, check=True)
rows = list(csv.DictReader(io.StringIO(bounded.stdout.decode('ascii'))))
assert len(rows) == 257 and rows[-2]['event'] == 'step_limit' and rows[-2]['step'] == '256' and rows[-1]['event'] == 'halt'
