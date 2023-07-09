#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAGIC_NUMBER 0x01030507

/*
 * Metadata for FreeBlock
 * - size in bytes. Excluding the metadata itself for convenience when allocating memory.
 * - prev/next link
 *
 * Maintain FreeBlock as a circular list. Maintain a sentinel at the head to make sure there is at least one FreeBlock.
 *
 * The FreeBlock circular list only contains information about free blocks. Allocated block does not
 * show up.
 *
 * The FreeBlock list sorted by address to ease merging.
 *
 * After a FreeBlock is allocated, we store a MATIC_NUMBER in prev_ pointer for debugging.
 * (we could store one more magic number in next_ if we want)
 */
struct FreeBlock {
 public:
  void* data() {
    return (void*) this + sizeof(*this);
  }

  void *data_end() {
    return data() + free_bytes_;
  }

  void check_magic_number() {
    uint32_t magic_number = (uint32_t) prev_;
    assert(magic_number == MAGIC_NUMBER);
  }

  void set_magic_number() {
    prev_ = (FreeBlock*) MAGIC_NUMBER;
  }

  void remove_from_list() {
    FreeBlock* lhs = prev_;
    FreeBlock* rhs = next_;
    lhs->next_ = rhs;
    rhs->prev_ = lhs;
  }
 public:
  uint32_t free_bytes_;
  FreeBlock* prev_, *next_;
};

#define META_SIZE sizeof(FreeBlock)
// each free block should have at least this amount of bytes except the sentinel FreeBlock which contains 0 bytes
#define MIN_BLOCK_FREE_BYTES 8 

// align the allocated memory
#define MIN_ALIGN 4

FreeBlock* free_block_head = nullptr;

void setup_malloc(void *start, uint32_t size) {
  printf("setup malloc start %p size 0x%x\n", start, size);
  assert(size > 2 * META_SIZE + MIN_BLOCK_FREE_BYTES);

  free_block_head = (FreeBlock*) start;
  FreeBlock* datablock = free_block_head + 1;

  free_block_head->free_bytes_ = 0;
  free_block_head->next_ = free_block_head->prev_ = datablock;

  datablock->free_bytes_ = size - 2 * META_SIZE;
  datablock->next_ = datablock->prev_ = free_block_head;
}

void* malloc(uint32_t nbytes) {
  if (nbytes == 0) {
    return nullptr;
  }

  assert(free_block_head);

  nbytes = ROUND_UP(nbytes, MIN_ALIGN);

  FreeBlock* found = nullptr;
  for (FreeBlock* cur = free_block_head->next_; cur != free_block_head; cur = cur->next_) {
    if (cur->free_bytes_ >= nbytes) {
      found = cur;
      break;
    }
  }
  assert(found && "Out of heap memory");

  // allocate nbytes from current_head
  if (found->free_bytes_ >= nbytes + META_SIZE + MIN_BLOCK_FREE_BYTES) {
    // easy case, split
    // we split the allocated memory from the end for the linked list does not
    // need to be changed.
    FreeBlock* allocated = (FreeBlock*) (found->data_end() - nbytes - META_SIZE);
    found->free_bytes_ -= nbytes + META_SIZE;

    // this field does not actually means free bytes but allocated bytes
    // in this context.
    allocated->free_bytes_ = nbytes;
    allocated->prev_ = (FreeBlock*) MAGIC_NUMBER;
    return allocated->data();
  }

  // the current block is exausted. Delete it from FreeBlock list and return it.
  found->remove_from_list();
  found->set_magic_number();
  return found->data();
}

void* realloc(void* orig_ptr, uint32_t new_size) {
  if (!orig_ptr) {
    return malloc(new_size);
  }
  new_size = ROUND_UP(new_size, MIN_ALIGN);
  FreeBlock* orig_block = (FreeBlock*) orig_ptr - 1;
  if (new_size <= orig_block->free_bytes_) {
    return orig_ptr; // don't do shrinking for now
  }

  void* new_ptr = malloc(new_size);
  memmove(new_ptr, orig_ptr, orig_block->free_bytes_);
  free(orig_ptr);
  return new_ptr;
}

bool merge_block(FreeBlock* lhs, FreeBlock* rhs) {
  assert(lhs->next_ == rhs);
  assert(rhs->prev_ == lhs);
  if (lhs == free_block_head || rhs == free_block_head) {
    return false;
  }
  assert(lhs < rhs);
  if (lhs->data_end() != (void*) rhs) {
    return false;
  }
  lhs->free_bytes_ += rhs->free_bytes_ + META_SIZE;
  lhs->next_ = rhs->next_;
  rhs->next_->prev_ = lhs;
  return true;
}

/*
 * TODO: is it a good heuristic to do the next allocation from the previous freed memory?
 */
void free(void* ptr) {
  FreeBlock* free_block = (FreeBlock*) ptr - 1;
  free_block->check_magic_number();

  FreeBlock* left_sib = nullptr;
  for (left_sib = free_block_head; left_sib->next_ != free_block_head && left_sib->next_ < free_block; left_sib = left_sib->next_) {
  }
  assert(left_sib == free_block_head || left_sib < free_block);
  FreeBlock* right_sib = left_sib->next_;
  assert(right_sib == free_block_head || right_sib > free_block);

  // to ease the operation below, insert the free_block first
  left_sib->next_ = free_block;
  free_block->prev_ = left_sib;
  free_block->next_ = right_sib;
  right_sib->prev_ = free_block;

  // the order the these 2 calls matters
  merge_block(free_block, right_sib);

  // no matter if the previous merge succeed or not, we can always try to merge
  // left_sib and free_block
  merge_block(left_sib, free_block);
}
