"""Merge only the actual compiled SDC2 USB flash inputs into a 4 MB image."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
from inspect_sdc2_image import inspect

EXPECTED=[("nvs",0x9000,0x5000),("otadata",0xe000,0x2000),
          ("app0",0x10000,0x180000),("app1",0x190000,0x180000),("spiffs",0x310000,0xf0000)]

def partitions(raw):
    rows=[]
    for offset in range(0,len(raw),32):
        if raw[offset:offset+2]!=b"\xaa\x50":break
        magic,kind,sub,start,size,label,flags=struct.unpack_from("<HBBII16sI",raw,offset)
        rows.append((label.split(b"\0")[0].decode(),start,size))
    if rows!=EXPECTED:raise ValueError(f"Unexpected partition layout: {rows}")
    return rows

def merge(segments):
    factory=bytearray(b"\xff"*0x400000)
    last=0
    for offset,blob in sorted(segments):
        if offset<last or offset<0 or offset+len(blob)>len(factory):raise ValueError("Overlapping or out-of-range flash segment")
        factory[offset:offset+len(blob)]=blob
        last=offset+len(blob)
    return factory

def main():
    p=argparse.ArgumentParser();p.add_argument("build",type=Path);p.add_argument("output",type=Path);a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True)
    report=inspect(a.build/"firmware.bin",slot_size=0x180000)
    inputs=json.loads((a.build/"sdc2-flash-inputs.json").read_text())
    segments=[]
    for entry in inputs:
        offset=entry["offset"];blob=Path(entry["path"]).read_bytes()
        if offset not in (0,0x8000,0xe000,0x10000):raise ValueError("Unexpected PlatformIO flash offset")
        if offset==0x8000:partitions(blob)
        segments.append((offset,blob))
    if not {0,0x8000,0x10000}.issubset({o for o,b in segments}):raise ValueError("Missing bootloader, partition table or application")
    fs=(a.build/"littlefs.bin").read_bytes()
    if len(fs)!=0xf0000:raise ValueError("Wrong filesystem size")
    segments.append((0x310000,fs))
    name="SmallTV_SDC2_USB.factory.bin"
    (a.output/name).write_bytes(merge(segments))
    shutil.copy2(a.build/"firmware.bin",a.output/"SmallTV_SDC2_NEW_LAYOUT_OTA.bin")
    report.update(stock_ota_compatible=False,usb_initial_install_required=True,app_slot_bytes=0x180000,
                  factory_bytes=0x400000,stock_clock_sources_compiled=False)
    (a.output/"image-report.json").write_text(json.dumps(report,indent=2))
    lines=[hashlib.sha256(f.read_bytes()).hexdigest()+"  "+f.name for f in sorted(a.output.glob("*.bin"))]
    (a.output/"SHA256SUMS.txt").write_text("\n".join(lines)+"\n")
    print(json.dumps(report,indent=2))

if __name__=="__main__":main()
