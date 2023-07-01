# Ext File System
According to the author of ext3/ext4, ext4 is a stop gap. Btrfs is the better direction.

Since I don't plan to implement journaling for now, the filesystem I implemented will look more similar to ext2.

# Simple File System (simfs)
Each directory entry has 10 directory blocks, 1 indicect block and 1 level-2 indirectory block. One indirect block contains '4K/4 = 1K' block ids; one level-2 indirect block contains 1K pointers to level-1 indirect blocks. In total, it points to 1M blocks storing data. This limits the max file/directory size to be around 4GB.

# Reference
- [ext - wikipedia](https://en.wikipedia.org/wiki/Extended_file_system)
- [ext2 - wikipedia](https://en.wikipedia.org/wiki/Ext2). Ext2 is the first commercial-grade filesystem for Linux.
- [ext3 - wikipedia](https://en.wikipedia.org/wiki/Ext3). Ext3 is a journaling file system.
- [ext4 - wikipedia](https://en.wikipedia.org/wiki/Ext4)
- [ext2_design.pdf](https://web.stanford.edu/class/cs240/old/sp2014/readings/ext2_design.pdf). ext2 author talks about design and implementation of ext2 in this paper.
