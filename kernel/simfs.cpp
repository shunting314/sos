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

uint32_t DirEnt::logicalToPhysBlockId(uint32_t logicalBlockId) const {
  if (logicalBlockId < N_DIRECT_BLOCK) {
    return blktable[logicalBlockId];
  } else if (logicalBlockId < N_DIRECT_BLOCK + BLOCK_SIZE / 4) {
    // load the indirect block. 
    // TODO Should we cache it?
    // TODO Can we not allocate the block on stack?
    uint32_t buf[BLOCK_SIZE / 4];
    SimFs::get().readBlock(blktable[IND_BLOCK_IDX_1], (uint8_t*) buf);
    return buf[logicalBlockId - N_DIRECT_BLOCK];
  } else {
    assert(false && "can not fully support level-2 indirect block yet");
  }
}

DirEntIterator DirEnt::begin() const {
  assert(isdir());
  return DirEntIterator(this, 0);
}

DirEntIterator DirEnt::end() const {
  assert(isdir());
  return DirEntIterator(this, file_size / sizeof(DirEnt));
}

DirEnt DirEnt::findEnt(const char *name, int len) const {
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

// TODO: avoid duplicate code with findEnt by returning a pair of DirEnt and index
// for findEnt
int DirEnt::findEntIdx(const char* name, int len) const {
  if (len < 0 || len > NAME_BUF_SIZE - 1) {
    return -1; // not found
  }
  assert(isdir());
  assert(file_size % sizeof(DirEnt) == 0);

	int idx = 0;
  for (auto curEntPtr : *this) {
    if (!strncmp(curEntPtr->name, name, len) && curEntPtr->name[len] == '\0') {
      return idx;
    }
		++idx;
  }
	return -1;
}

// fail if the entry already exists
DirEnt DirEnt::createEnt(const char* path, int pathlen, const char *name, int namelen, int8_t ent_type) {
	assert(!findEnt(name, namelen));
	assert(this->ent_type == ET_DIR);
	assert(file_size % sizeof(DirEnt) == 0);
	int pos = file_size;

	resize(file_size + sizeof(DirEnt)); // file_size changed
  // flush the parent DirEnt to disk because of it's size change
  // TODO: call flush inside resize after we add path into in-mem view of DirEnt
	flush(path, pathlen);

	assert(file_size == pos + sizeof(DirEnt));

	DirEnt newent(name, namelen, ent_type);
	write(pos, sizeof(DirEnt), &newent);
	return newent;
}

void DirEnt::resize(int newsize) {
	int oldsize = file_size;
	file_size = newsize;

	assert(newsize > oldsize);

	// may need to allocate more blocks
	int oldLastLogBlk = (oldsize - 1) / BLOCK_SIZE;
	if (oldsize == 0) {
		oldLastLogBlk = -1;
	}
	int newLastLogBlk = (newsize - 1) / BLOCK_SIZE;

	assert(newLastLogBlk < N_DIRECT_BLOCK); // TODO support indirect block

  // it's possible that no new blocks need to be allocated
	for (int lb_idx = oldLastLogBlk + 1; lb_idx <= newLastLogBlk; ++lb_idx) {
		blktable[lb_idx] = SimFs::get().allocPhysBlk();
	}
}

void DirEnt::flush(const char* path, int pathlen) {
  auto wpres = SimFs::get().walkPath(path, pathlen, true);
  if (wpres.pathIsRoot) {
    SimFs::get().updateRootDirEnt(*this);
    return;
  }
  assert(wpres.dirent);
  int child_idx = wpres.dirent.findEntIdx(wpres.lastItemPtr, wpres.lastItemLen);
  wpres.dirent.write(child_idx * sizeof(DirEnt), sizeof(DirEnt), (const char*) this);
}

int DirEnt::write(int pos, int size, const void* buf) {
	assert(pos >= 0);
	assert(size >= 0);
	assert(pos + size - 1 < file_size);
	if (size == 0) {
		return 0;
	}

	char block_cont[BLOCK_SIZE];
	if (pos % BLOCK_SIZE != 0 || size < BLOCK_SIZE) {
		// partial block
		// read first, apply the update, then write back to the disk
		readBlockForOff(pos, block_cont);
		int ncpy = min(size, BLOCK_SIZE - pos % BLOCK_SIZE);
		memmove(block_cont + pos % BLOCK_SIZE, buf, ncpy);
		writeBlockForOff(pos, block_cont);
		return write(pos + ncpy, size - ncpy, buf + ncpy) + ncpy;
	} else {
    #if 0
		// whole block. no need to read the block from the disk first
    writeBlockForOff(pos, (const char*) buf);
    #else
    // NOTE: even if we don't ready need to copy a block from buf to block_cont,
    // we still do that.
    // The reason is buf may be an address from user space, which does not equal
    // to the physical address. Copy the content over to the kernel buffer so
    // we can use the virtual address as physically address directly.
    //
    // A physical address is needed to do DMA for USB drive.
    //
    // TODO: create a function to return physical address given a virtual address
    memmove(block_cont, buf, BLOCK_SIZE);
    writeBlockForOff(pos, block_cont);
    #endif
    return write(pos + BLOCK_SIZE, size - BLOCK_SIZE, buf + BLOCK_SIZE) + BLOCK_SIZE;
	}
}

void DirEnt::readBlockForOff(int off, char *buf) {
	int log_blk_idx = off / BLOCK_SIZE;
	int phys_blk_idx = logicalToPhysBlockId(log_blk_idx);
	SimFs::get().readBlock(phys_blk_idx, (uint8_t*) buf);
}

void DirEnt::writeBlockForOff(int off, const char *buf) {
	int log_blk_idx = off / BLOCK_SIZE;
	int phys_blk_idx = logicalToPhysBlockId(log_blk_idx);
	SimFs::get().writeBlock(phys_blk_idx, (const uint8_t*) buf);
}

SimFs SimFs::instance_;

#define USB_SECTOR_OFF ((0x100000 / SECTOR_SIZE))
int blockIdToUSBSectorNo(int blockId) {
  return blockId * SECTORS_PER_BLOCK + USB_SECTOR_OFF; 
}

int SimFs::blockIdToSectorNo(int blockId) {
  return blockId * SECTORS_PER_BLOCK;
}

/*
 * buf should have enough space to store the entire block even if
 * len < BLOCK_SIZE. Treat len as an hint so readBlock can read less
 * data if possible.
 */
void SimFs::readBlock(int blockId, uint8_t buf[], int len) {
  assert(len > 0 && len <= BLOCK_SIZE);

  // TODO take advantage of len

#if USB_BOOT
  // a usb block is actually a sector
  dev_.readBlocks(blockIdToUSBSectorNo(blockId), SECTORS_PER_BLOCK, buf);
#else
  dev_.read(buf, blockIdToSectorNo(blockId), SECTORS_PER_BLOCK);
#endif
}

void SimFs::writeBlock(int blockId, const uint8_t* buf) {
#if USB_BOOT
  dev_.writeBlocks(blockIdToUSBSectorNo(blockId), SECTORS_PER_BLOCK, buf);
#else
	dev_.write(buf, blockIdToSectorNo(blockId), SECTORS_PER_BLOCK);
#endif
}

void SimFs::init() {
  uint8_t buf[BLOCK_SIZE];
#if USB_BOOT
  extern MassStorageDevice<XHCIDriver> msd_dev;
  dev_ = msd_dev;
#else
  // hardcode to use the slave IDE device for the filesystem for now
  dev_ = createSlaveIDE();
#endif
  readBlock(0, buf, sizeof(SuperBlock));

  superBlock_ = *((SuperBlock*) buf);
  printf("Total number of block in super block %d\n", superBlock_.tot_block);
}

DirEnt SimFs::walkPath(const char* path) {
  return walkPath(path, strlen(path), false).dirent;
}

WalkPathResult SimFs::walkPath(const char* path, int pathlen, bool returnParent) {
  assert(pathlen >= 0);
  const char* end = path + pathlen;
  const char* cur = path;
  WalkPathResult wpres;
  while (cur != end && *cur == '/') {
    ++cur;
  }
  if (cur == end) {
    wpres.pathIsRoot = true;
    if (!returnParent) {
      wpres.dirent = superBlock_.rootdir;
    }
    return wpres;
  }
  const char* curend = cur;
  while (curend != end && *curend != '/') {
    ++curend;
  }
  assert(curend - cur > 0);

  DirEnt parentDirEnt = superBlock_.rootdir;
  // loop invariant:
  // - [cur, curend) represents the current path item
  // - parentDirEnt is for the parent direcotry of it.
  // exit the loop if there is no more path items
  const char* next;
  while (true) {
    next = curend;
    while (next != end && *next == '/') {
      ++next;
    }
    if (next == end) { // no more path item
      break;
    }
    // walk the path one level
    parentDirEnt = parentDirEnt.findEnt(cur, curend - cur);
    if (!parentDirEnt) {
      assert(!wpres.dirent);
      return wpres;
    }
    cur = next;
    curend = cur;
    while (curend != end && *curend != '/') {
      ++curend;
    }
  }

  if (returnParent) {
    wpres.dirent = parentDirEnt;
    wpres.lastItemPtr = cur;
    wpres.lastItemLen = curend - cur;
  } else {
    wpres.dirent = parentDirEnt.findEnt(cur, curend - cur); 
  }
  return wpres;
}

DirEnt SimFs::createFile(const char* path) {
  auto wpres = walkPath(path, strlen(path), true);
  if (!wpres.dirent) { // fail to walk to the parent directory
    return wpres.dirent;
  }
  /*
   * TODO: we need pass the path fragment for parent to createFile since so we
   * can use it to keep the DirEnt on disk up to date.
   *
   * It's much better and straightforward if we record the path for an file/dir in
   * it's DirEnt. The problem here is:
   * 1. we don't want to store the path on disk since that's a waste of space
   * 2. we need store the path in DirEnt when it's loaded to memory
   *
   * That being said, we would need 2 views of DirEnt:
   * - an on-disk view WITHOUT the path field
   * - an in-memory view WITH the path field
   */
  return wpres.dirent.createFile(path, wpres.lastItemPtr - path, wpres.lastItemPtr, wpres.lastItemLen);
}

uint32_t SimFs::allocPhysBlk() {
	assert(superBlock_.freelist != 0 && "Out of disk space");
	int ret = superBlock_.freelist;
	char buf[BLOCK_SIZE];

	// TODO: only need the first 4 bytes
	readBlock(ret, (uint8_t*) buf);
	superBlock_.freelist = *(uint32_t*) buf;
	flushSuperBlock(); // TODO: do this lazily?
	return ret;
}

void SimFs::updateRootDirEnt(const DirEnt& newent) {
	superBlock_.rootdir = newent;
	flushSuperBlock();
}

void SimFs::flushSuperBlock() {
  char buf[BLOCK_SIZE];
	// TODO: we can avoid writing the whole block
	memmove(buf, (void*) &superBlock_, sizeof(SuperBlock));
	writeBlock(0, (const uint8_t*) buf);
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
