#!/usr/bin/env python3.6
import sys
import re

def resolve_reloc(outlist, addr_map, reloc_map):
    for addr, (reltype, sym) in reloc_map.items():
        assert sym in addr_map
        symaddr = addr_map[sym]
        assert symaddr < 65536

        off = addr - 0x7c00
        if reltype == "X86_64_RELOC_UNSIGNED":
            outlist[off] = symaddr % 256
            outlist[off + 1] = symaddr // 256
        elif reltype == "X86_64_RELOC_BRANCH":
            rel16 = symaddr - (addr + 2)
            outlist[off] = rel16 % 256
            outlist[off + 1] = rel16 // 256
        else:
            raise runtime_error(f"Unsupported relocation type {reltype}")


if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} in-path out-path")
    sys.exit(1)

inpath = sys.argv[1]
outpath = sys.argv[2]
outlist = []
reloc_map = {} # addr -> (reltype, sym)
addr_map = {} # sym -> addr
with open(inpath) as inf:
    for line in inf.readlines():
        line = line.rstrip()

        print(f"Line: {line}")
        # add to addr_map. Example: "0000000000007c15 msg:"
        m = re.match("([A-Fa-f0-9]+) ([A-Za-z_][A-Za-z0-9_]*):", line)
        if m:
            addr = int(m[1], 16)
            assert addr < 65536
            addr_map[m[2]] = addr
            continue

        # add to reloc_map. Example: "  0000000000007c0d:  X86_64_RELOC_UNSIGNED  msg"
        m = re.match(r"[ \t]*([A-Fa-f0-9]+):[ \t]*(X86_64_RELOC_(UNSIGNED|BRANCH))[ \t]+([A-Za-z_][A-Za-z0-9_]*)", line)
        if m:
            addr = int(m[1], 16)
            reloc_map[addr] = (m[2], m[4])
            continue

        # assembly instruction. Example: "  7c2b: 65 6c  insb    %dx, %es:(%rdi)"
        m = re.match(r"[ \t]*[A-Fa-f0-9]+:(( [A-Fa-f0-9]{2})+)", line)
        if m:
            print(f"Match: {m[1]}")
            for dual in m[1].strip().split(' '):
                dual = dual.lower()
                assert len(dual) == 2
                outlist.append(int(dual, 16))
    assert len(outlist) <= 510
    outlist.extend([0] * (510 - len(outlist)) + [0x55, 0xaa])

resolve_reloc(outlist, addr_map, reloc_map)
with open(outpath, 'wb') as outf:
    outf.write(bytes(outlist))
print(addr_map)
print(reloc_map)
