According to the author of ext3/ext4, ext4 is a stop gap. Btrfs is the better direction.

Since I don't plan to implement journaling for now, the filesystem I implemented will look more similar to ext2.

# Reference
- [ext - wikipedia](https://en.wikipedia.org/wiki/Extended_file_system)
- [ext2 - wikipedia](https://en.wikipedia.org/wiki/Ext2). Ext2 is the first commercial-grade filesystem for Linux.
- [ext3 - wikipedia](https://en.wikipedia.org/wiki/Ext3). Ext3 is a journaling file system.
- [ext4 - wikipedia](https://en.wikipedia.org/wiki/Ext4)
- [ext2_design.pdf](https://web.stanford.edu/class/cs240/old/sp2014/readings/ext2_design.pdf). ext2 author talks about design and implementation of ext2 in this paper.
