from pathlib import Path
import sys, struct, zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from verify_ota0 import validate_boot_target, validate_firmware

def mk(seq=None):
    raw=bytearray(b"\xff"*8192)
    if seq is not None:
        record=bytearray(b"\xff"*32)
        struct.pack_into('<I',record,0,seq)
        struct.pack_into('<I',record,24,2)  # ESP_OTA_IMG_VALID
        struct.pack_into('<I',record,28,zlib.crc32(record[:4],0xffffffff))
        raw[:32]=record
    return bytes(raw)
assert 'ota_0' in validate_boot_target(mk())
assert 'ota_0' in validate_boot_target(mk(1))
assert 'ota_0' in validate_boot_target(mk(3))
for raw in (mk(2), mk(4), bytes(8192)):
    try:validate_boot_target(raw);assert False, 'bad otadata accepted'
    except ValueError: pass
# Image chip header tests.
from tempfile import TemporaryDirectory
with TemporaryDirectory() as td:
    p=Path(td)/'firmware.bin'; buf=bytearray(64);buf[0]=0xe9;buf[1]=3;struct.pack_into('<H',buf,12,9)
    p.write_bytes(buf);assert validate_firmware(p)==64
    struct.pack_into('<H',buf,12,0);p.write_bytes(buf)
    try:validate_firmware(p);assert False,'wrong chip accepted'
    except ValueError:pass
print('PASS OTA0 boot slot selection, reject OTA1, validate S3 header')
