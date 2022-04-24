#pragma once
/*
 * A simple file system (simfs).
 *
 * Concept:
 * We define logical block id as the index of the block relative to the beginning
 * of the file/directory. E.g. the first block for a file will have logical block id 0.
 * We define physical block id as the index of the block in the device which is
 * independent to any file/directory.
 */

#include <stdint.h>
#include <kernel/ide.h>

// max file/subdir name size 63 following by '\0'
#define NAME_BUF_SIZE 64
#define BLOCK_SIZE 4096
#define SECTORS_PER_BLOCK ((BLOCK_SIZE / SECTOR_SIZE))
/*
 * The max file will contains N_DIRECT_BLOCK + BLOCK_SIZE / 4 + BLOCK_SIZE / 4 * BLOCK_SIZE / 4 = 1049610 blocks
 * which is 4299202560 bytes (~= 4GiB)
 */
#define N_DIRECT_BLOCK 10
// indirect block index
#define IND_BLOCK_IDX_1 (N_DIRECT_BLOCK)
#define IND_BLOCK_IDX_2 (IND_BLOCK_IDX_1 + 1)

enum DIR_ENT_TYPE {
  ET_FILE, // regular file
  ET_DIR, // subdirectory
  ET_NOEXIST,
};

class DirEntIterator;

class DirEnt {
 public:
  explicit DirEnt() : ent_type(ET_NOEXIST) {
  }
  char name[NAME_BUF_SIZE]; // 64 bytes
  uint32_t file_size; // 4 bytes
  uint32_t blktable[IND_BLOCK_IDX_2 + 1]; // 12 * 4 = 48 bytes
  int8_t ent_type; // check DIR_ENT_TYPE
  char padding[128 - (NAME_BUF_SIZE + 4 + (IND_BLOCK_IDX_2 + 1) * 4 + 1)]; // pad to 128 bytes

  const char* typestr() const {
    switch (ent_type) {
      case ET_FILE:
        return "f";
      case ET_DIR:
        return "d";
      default:
        assert(false && "Invalid DirEnt type");
        return nullptr;
    }
  }

  int nchild() const {
    assert(isdir());
    return file_size / sizeof(DirEnt);
  }

  DirEntIterator begin() const;
  DirEntIterator end() const;

  DirEnt findEnt(char *name, int len) const;

  bool isdir() const {
    return ent_type == ET_DIR;
  }

  uint32_t logicalToPhysBlockId(uint32_t logicalBlockId) const {
    assert(logicalBlockId < N_DIRECT_BLOCK); // TODO support large file that have indirct block
    return blktable[logicalBlockId];
  }

  // use ent_type == ET_NOEXIST to identify a returned DirEnt does not exist
  operator bool() const {
    return ent_type != ET_NOEXIST;
  }
} __attribute__((packed));

#define NDIR_ENT_PER_BLOCK (BLOCK_SIZE / sizeof(DirEnt))
static_assert(sizeof(DirEnt) == 128);
static_assert(NDIR_ENT_PER_BLOCK == 32);

/*
 * TODO:
 * NOTE that the iterator stores a buffer of BLOCK_SIZE. It's *NOT* lightweight.
 * We can avoid an inline buffer once we have malloc/free in kernel mode.
 *
 * All the API that changes entIdx_ need to invaliate loaded_ if needed.
 */
class DirEntIterator {
 public:
  explicit DirEntIterator(const DirEnt* parentDirEnt, int entIdx) : parentDirEnt_(parentDirEnt), entIdx_(entIdx), loaded_(false) {
    assert(parentDirEnt->isdir());
  }

  // return a pointer point to somewhere in buf_.
  // The next call to operator* may reload buf_ and invalidate the previous
  // returned DirEnt*
  DirEnt* operator*();

  int getEntIdx() const { return entIdx_; }

  // prefix increment
  DirEntIterator& operator++() {
    ++entIdx_;
    if (entIdx_ % NDIR_ENT_PER_BLOCK == 0) {
      loaded_ = false;
    }
    return *this;
  }

  // postfix increment
  // TODO: how to make it more efficient regarding the memory cost of buf_
  // or we should always use prefix increment and disable the postfix increment?
  /* // disable postfix increment
  DirEntIterator operator++(int) {
    assert(false && "don't use postfix increment for DirEntIterator");
  }
  */
 private:
  uint8_t buf_[BLOCK_SIZE];
  const DirEnt* parentDirEnt_;
  int entIdx_;
  bool loaded_; // if buf_ is loaded with the correct data
};

/*
 * There are 2 strategies to maintain the free blocks:
 * 1. allocate a bitmap to track the free blocks
 * 2. chain the free blocks into a free list
 *
 * The first stragey has big advantage over the second. When free the blocks for a huge file, 
 * the former only requires updating the bitmap which only span across a few blocks but the later
 * requires a disk write for each of the blocks freed.
 * Also for the second strategy, if any free block is corrupted, the damage is huge.
 *
 * I'll still go on to implement strategy 2 though. The main purpose is to experiment the idea and see how that works
 * in practice. We should have multiple alternative filesystem implementations later on anyways.
 * So it's not a big deal.
 */
class SuperBlock {
 public:
  DirEnt rootdir;
  uint32_t freelist;
  uint32_t tot_block;
} __attribute__((packed));

static_assert(sizeof(SuperBlock) <= BLOCK_SIZE); // make sure the SuperBlock can be put inside the first block

class SimFs {
 public:
  // TODO Detect the FS object from path rather than assuming the singleton SimFs instance?
  static SimFs& get() {
    return SimFs::instance_;
  }
  // initialize the superblock
  void init();
  // the blockId here is a physical block id
  void readBlock(int blockId, uint8_t buf[], int len = BLOCK_SIZE);
  // walk path from rootdir
  DirEnt walkPath(char* path) {
    return walkPath(superBlock_.rootdir, path);
  }

  // walk path from the specified parent dir
  // path is relative to parent
  DirEnt walkPath(const DirEnt& parent, char* path);
 private:
  static int blockIdToSectorNo(int blockId);

  static SimFs instance_;
  IDEDevice dev_;
  SuperBlock superBlock_;
};

int ls(char* path);
int cat(char* path);

static inline bool operator==(const DirEntIterator& lhs, const DirEntIterator& rhs) {
  return lhs.getEntIdx() == rhs.getEntIdx();
}
static inline bool operator!=(const DirEntIterator& lhs, const DirEntIterator& rhs) {
  return !(lhs.getEntIdx() == rhs.getEntIdx());
}
