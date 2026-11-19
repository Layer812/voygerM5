#!/usr/bin/env python3
"""Check current target partition table and app magic BEFORE flash. No device changes."""
import argparse, struct, sys, zlib
from pathlib import Path

# Whitelist only layouts confirmed by real StopWatch partition dumps.
# Layout changes here NEVER write partition tables or otadata to the device.
BASE = {
  "nvs": (0x01, 0x02, 0x9000, 0x4000),
  "otadata": (0x01, 0x00, 0xD000, 0x2000),
  "phy_init": (0x01, 0x01, 0xF000, 0x1000),
  "coredump": (0x01, 0x03, 0xE00000, 0x10000),
}
LAYOUTS = {
  "SINGLE_OTA_3MB": {
    **BASE,
    "ota_0": (0x00, 0x10, 0x20000, 0x300000),
    "storage": (0x01, 0x81, 0x320000, 0xAE0000),
  },
  "DUAL_OTA_4FMB": {
    **BASE,
    "ota_0": (0x00, 0x10, 0x20000, 0x4F0000),
    "ota_1": (0x00, 0x11, 0x510000, 0x4F0000),
    "storage": (0x01, 0x81, 0xA00000, 0x400000),
  },
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
    # Validate all boundaries even on a whitelisted layout.
    if len(buf)!=0x1000:
        raise ValueError('partition table dump must contain exactly 4096 bytes')
    regions=[]
    for name,(typ,sub,addr,size) in found.items():
        if addr<0x9000 or not size or addr+size>0x1000000:
            raise ValueError('out-of-range partition %s' % name)
        alignment=0x10000 if typ==0 else 0x1000
        if addr%alignment or size%0x1000:
            raise ValueError('misaligned partition %s' % name)
        if typ==0 and size%0x10000:
            raise ValueError('app partition size not 64KB aligned: %s' % name)
        regions.append((addr,addr+size,name))
    regions.sort()
    for a,b in zip(regions,regions[1:]):
        if a[1]>b[0]:
            raise ValueError('overlapping partitions %s and %s' % (a[2],b[2]))
    for layout, expected in LAYOUTS.items():
        if found == expected:
            return layout, found
    # R13 FIX3-compatible factory-only mode: dynamically select the sole
    # type=APP/subtype=FACTORY entry. No OTA slot or OTA-data assumption.
    app_entries=[(name,part) for name,part in found.items() if part[0]==0]
    if len(app_entries)==1 and app_entries[0][1][:2]==(0,0):
        name,app=app_entries[0]
        if app[2]<0x10000 or app[3]<0x10000:
            raise ValueError('invalid factory APP range')
        # No OTA APP means this factory entry is the sole executable slot.
        return 'FACTORY_ONLY', found
    # Fail closed on other mixed/unknown OTA layouts or multiple APP slots.
    possibilities=[]
    for layout,expected in LAYOUTS.items():
        diffs=['%s: expected=%s actual=%s' % (name,v,found.get(name))
               for name,v in expected.items() if found.get(name)!=v]
        diffs += ['extra partition '+name for name in found.keys()-expected.keys()]
        possibilities.append('%s [%s]' % (layout,'; '.join(diffs)))
    raise ValueError('Unrecognized partition table. ' + ' OR '.join(possibilities))

def validate_boot_target(raw, num_ota_slots):
    """Conservative read-only check for the existing OTA0 boot target.

    ESP-IDF stores two 32-byte esp_ota_select_entry_t records at the start
    of two separate 4KB sectors. CRC is esp_rom_crc32_le(0xffffffff, seq, 4).
    With no factory partition and erased otadata, boot defaults to ota_0.
    Do NOT infer boot target from the partition table alone.
    """
    if len(raw)!=0x2000:
        raise ValueError('otadata read size is not 0x2000 bytes')
    if num_ota_slots not in (1,2):
        raise ValueError('unsupported OTA app count')
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
        if raw!=b'\xff'*0x2000:
            raise ValueError('otadata records missing, but partition is not fully erased')
        return 'ota_0 (fully erased otadata, no factory APP)'
    high=max(seq for seq,_ in selected)
    if len(selected)==2 and selected[0][0]==selected[1][0]:
        raise ValueError('OTA sequence tie; next boot target ambiguous')
    target=(high-1)%num_ota_slots
    if target!=0:
        raise ValueError('NEXT BOOT TARGET IS ota_%d (seq=%d); refuse to flash' % (target,high))
    return 'ota_0 (validated OTA sequence %d; %d OTA slot(s))' % (high,num_ota_slots)

def validate_firmware(path, slot_size):
    blob=Path(path).read_bytes()
    if len(blob)<24 or blob[0]!=0xE9:
        raise ValueError('Not an ESP32 app image: missing header/magic 0xE9')
    if not (1<=blob[1]<=16):
        raise ValueError('Bad ESP image segment count')
    chip_id=struct.unpack_from('<H',blob,12)[0]
    if chip_id!=9:
        raise ValueError('Wrong ESP chip ID %d (expected ESP32-S3 chip ID 9)' % chip_id)
    if len(blob)>slot_size:
        raise ValueError('Firmware (%d bytes) exceeds selected APP size (%d bytes)' % (len(blob), slot_size))
    # first-byte magic is only an initial sanity check; bootloader performs full image validation
    return len(blob)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--dump',required=True)
    ap.add_argument('--otadata',required=True)
    ap.add_argument('--bin',required=True)
    ap.add_argument('--target-file',required=True)
    ap.add_argument('--existing-header',help='Read-only dump of the existing APP header for final preflash check')
    args=ap.parse_args()
    try:
        layout, parts=validate_table(Path(args.dump).read_bytes())
        if layout=='FACTORY_ONLY':
            # Partitions were validated and exactly one APP exists.
            # A factory-only device boots that APP without depending on OTA data.
            factory=[(name,p) for name,p in parts.items() if p[:2]==(0,0)]
            name,selected=factory[0]
            boot='factory (sole APP; no OTA APP entries)'
        else:
            num_slots=int('ota_0' in parts)+int('ota_1' in parts)
            boot=validate_boot_target(Path(args.otadata).read_bytes(), num_slots)
            name='ota_0'
            selected=parts[name]
        n=validate_firmware(args.bin, selected[3])
        if args.existing_header:
            existing=Path(args.existing_header).read_bytes()
            if len(existing)<24 or existing[0]!=0xE9:
                raise ValueError('existing selected APP header is absent/invalid')
            if struct.unpack_from('<H',existing,12)[0]!=9:
                raise ValueError('existing selected APP is not ESP32-S3')
        Path(args.target_file).write_text('0x%X 0x%X\n' %
                                           (selected[2],selected[3]),encoding='ascii')
    except (OSError,ValueError) as e:
        print('ERROR: Recovery SAFETY CHECK FAILED:',e,file=sys.stderr)
        return 2
    print('PASS: validated partition layout %s (%d partitions).' % (layout,len(parts)))
    print('PASS: selected boot APP:',boot)
    print('PASS: ESP32-S3 image header and size: %d bytes <= %d.' % (n,selected[3]))
    if args.existing_header:
        print('PASS: existing APP header is an ESP32-S3 image.')
    print('Authorized APP-ONLY target: %s at 0x%X, size 0x%X.' %
          (name,selected[2],selected[3]))
    return 0

if __name__=='__main__': sys.exit(main())
