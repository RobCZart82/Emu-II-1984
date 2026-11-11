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
assert [list(row.values()) for row in rows] == [row + [''] for row in expected], rows
for payload in (pair[:-1], pair + b'\0'):
    invalid = subprocess.run(command, input=payload, capture_output=True)
    assert invalid.returncode == 1 and not invalid.stdout

# A NOP-only ROM reaches the cap. HALT ends the other trace immediately.
bounded = subprocess.run(command, input=bytes(8192) + b'\x76' + bytes(8191),
                         capture_output=True, check=True)
rows = list(csv.DictReader(io.StringIO(bounded.stdout.decode('ascii'))))
assert len(rows) == 257 and rows[-2]['event'] == 'step_limit' and rows[-2]['step'] == '256' and rows[-1]['event'] == 'halt'

# Stop on opcode fetch, data read, and data write outside the known backing.
for code, direction, physical in [
        (bytes([0xc3, 0, 0x28]), 'read', 0x2800),
        (bytes([0x3a, 0, 0x28]), 'read', 0x2800),
        (bytes([0x32, 0, 0x28]), 'write', 0x2800),
        (bytes([0x3e, 1, 0xd3, 0x6a]), 'read', 0x8004)]:
    payload = code.ljust(8192, b'\0') + b'\x76' + bytes(8191)
    result = subprocess.run(command, input=payload, capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    main_rows = [row for row in trace if row['cpu'] == 'main']
    last = main_rows[-1]
    assert last['event'] == 'unmodelled_memory_' + direction, trace
    assert int(last['memory_address']) == physical, trace
    assert last['pc_after'] == '' and last['port'] == '', trace
    assert trace[-1]['cpu'] == 'scanner' and trace[-1]['event'] == 'halt', trace

# Each DD/FD prefix consumes an upstream step before the indexed instruction.
for prefix in (0xdd, 0xfd):
    code = bytes([prefix, 0x21, 0x34, 0x12, 0x76])
    payload = code.ljust(8192, b'\0') + b'\x76' + bytes(8191)
    result = subprocess.run(command, input=payload, capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    main_rows = [row for row in trace if row['cpu'] == 'main']
    assert [(row['step'], row['pc_before'], row['pc_after'], row['t_states'], row['event'])
            for row in main_rows] == [
                ('1', '0', '1', '4', 'step'),
                ('2', '1', '4', '14', 'step'),
                ('3', '4', '5', '18', 'halt')], trace
