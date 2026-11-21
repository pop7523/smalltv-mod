"""Test the actual USB merger and partition table checks, with negative cases."""
from package_sdc2_usb import merge,partitions,EXPECTED
import struct
def table(rows):
    return b''.join(struct.pack('<HBBII16sI',0x50aa,0 if name.startswith('app') else 1,0,offset,size,name.encode(),0) for name,offset,size in rows)
assert partitions(table(EXPECTED))==EXPECTED
bad=EXPECTED.copy();bad[2]=('app0',0x10000,0xc0000)
for rows in (bad,EXPECTED[:-1]):
    try:partitions(table(rows))
    except ValueError:pass
    else:raise AssertionError('wrong partition table accepted')
assert merge([(0,b'boot'),(0x10000,b'app')])[0x10000:0x10003]==b'app'
for segments in ([(0,b'ab'),(1,b'cd')],[(-1,b'x')],[(0x400000,b'x')]):
    try:merge(segments)
    except ValueError:pass
    else:raise AssertionError('invalid flash segments accepted')
print('PASS: exact new partition table, stock layout rejection, overlap and flash bounds')
