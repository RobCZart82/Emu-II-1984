# SPDX-License-Identifier: AGPL-3.0-only
import csv
import io
import subprocess
import sys

def check(condition, details=None):
    if not condition:
        raise AssertionError(details if details is not None else 'ROM trace expectation failed')


runner = sys.argv[1]

# LD A,0, OUT to the main CPU's low page selector, then unmodelled IN.
main_rom = bytes([0x3e, 0, 0xd3, 0x6a, 0xdb, 0x42]).ljust(8192, b'\0')
# The same low port mirrors scanner CTC channel 2. FF holds reset.
scanner_rom = bytes([0xd3, 0x6a, 0x76]).ljust(8192, b'\0')
pair = main_rom + scanner_rom
command = [runner]
run = subprocess.run(command, input=pair, capture_output=True, check=True)
rows = list(csv.DictReader(io.StringIO(run.stdout.decode('ascii'))))
expected = [
    ['main', '1', '0', '2', '7', 'step', ''],
    ['main', '2', '2', '4', '18', 'step', ''],
    ['main', '3', '4', '', '28', 'unmodelled_in', '66'],
    ['scanner', '1', '0', '2', '11', 'step', ''],
    ['scanner', '2', '2', '3', '15', 'halt', ''],
]
check([list(row.values()) for row in rows] == [row + [''] for row in expected], rows)
for payload in (pair[:-1], pair + b'\0'):
    invalid = subprocess.run(command, input=payload, capture_output=True)
    check(invalid.returncode == 1 and not invalid.stdout)

# A NOP-only ROM reaches the cap. HALT ends the other trace immediately.
bounded = subprocess.run(command, input=bytes(8192) + b'\x76' + bytes(8191),
                         capture_output=True, check=True)
rows = list(csv.DictReader(io.StringIO(bounded.stdout.decode('ascii'))))
check(len(rows) == 257 and rows[-2]['event'] == 'step_limit' and rows[-2]['step'] == '256' and rows[-1]['event'] == 'halt')

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
    check(last['event'] == 'unmodelled_memory_' + direction, trace)
    check(int(last['memory_address']) == physical, trace)
    check(last['pc_after'] == '' and last['port'] == '', trace)
    check(trace[-1]['cpu'] == 'scanner' and trace[-1]['event'] == 'halt', trace)

# Each DD/FD prefix consumes an upstream step before the indexed instruction.
for prefix in (0xdd, 0xfd):
    code = bytes([prefix, 0x21, 0x34, 0x12, 0x76])
    payload = code.ljust(8192, b'\0') + b'\x76' + bytes(8191)
    result = subprocess.run(command, input=payload, capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    main_rows = [row for row in trace if row['cpu'] == 'main']
    check([(row['step'], row['pc_before'], row['pc_after'], row['t_states'], row['event'])
            for row in main_rows] == [
                ('1', '0', '1', '4', 'step'),
                ('2', '1', '4', '14', 'step'),
                ('3', '4', '5', '18', 'halt')], trace)

# Every low-byte mirror of scanner channel zero accepts vector programming.
for port in (0x60, 0x64, 0x68, 0x6c, 0xe0, 0xe4, 0xe8, 0xec):
    scanner = bytes([0x3e, 0xf0, 0xd3, port, 0x76]).ljust(8192, b'\0')
    result = subprocess.run(command, input=b'\x76' + bytes(8191) + scanner,
                            capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    check(trace[-1]['event'] == 'halt' and trace[-1]['step'] == '3', trace)

# Stop at time-constant loading before running the unimplemented timer.
scanner = bytes([0x3e, 0xf0, 0xd3, 0xec, 0x3e, 7, 0xd3, 0xec,
                 0x3e, 1, 0xd3, 0xec, 0x76]).ljust(8192, b'\0')
result = subprocess.run(command, input=b'\x76' + bytes(8191) + scanner,
                        capture_output=True, check=True)
trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
check(trace[-1]['event'] == 'unmodelled_ctc_timing', trace)
check(trace[-1]['step'] == '6' and trace[-1]['pc_after'] == '', trace)
check(int(trace[-1]['port']) == 0x01ec, trace)

# CTC reads and PIO data outputs retain their unsupported boundaries.
for code, event in [(bytes([0xdb, 0xec]), 'unmodelled_in'),
                    (bytes([0xd3, 0x50]), 'unmodelled_out')]:
    result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                            capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    check(trace[-1]['event'] == event and trace[-1]['step'] == '1', trace)

# Every PIO control mirror uses alternate A/B wiring and consumes masks first.
for mirror in (0, 4, 8, 12, 0x80, 0x84, 0x88, 0x8c):
    a, b = 0x51 | mirror, 0x53 | mirror
    code = bytes([0x3e, 0xf8, 0xd3, a, 0x3e, 0xfa, 0xd3, b,
                  0x3e, 0x17, 0xd3, b, 0x3e, 0xcf, 0xd3, b, 0x76])
    result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                            capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    check(trace[-1]['event'] == 'halt' and trace[-1]['step'] == '9', trace)

# Mode changes and interrupt enable need external pin/timing behavior.
for value, event in [(0x0f, 'unmodelled_pio_mode'),
                     (0x4f, 'unmodelled_pio_mode'),
                     (0x8f, 'unmodelled_pio_mode'),
                     (0xcf, 'unmodelled_pio_mode'),
                     (0x83, 'unmodelled_pio_interrupts'),
                     (0x97, 'unmodelled_pio_interrupts'),
                     (0x01, 'unmodelled_out')]:
    code = bytes([0x3e, value, 0xd3, 0xdd, 0x76])
    result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                            capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    check(trace[-1]['event'] == event and trace[-1]['step'] == '2', trace)
    check(int(trace[-1]['port']) == (value << 8) | 0xdd, trace)
    check(trace[-1]['pc_after'] == '', trace)

# PIO reads, both data registers and main PIO are still outside this model.
for code, cpu, event in [(bytes([0xdb, 0xdd]), 'scanner', 'unmodelled_in'),
                         (bytes([0xd3, 0x50]), 'scanner', 'unmodelled_out'),
                         (bytes([0xd3, 0x52]), 'scanner', 'unmodelled_out'),
                         (bytes([0xd3, 0xbe]), 'scanner', 'unmodelled_lcd_command'),
                         (bytes([0xd3, 0x31]), 'main', 'unmodelled_out')]:
    rom = code.ljust(8192, b'\0')
    halt = b'\x76' + bytes(8191)
    result = subprocess.run(command, input=(rom + halt if cpu == 'main' else halt + rom),
                            capture_output=True, check=True)
    trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
    last = [row for row in trace if row['cpu'] == cpu][-1]
    check(last['event'] == event and last['step'] == '1', trace)

# IC47 ignores SA0/SA7; the LCD independently uses SA0 as register select.
for output in range(8):
    for mirror in (0, 1, 0x80, 0x81):
        port = 0x30 | (output << 1) | mirror
        for opcode in (0xd3, 0xdb):
            code = bytes([0x3e, 0x3b, opcode, port, 0x76])
            result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                                    capture_output=True, check=True)
            trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
            if output == 7 and opcode == 0xd3 and not (port & 1):
                check(trace[-1]['event'] == 'halt' and trace[-1]['step'] == '3', trace)
            else:
                event = 'unmodelled_scanner_strobe'
                if output == 7:
                    event = 'unmodelled_lcd_bus_read' if opcode == 0xdb else 'unmodelled_lcd_data'
                check(trace[-1]['event'] == event and trace[-1]['step'] == '2', trace)
                check(int(trace[-1]['port']) == 0x3b00 | port, trace)
                check(trace[-1]['pc_after'] == '', trace)

# Simultaneous PIO/CTC/strobe selects must stop before single-device handling.
for port in (0x00, 0x11, 0x20, 0x41, 0x80, 0x91, 0xa0, 0xc1):
    for opcode in (0xd3, 0xdb):
        # An even byte would otherwise be accepted as a vector by either chip.
        code = bytes([0x3e, 0xf8, opcode, port, 0x76])
        result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                                capture_output=True, check=True)
        trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
        check(trace[-1]['event'] == 'unmodelled_scanner_overlap', trace)
        check(trace[-1]['step'] == '2' and trace[-1]['pc_after'] == '', trace)
        check(int(trace[-1]['port']) == 0xf800 | port, trace)

# LCD RS comes from SA0, independently of data bit 2.
for port in (0x3e, 0x3f, 0xbe, 0xbf):
    for bus in (0x3b, 0x3f, 0x01):
        if port & 1:
            event = 'unmodelled_lcd_data'
        else:
            event = 'halt' if (bus & 0xe0) == 0x20 else 'unmodelled_lcd_command'
        code = bytes([0x3e, bus, 0xd3, port, 0x76])
        result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                                capture_output=True, check=True)
        trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
        check(trace[-1]['event'] == event, trace)

# Two immediate function writes violate the modeled LCD execution interval.
code = bytes([0x3e, 0x3b, 0xd3, 0xbe, 0xd3, 0x3e, 0x76])
result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                        capture_output=True, check=True)
trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
check(trace[-1]['event'] == 'unmodelled_lcd_busy' and trace[-1]['step'] == '3', trace)

# A spaced 8-to-4-bit transition and two-nibble function set complete.
# 40 NOPs provide 160 T-states between completed commands.
code = bytes([0x3e, 0x2b, 0xd3, 0xbe]) + bytes(40)
code += bytes([0xd3, 0xbe, 0x3e, 0x8b, 0xd3, 0x3e, 0x76])
result = subprocess.run(command, input=b'\x76' + bytes(8191) + code.ljust(8192, b'\0'),
                        capture_output=True, check=True)
trace = list(csv.DictReader(io.StringIO(result.stdout.decode('ascii'))))
check(trace[-1]['event'] == 'halt', trace)
