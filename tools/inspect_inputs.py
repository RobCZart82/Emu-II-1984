#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Inspect local firmware and media or verify an explicit main/scanner ROM pair."""
import argparse
import hashlib
import json
import lzma
from pathlib import Path, PurePosixPath, PureWindowsPath
import stat
import struct
import sys
import zipfile
import zlib

MAX_FILE = 8 * 1024 * 1024
MAX_ARCHIVE = 64 * 1024 * 1024
MAX_MEMBERS = 512
EXTENSIONS = {'.bin', '.rom', '.ic28', '.ic42', '.e2o', '.eii', '.img', '.emuiifd', '.hfe', '.hex'}
# File-level metadata from MAME's emu2.cpp, pin documented in SOURCE_AUDIT.md.
ROM_REFERENCES = {
    '4ec4bf6df95638d4503ab8ff7e859c420ec0720e': ('main_boot', '2.3', 8192),
    'a2332266d6a12eade584b5da85817f3b3ed9037c': ('main_boot', '2.1', 8192),
    'd572d88c8788e25154a3bb8fd2ba3ff8a300ae40': ('main_diagnostics', 'IP341', 8192),
    '4485e5423606014be73f121c2ee1f053a6eb148a': ('scanner', '2.1', 8192),
    'e354388cd694a1ce55a5b7d89c546308628090c7': ('scanner', '3.1', 8192),
}


def inspect_bytes(name, data):
    """Return evidence only. A candidate is never an approved boot input."""
    sha1 = hashlib.sha1(data).hexdigest()
    result = {'size': len(data), 'sha1': sha1,
              'sha256': hashlib.sha256(data).hexdigest(),
              'kind': 'unidentified', 'evidence': 'hashes_only', 'errors': []}
    reference = ROM_REFERENCES.get(sha1)
    if reference and len(data) == reference[2]:
        result.update(kind=reference[0], version=reference[1], evidence='reference_hash_match')
        return result
    ext = Path(name).suffix.lower()
    sizes = {'.e2o': (72704, 'os_candidate'), '.eii': (485887, 'bank_candidate'),
             '.img': (573440, 'raw_disk_candidate'), '.emuiifd': (573440, 'raw_disk_candidate')}
    if ext in sizes:
        expected, kind = sizes[ext]
        if len(data) == expected:
            result.update(kind=kind, evidence='extension_and_size_only')
        else:
            result['errors'].append(f'unsupported size for {ext}: expected {expected}')
    elif ext == '.hfe':
        try:
            result['hfe'] = inspect_hfe(data)
            result.update(kind='hfe_container', evidence='header_and_ranges_only')
        except ValueError as error:
            result['errors'].append(str(error))
    return result


def inspect_hfe(data):
    """Bounds-check HFE v1 storage. Does not decode tracks or check FM CRCs."""
    if len(data) < 512 or data[:8] != b'HXCPICFE':
        raise ValueError('missing/truncated HFE v1 header')
    revision, tracks, sides, encoding = data[8:12]
    if revision != 0 or tracks == 0 or sides not in (1, 2):
        raise ValueError('unsupported HFE revision or geometry')
    table = struct.unpack_from('<H', data, 18)[0] * 512
    if table < 512 or table + tracks * 4 > len(data):
        raise ValueError('HFE track table outside data area')
    # Track storage cannot overlap the rounded-up LUT blocks or another track.
    table_end = ((table + tracks * 4 + 511) // 512) * 512
    ranges = []
    for index in range(tracks):
        block, length = struct.unpack_from('<HH', data, table + index * 4)
        start = block * 512
        end = start + ((length + 511) // 512) * 512
        if length == 0 or start < table_end or end > len(data):
            raise ValueError(f'HFE track {index} has an invalid storage range')
        ranges.append((start, end))
    ordered = sorted(ranges)
    if any(left[1] > right[0] for left, right in zip(ordered, ordered[1:])):
        raise ValueError('overlapping HFE track storage')
    return {'revision': revision, 'tracks': tracks, 'sides': sides, 'encoding': encoding,
            'interface': data[16], 'track_ranges_valid': True,
            'eii_geometry_candidate': tracks == 80 and sides == 2 and encoding == 3 and data[16] == 11}


def read_limited(stream, limit):
    data = stream.read(limit + 1)
    if len(data) > limit:
        raise ValueError('uncompressed input exceeds size limit')
    return data


def inspect_file(path):
    source = str(path)
    records = []
    try:
        if path.is_symlink():
            raise ValueError('symbolic links are not inspected')
        limit = MAX_ARCHIVE if path.suffix.lower() == '.zip' else MAX_FILE
        metadata = path.stat()
        if not stat.S_ISREG(metadata.st_mode):
            raise ValueError('input is not a regular file')
        if metadata.st_size > limit:
            raise ValueError('input exceeds size limit')
        if path.suffix.lower() == '.zip':
            with zipfile.ZipFile(path) as archive:
                members = archive.infolist()
                if len(members) > MAX_MEMBERS or sum(m.file_size for m in members) > MAX_ARCHIVE:
                    raise ValueError('archive exceeds member count or expanded size limit')
                for index, member in enumerate(members):
                    if member.is_dir():
                        continue
                    record = {'source': source, 'member': member.filename, 'member_index': index}
                    try:
                        member_path = PurePosixPath(member.filename.replace('\\', '/'))
                        mode = member.external_attr >> 16
                        if (member_path.is_absolute() or PureWindowsPath(member.filename).is_absolute()
                                or '..' in member_path.parts or stat.S_ISLNK(mode)):
                            raise ValueError('unsafe archive member path/type')
                        if member.file_size > MAX_FILE:
                            raise ValueError('archive member exceeds size limit')
                        # Read even non-media entries: CRC validation and a complete hash inventory.
                        with archive.open(member) as stream:
                            data = read_limited(stream, MAX_FILE)
                        record.update(inspect_bytes(member.filename, data))
                    except (OSError, ValueError, RuntimeError, zipfile.BadZipFile,
                            NotImplementedError, zlib.error, lzma.LZMAError) as error:
                        record['errors'] = [str(error)]
                    records.append(record)
        else:
            with path.open('rb') as stream:
                records.append({'source': source, **inspect_bytes(path.name, read_limited(stream, MAX_FILE))})
    except (OSError, ValueError, RuntimeError, zipfile.BadZipFile, NotImplementedError, zlib.error) as error:
        records.append({'source': source, 'errors': [str(error)]})
    return records


def inventory(paths):
    files = set()
    errors = []
    for path in paths:
        path = Path(path)
        if path.is_symlink() or not path.exists():
            errors.append({'source': str(path), 'errors': ['missing path or symbolic link']})
        elif path.is_dir():
            for candidate in path.rglob('*'):
                if candidate.suffix.lower() in EXTENSIONS | {'.zip'} and candidate.is_file():
                    files.add(candidate)
        else:
            files.add(path)
    records = errors
    for path in sorted(files, key=str):
        records.extend(inspect_file(path))
    return {'schema_version': 1, 'purpose': 'local_evidence_not_boot_approval',
            'records': records, 'error_count': sum(bool(r['errors']) for r in records)}


def load_verified_rom(path, role):
    """Return the exact verified bytes and their identity from one bounded read."""
    path = Path(path)
    if path.is_symlink() or not stat.S_ISREG(path.stat().st_mode):
        raise ValueError(f'{role}: expected a regular ROM file: {path}')
    expected_size = 8192
    with path.open('rb') as stream:
        data = stream.read(expected_size + 1)
    if len(data) != expected_size:
        raise ValueError(f'{role}: expected an 8192-byte ROM: {path}')
    evidence = inspect_bytes(path.name, data)
    if evidence['evidence'] != 'reference_hash_match':
        raise ValueError(f'{role}: unknown ROM identity: {path}')
    if evidence['kind'] != role:
        raise ValueError(f"{role}: selected ROM belongs to {evidence['kind']}: {path}")
    return data, {'source': str(path), **evidence}


def load_rom_pair(main_path, scanner_path):
    """Validate both roles before handing any bytes to a consumer."""
    main_rom = load_verified_rom(main_path, 'main_boot')
    scanner_rom = load_verified_rom(scanner_path, 'scanner')
    return main_rom, scanner_rom


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('paths', nargs='*', type=Path, help='files or directories to inventory')
    parser.add_argument('--main-rom', type=Path, help='explicit main boot ROM file')
    parser.add_argument('--scanner-rom', type=Path, help='explicit scanner ROM file')
    args = parser.parse_args(argv)
    pair_mode = args.main_rom is not None or args.scanner_rom is not None
    if pair_mode:
        if args.paths or args.main_rom is None or args.scanner_rom is None:
            parser.error('use both --main-rom and --scanner-rom without inventory paths')
        try:
            (_, main), (_, scanner) = load_rom_pair(args.main_rom, args.scanner_rom)
        except (OSError, ValueError) as error:
            print(str(error), file=sys.stderr)
            return 1
        report = {'schema_version': 1, 'purpose': 'verified_rom_selection',
                  'records': [main, scanner], 'error_count': 0}
    elif not args.paths:
        parser.error('provide inventory paths or an explicit ROM pair')
    else:
        try:
            report = inventory(args.paths)
        except OSError as error:
            print(str(error), file=sys.stderr)
            return 2
    json.dump(report, sys.stdout, ensure_ascii=True, indent=2)
    sys.stdout.write('\n')
    return 1 if report['error_count'] else 0


if __name__ == '__main__':
    sys.exit(main())
