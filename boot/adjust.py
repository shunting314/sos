import sys
import os

# 1 sector excluding 4 partition table entries and 2 byte signature
code_size_limit = 512 - 16 * 4 - 2

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} file")
    sys.exit(1)

fname = sys.argv[1]
st = os.stat(fname)
print(f"Boot loader size before padding {st.st_size} (limit {code_size_limit})")
if st.st_size > code_size_limit:
    print("File too large: {st.st_size}")
    sys.exit(1)

fd = open(fname, "ab")

for _ in range(code_size_limit - st.st_size):
    fd.write(b"\x00")

def write_partition(fd):
    # write status byte
    fd.write(b"\x80")

    # write dummy start CHS
    fd.write(b"\x00\x00\x00")

    # write partition type
    fd.write(b"\x01") # 0 does not work

    # write dummy end CHS
    fd.write(b"\x00\x00\x00")

    # write dummy start linear block address
    fd.write(b"\x00\x00\x00\x00")

    # write dummy number of sectors
    fd.write(b"\x00\x00\x00\x00")

write_partition(fd)
for _ in range(48):
    fd.write(b"\x00")
fd.write(b"\x55\xaa")

sys.exit(0)
