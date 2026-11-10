# SPDX-License-Identifier: AGPL-3.0-only
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import warnings
import zipfile

TOOL = Path(__file__).resolve().parents[2] / 'tools' / 'inspect_inputs.py'
spec = importlib.util.spec_from_file_location('inspect_inputs', TOOL)
inspector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inspector)


def synthetic_hfe(tracks=2):
    data = bytearray((2 + tracks) * 512)
    data[:8] = b'HXCPICFE'
    data[8:12] = bytes([0, tracks, 2, 3])
    data[16] = 11
    struct.pack_into('<H', data, 18, 1)
    for index in range(tracks):
        struct.pack_into('<HH', data, 512 + index * 4, 2 + index, 512)
    return data


class InputInspectorTests(unittest.TestCase):
    def test_hash_reference_overrides_misleading_filename(self):
        # Synthetic reference injection; no original firmware in tests.
        data = b'synthetic CPU image'
        sha1 = hashlib.sha1(data).hexdigest()
        with patch.dict(inspector.ROM_REFERENCES, {sha1: ('scanner', 'test', len(data))}):
            result = inspector.inspect_bytes('main_v9.img', data)
            self.assertEqual(result['kind'], 'scanner')
            self.assertEqual(result['evidence'], 'reference_hash_match')
            self.assertEqual(result['sha256'], hashlib.sha256(data).hexdigest())
            self.assertEqual(result['errors'], [])

    def test_bank_os_and_disk_candidates_are_not_boot_approvals(self):
        for name, size, kind in [('x.E2O', 72704, 'os_candidate'), ('x.eii', 485887, 'bank_candidate'),
                                 ('x.img', 573440, 'raw_disk_candidate')]:
            result = inspector.inspect_bytes(name, bytes(size))
            self.assertEqual(result['kind'], kind)
            self.assertEqual(result['evidence'], 'extension_and_size_only')
            self.assertTrue(inspector.inspect_bytes(name, bytes(size - 1))['errors'])
        self.assertEqual(inspector.inspect_bytes('unknown.rom', bytes(8192))['kind'], 'unidentified')

    def test_hfe_header_and_track_bounds(self):
        result = inspector.inspect_bytes('test.hfe', synthetic_hfe())
        self.assertEqual(result['errors'], [])
        self.assertEqual(result['evidence'], 'header_and_ranges_only')
        self.assertFalse(result['hfe']['eii_geometry_candidate'])
        self.assertTrue(inspector.inspect_bytes('bad.hfe', b'HXCPICFE')['errors'])
        self.assertTrue(inspector.inspect_bytes('bad.hfe', bytes(512))['errors'])
        self.assertTrue(inspector.inspect_bytes('truncated.hfe', synthetic_hfe()[:-1])['errors'])

    def test_hfe_overlap_empty_tracks_geometry_and_lut_are_rejected(self):
        mutations = [(8, 1), (9, 0), (10, 3)]
        for offset, value in mutations:
            data = synthetic_hfe(); data[offset] = value
            self.assertTrue(inspector.inspect_bytes('bad.hfe', data)['errors'])
        for track, block, length in [(0, 0, 512), (0, 2, 0), (1, 2, 512), (0, 65535, 512)]:
            data = synthetic_hfe(); struct.pack_into('<HH', data, 512 + track * 4, block, length)
            self.assertTrue(inspector.inspect_bytes('bad.hfe', data)['errors'])
        data = synthetic_hfe(); struct.pack_into('<H', data, 18, 65535)
        self.assertTrue(inspector.inspect_bytes('bad.hfe', data)['errors'])

    def test_duplicate_zip_member_names_preserve_both_records(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'duplicate.zip'
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', UserWarning)
                with zipfile.ZipFile(path, 'w') as z:
                    z.writestr('same.bin', b'first'); z.writestr('same.bin', b'second')
            records = inspector.inspect_file(path)
            self.assertEqual([r['member_index'] for r in records], [0, 1])
            self.assertNotEqual(records[0]['sha256'], records[1]['sha256'])
            self.assertEqual(sorted(p.name for p in Path(directory).iterdir()), ['duplicate.zip'])

    def test_crc_corruption_is_an_error_instead_of_identified_input(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'corrupt.zip'
            with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_STORED) as z:
                z.writestr('x.bin', b'payload')
            data = bytearray(path.read_bytes())
            name_len, extra_len = struct.unpack_from('<HH', data, 26)
            data[30 + name_len + extra_len] ^= 1
            path.write_bytes(data)
            records = inspector.inspect_file(path)
            self.assertTrue(records[0]['errors'])
            self.assertNotIn('sha256', records[0])

    def test_unsafe_members_and_zip_symlinks_are_not_read(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'unsafe.zip'
            with zipfile.ZipFile(path, 'w') as z:
                z.writestr('../escape.bin', b'no')
                z.writestr('/absolute.bin', b'no')
                link = zipfile.ZipInfo('link.bin'); link.create_system = 3
                link.external_attr = 0o120777 << 16
                z.writestr(link, b'/outside')
            records = inspector.inspect_file(path)
            self.assertEqual(len(records), 3)
            self.assertTrue(all(r['errors'] for r in records))
            self.assertEqual(len(list(Path(directory).iterdir())), 1)

    def test_archive_limits_are_checked_before_decompression(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'compressed.zip'
            with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_DEFLATED) as z:
                z.writestr('large.bin', bytes(1024))
            with patch.object(inspector, 'MAX_FILE', 100):
                self.assertTrue(inspector.inspect_file(path)[0]['errors'])
            with patch.object(inspector, 'MAX_MEMBERS', 0):
                self.assertTrue(inspector.inspect_file(path)[0]['errors'])
            with patch.object(inspector, 'MAX_ARCHIVE', 500):
                self.assertTrue(inspector.inspect_file(path)[0]['errors'])

    def test_invalid_archives_missing_paths_and_read_limit(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'bad.zip'; path.write_bytes(b'not a zip')
            self.assertEqual(inspector.inventory([path])['error_count'], 1)
            self.assertEqual(inspector.inventory([path.with_name('missing')])['error_count'], 1)
        with self.assertRaises(ValueError):
            inspector.read_limited(io.BytesIO(b'12345'), 4)

    def test_inventory_is_stable_and_does_not_modify_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'firmware.bin'; path.write_bytes(b'research data')
            before = path.read_bytes()
            one = inspector.inventory([directory]); two = inspector.inventory([directory])
            self.assertEqual(one, two)
            self.assertEqual(path.read_bytes(), before)
            self.assertEqual(one['purpose'], 'local_evidence_not_boot_approval')

    def test_cli_outputs_json_and_reports_invalid_input_exit_status(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'bad.e2o'; path.write_bytes(b'bad')
            run = subprocess.run([sys.executable, str(TOOL), str(path)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 1)
            self.assertEqual(json.loads(run.stdout)['error_count'], 1)
            path = Path(directory) / 'unknown.bin'; path.write_bytes(b'unknown')
            run = subprocess.run([sys.executable, str(TOOL), str(path)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0)
            self.assertEqual(json.loads(run.stdout)['records'][0]['kind'], 'unidentified')


if __name__ == '__main__':
    unittest.main()
