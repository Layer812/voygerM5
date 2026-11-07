#!/usr/bin/env python3
"""Check current target partition table and app magic BEFORE flash. No device changes."""
import argparse, struct, sys, zlib
from pathlib import Path

EXPECTED = {
  "nvs": (0x01,0x02,0x9000,0x4000),
  "otadata": (0x01,0x00,0xD000,0x2000),
  "phy_init": (0x01,0x01,0xF000,0x1000),
  "ota_0": (0x00,0x10,0x20000,0x4F0000),
  "ota_1": (0x00,0x11,0x510000,0x4F0000),
  "storage": (0x01,0x81,0xA00000,0x400000),
  "coredump": (0x01,0x03,0xE00000,0x10000),
}

def validate_table(buf):
    found={}
    for off in range(0, len(buf)-31, 32):
        magic,typ,sub,addr,size,label,flags=struct.unpack_from('<HBBII16sI',buf,off)
        if magic==0xFFFF: break
        if magic==0xEBEB: break  # partition table MD5 entry
        if magic!=0x50AA:
            raise ValueError('Unknown partition entry at offset 0x%03x: 0x%04x' % (off, magic))
        name=label.split(b'\x00')[0].decode('ascii','replace')
        if name in found: raise ValueError('duplicate partition '+name)
        found[name]=(typ,sub,addr,size)
    if found!=EXPECTED:
        diffs=[]
        for name,v in EXPECTED.items():
            if found.get(name)!=v: diffs.append('%s: expected=%s actual=%s' % (name,v,found.get(name)))
        for name in found.keys()-EXPECTED.keys(): diffs.append('extra partition '+name)
        raise ValueError('; '.join(diffs))
    return found

def validate_boot_target(raw):
    """Conservative read-only check for the existing OTA0 boot target.

    ESP-IDF stores two 32-byte esp_ota_select_entry_t records at the start
    of two separate 4KB sectors. CRC is esp_rom_crc32_le(0xffffffff, seq, 4).
    With no factory partition and erased otadata, boot defaults to ota_0.
    Do NOT infer boot target from the partition table alone.
    """
    if len(raw)!=0x2000:
        raise ValueError('otadata read size is not 0x2000 bytes')
    selected=[]
    for sector in (0,1):
        record=raw[sector*0x1000:sector*0x1000+32]
        seq,state,crc=(struct.unpack_from('<I',record,0)[0],
                       struct.unpack_from('<I',record,24)[0],
                       struct.unpack_from('<I',record,28)[0])
        if record==b'\xff'*32:
            continue
        if seq==0 or seq==0xffffffff:
            raise ValueError('otadata sector %d has invalid/ambiguous seq' % sector)
        computed=zlib.crc32(record[:4],0xffffffff)&0xffffffff
        if crc!=computed:
            raise ValueError('otadata sector %d CRC mismatch' % sector)
        if state not in (2,0xffffffff):
            raise ValueError('otadata sector %d OTA state %d is not confirmed VALID/UNDEFINED' % (sector,state))
        selected.append((seq,sector))
    if not selected:
        # Only a fully erased otadata can safely use the OTA0 default.
        if raw[:32]!=b'\xff'*32 or raw[0x1000:0x1020]!=b'\xff'*32:
            raise ValueError('otadata is not in a recognized erased state')
        return 'ota_0 (initial erased otadata)'
    high=max(seq for seq,_ in selected)
    if len(selected)==2 and selected[0][0]==selected[1][0]:
        raise ValueError('OTA sequence tie; next boot target ambiguous')
    target=(high-1)%2
    if target!=0:
        raise ValueError('NEXT BOOT TARGET IS ota_1 (seq=%d), not ota_0; refuse to flash' % high)
    return 'ota_0 (validated OTA sequence %d)' % high

def validate_firmware(path):
    blob=Path(path).read_bytes()
    if len(blob)<24 or blob[0]!=0xE9:
        raise ValueError('Not an ESP32 app image: missing header/magic 0xE9')
    if not (1<=blob[1]<=16):
        raise ValueError('Bad ESP image segment count')
    chip_id=struct.unpack_from('<H',blob,12)[0]
    if chip_id!=9:
        raise ValueError('Wrong ESP chip ID %d (expected ESP32-S3 chip ID 9)' % chip_id)
    if len(blob)>0x4F0000:
        raise ValueError('Firmware larger than ota_0 0x4F0000 region')
    # first-byte magic is only an initial sanity check; bootloader performs full image validation
    return len(blob)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--dump',required=True)
    ap.add_argument('--otadata',required=True)
    ap.add_argument('--bin',required=True)
    args=ap.parse_args()
    try:
        parts=validate_table(Path(args.dump).read_bytes())
        boot=validate_boot_target(Path(args.otadata).read_bytes())
        n=validate_firmware(args.bin)
    except (OSError,ValueError) as e:
        print('ERROR: Recovery SAFETY CHECK FAILED:',e,file=sys.stderr)
        return 2
    print('PASS: target OTA table EXACT MATCH (7 partitions).')
    print('PASS: boot target is',boot)
    print('PASS: ESP32-S3 image header and size: %d bytes <= %d.' % (n, 0x4F0000))
    print('Authorized APP-ONLY destination: ota_0 at 0x20000. No other region.')
    return 0

if __name__=='__main__': sys.exit(main())
