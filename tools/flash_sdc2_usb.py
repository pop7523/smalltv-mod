"""Back up all 4 MB before writing SmallTV SDC2. Run with --port COM5."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',required=True)
    a=p.parse_args();root=Path(__file__).resolve().parent
    image=root/'SmallTV_SDC2_USB.factory.bin'
    if image.stat().st_size!=0x400000:raise SystemExit('Wrong factory image size')
    sums={line.split()[1]:line.split()[0] for line in (root/'SHA256SUMS.txt').read_text().splitlines()}
    if hashlib.sha256(image.read_bytes()).hexdigest()!=sums.get(image.name):raise SystemExit('Factory checksum mismatch')
    report=json.loads((root/'image-report.json').read_text())
    if report.get('image_checks')!='PASS' or report.get('chip_id')!=12:raise SystemExit('Image report mismatch')
    command=[sys.executable,'-m','esptool','--chip','esp32c2','--port',a.port,'--baud','115200']
    backup=root/('SDC2-backup-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.bin')
    subprocess.run(command+['read-flash','0x0','0x400000',str(backup)],check=True)
    if backup.stat().st_size!=0x400000:raise SystemExit('Backup incomplete; no write performed')
    backup.with_suffix('.sha256').write_text(hashlib.sha256(backup.read_bytes()).hexdigest()+'\n')
    print('Backup saved:',backup)
    subprocess.run(command+['write-flash','0x0',str(image)],check=True)
    print('Write complete. Join SmallTV-Setup and open http://192.168.4.1')
if __name__=='__main__':main()
