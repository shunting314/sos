#include <kernel/simfs.h>
#include <string.h>
#include <stdlib.h>

DirEnt* DirEntIterator::operator*() {
  assert(entIdx_ < parentDirEnt_->nchild());
  if (!loaded_) { 
    int logical_blk_idx = (entIdx_ * sizeof(DirEnt)) / BLOCK_SIZE;
    int32_t phys_blk_idx = parentDirEnt_->logicalToPhysBlockId(logical_blk_idx);
    SimFs::get().readBlock(phys_blk_idx, buf_);
    loaded_ = true;
  }
  return (DirEnt*) (buf_ + (entIdx_ * sizeof(DirEnt)) % BLOCK_SIZE);
}

DirEntIterator DirEnt::begin() const {
  assert(isdir());
  return DirEntIterator(this, 0);
}

DirEntIterator DirEnt::end() const {
  assert(isdir());
  return DirEntIterator(this, file_size / sizeof(DirEnt));
}

DirEnt DirEnt::findEnt(char *name, int len) const {
  if (len < 0 || len > NAME_BUF_SIZE - 1) {
    return DirEnt(); // not found
  }
  assert(isdir());
  assert(file_size % sizeof(DirEnt) == 0);

  for (auto curEntPtr : *this) {
    if (!strncmp(curEntPtr->name, name, len) && curEntPtr->name[len] == '\0') {
      return *curEntPtr;
    }
  }
  return DirEnt(); // not found
}

SimFs SimFs::instance_;

/*
 * buf should have enough space to store the entire block even if
 * len < BLOCK_SIZE. Treat len as an hint so readBlock can read less
 * data if possible.
 */
void SimFs::readBlock(int blockId, uint8_t buf[], int len) {
  assert(len > 0 && len <= BLOCK_SIZE);

  // TODO take advantage of len

  dev_.read(buf, blockIdToSectorNo(blockId), SECTORS_PER_BLOCK);
}

int SimFs::blockIdToSectorNo(int blockId) {
  return blockId * SECTORS_PER_BLOCK;
}

void SimFs::init() {
  // hardcode to use the slave IDE device for the filesystem for now
  dev_ = createSlaveIDE();
  uint8_t buf[BLOCK_SIZE];
  readBlock(0, buf, sizeof(SuperBlock));
  superBlock_ = *((SuperBlock*) buf);
  printf("Total number of block in super block %d\n", superBlock_.tot_block);
}

/*
 * parent does not need to be a dir. If path is empty, we return parent directly.
 */
DirEnt SimFs::walkPath(const DirEnt& parent, char* path) {
  assert(parent);
  assert(path);

  char *cur = path;
  while (*cur == '/') {
    ++cur;
  }
  if (!*cur) {
    // we are done!
    return parent;
  }

  if (!parent.isdir()) {
    printf("Directory required!\n");
    return DirEnt();
  }

  char *nxt = cur;
  while (*nxt != '/' && *nxt != '\0') {
    ++nxt;
  }
  auto child = parent.findEnt(cur, nxt - cur);
  if (!child) {
    return child;
  }
  return walkPath(child, nxt);
}

/*
 * return 0 on success
 */
int ls(char* path) {
  auto dent = SimFs::get().walkPath(path);
  if (!dent) {
    printf("Path does not exist %s\n", path);
    return -1;
  }
  bool found = false;
  for (auto itr : dent) {
    found = true;
    DirEnt* curEnt = &(*itr);
    printf("  %s - type %s - %d bytes\n", curEnt->name, curEnt->typestr(), curEnt->file_size);
  }
  if (!found) {
    printf("Empty directory!\n");
  }
  return 0;
}

/*
 * return 0 on success
 */
int cat(char* path) {
  auto dent = SimFs::get().walkPath(path);
  if (!dent) {
    printf("Path does not exist %s\n", path);
    return -1;
  }
  uint8_t buf[BLOCK_SIZE];
  for (int i = 0; i < dent.file_size; i += BLOCK_SIZE) {
    uint32_t logical_blkid = i / BLOCK_SIZE;
    uint32_t phys_blkid = dent.logicalToPhysBlockId(logical_blkid);
    SimFs::get().readBlock(phys_blkid, buf);

    int len = min(BLOCK_SIZE, dent.file_size - i);
    for (int j = 0; j < len; ++j) {
      printf("%c", (char)buf[j]);
    }
  }
  return 0;
}
