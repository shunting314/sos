#!/usr/bin/env python

r"""
The code here must match the format defined in kernel/simfs.h .
"""

import argparse
from dataclasses import dataclass
import os
from typing import List, BinaryIO

BLOCK_SIZE = 4096
DIRENT_SIZE = 128
NDIRENT_PER_BLOCK = BLOCK_SIZE // DIRENT_SIZE
MAX_FILE_NAME = 64
N_DIRECT_BLOCK = 10
IND_BLOCK_IDX_2 = 11

@dataclass
class MkfsCtx:
    next_free_block: int
    tot_block: int
    img_file_fd: BinaryIO

    def allocate_block(self, nblock: int) -> List[int]:
        prev = self.next_free_block
        self.next_free_block += nblock
        assert self.next_free_block <= self.tot_block
        return list(range(prev, prev + nblock))

    def seek(self, pos: int):
        self.img_file_fd.seek(pos)

    def writeint(self, val: int, length: int = 4):
        self.img_file_fd.write(val.to_bytes(length=length, byteorder="little"))

    def write_to_blocks(self, blocks, bytes_to_write):
        assert len(bytes_to_write) <= len(blocks) * BLOCK_SIZE
        pos = 0
        while pos < len(bytes_to_write):
            assert pos % BLOCK_SIZE == 0
            block_id = blocks[pos // BLOCK_SIZE]
            self.img_file_fd.seek(block_id * BLOCK_SIZE)
            pos += self.img_file_fd.write(bytes_to_write[pos: BLOCK_SIZE])

    def setup_freelist(self) -> int:
        head = 0
        # reverse the free blocks since we prepending them.
        # the net effect is the free list has increasing block id initially.
        for blkid in reversed(range(self.next_free_block, self.tot_block)):
            # mark blkid pointing to head
            self.seek(blkid * BLOCK_SIZE)
            self.writeint(head, 4)
            head = blkid
        return head

def write_dirent(ctx: MkfsCtx, dirent_loc: int, name: str, size: int, blocklist: List[int], isdir: bool):
    ctx.img_file_fd.seek(dirent_loc)
    bin_name = name.encode("utf-8")
    assert len(bin_name) < MAX_FILE_NAME
    bin_name = bin_name + b"\0" * (MAX_FILE_NAME - len(bin_name))
    ctx.img_file_fd.write(bin_name)
    ctx.writeint(size)
    
    # TODO: handle large file by using indirect blocks
    assert len(blocklist) <= N_DIRECT_BLOCK
    blocktbl = blocklist + [0] * (IND_BLOCK_IDX_2 + 1 - len(blocklist))
    for bid in blocktbl:
        ctx.writeint(bid)

    ctx.writeint(isdir, 1) 

def handle_file(ctx: MkfsCtx, dirent_loc: int, curname: str, curpath: str):
    r"""
    Check the docstring for dfs
    """
    assert os.path.isfile(curpath)
    filesize = os.path.getsize(curpath)

    nalloc = (filesize + BLOCK_SIZE - 1) // BLOCK_SIZE  
    blocks = ctx.allocate_block(nalloc)
    write_dirent(ctx, dirent_loc, curname, filesize, blocks, False)

    with open(curpath, "rb") as f:
        ctx.write_to_blocks(blocks, f.read())

def dfs(ctx: MkfsCtx, dirent_loc: int, curname: str, curpath: str):
    r"""
    Do a preorder dfs to traverse the rootdir.

    dirent_loc: the location in the image file for the direntry of the current path.
    curname: the name of the dirent entry
    curpath: it's the path in the operating system that creates the image rather
             than the path in the context of the fs image.
    """

    if os.path.isfile(curpath):
        handle_file(ctx, dirent_loc, curname, curpath)
        return
    assert os.path.isdir(curpath)

    entry_list = os.listdir(curpath)
    start_loc = ctx.next_free_block * BLOCK_SIZE
    # round up
    nalloc = (len(entry_list) + NDIRENT_PER_BLOCK - 1) // NDIRENT_PER_BLOCK
    blocklist = ctx.allocate_block(nalloc)

    # set current dirent
    write_dirent(ctx, dirent_loc, curname, len(entry_list) * DIRENT_SIZE, blocklist, True)

    for i, child in enumerate(entry_list):
        dfs(ctx, start_loc + i * DIRENT_SIZE, child, os.path.join(curpath, child))

def mkfs(args: argparse.Namespace):
    with open(args.imgpath, "wb") as img_file_fd:
        ctx = MkfsCtx(
            next_free_block=1, # skip the super block
            tot_block=args.nblocks,
            img_file_fd=img_file_fd,
        )

        # preallocate the file size
        ctx.seek(args.nblocks * BLOCK_SIZE - 1)
        ctx.writeint(0, length=1)

        # rootdir DirEnt is at the beginning of the super block
        assert os.path.isdir(args.rootdir)
        dfs(ctx, 0, "", args.rootdir)

        # setup freelist and superblock. The dirent for rootdir has already been
        # set by dfs(...)
        freelist = ctx.setup_freelist()
        ctx.seek(DIRENT_SIZE)
        ctx.writeint(freelist, 4)
        ctx.writeint(args.nblocks, 4)

def main():
    parser = argparse.ArgumentParser(
        description="Format a filesystem image from the specified directory")
   
    parser.add_argument("rootdir", type=str, help="The root directory for the file system")
    parser.add_argument("imgpath", type=str, help="The path of the fs image to create")
    # by default create 16M size of image, which translate to 4096 blocks
    parser.add_argument("--nblocks", type=int, default=4096, help="The number of blocks the filesystem will have. Block size 4096.")
    parser.add_argument("--skip-prompt", action="store_true", help="Whether to skip prompt. Make it hard to erase the existing image by mistake.")
    args = parser.parse_args()

    if not args.skip_prompt:
        passphrase = "GOGOGO"
        print(f"Enter '{passphrase}' if you really want to recreate the fs image!")
        print("The command will exit directly if the input is wrong.")
        entered = input()
        if entered != passphrase:
            print("Wrong passphrase! Bye!")
            return

    mkfs(args)
    print(f"bye!")

if __name__ == "__main__":
    main()
