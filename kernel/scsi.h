#pragma once

#include <assert.h>
#include <stdint.h>

// reuse the byteorder.h for network protocols
#include <kernel/net/byteorder.h>

namespace scsi {

enum OperationCode {
  READ_CAPACITY_10 = 0x25,
  READ_10 = 0x28,
  WRITE_10 = 0x2A,
};

// command descriptor block
// multi-byte fields are in big-endian order
class CDB {
 public:
  explicit CDB(uint8_t operation_code) : operation_code_(operation_code) { }
 protected:
  uint8_t operation_code_;
};

class ReadCapacity10Response {
 public:
  uint32_t returned_logical_block_address() const {
    return ntoh(returned_logical_block_address_);
  }
  uint32_t block_length_in_bytes() const {
    return ntoh(block_length_in_bytes_);
  }

  void print() const {
    printf("ReadCapacity10 returns:\n"
      "  returned_logical_block_address = %d, block_length_in_bytes = %d\n",
      returned_logical_block_address(), block_length_in_bytes());
  }
 private:
  // LBA of the last available block. In bit endian.
  uint32_t returned_logical_block_address_;
  uint32_t block_length_in_bytes_;
};
static_assert(sizeof(ReadCapacity10Response) == 8);

class ReadCapacity10 : public CDB {
 public:
  explicit ReadCapacity10() : CDB(READ_CAPACITY_10) { }
 private:
  uint8_t dummy[9] = {0};
} __attribute__((packed));
static_assert(sizeof(ReadCapacity10) == 10);

class Read10 : public CDB {
 public:
  explicit Read10(uint32_t lba, uint16_t transfer_length) : CDB(READ_10) {
    lba_ = hton(lba);
    transfer_length_ = hton(transfer_length);
  }

 private:
  uint8_t reserved_ = 0;
  uint32_t lba_;
  uint8_t reserved_2_ = 0;
  uint16_t transfer_length_; // number of blocks
  uint8_t control_ = 0;
} __attribute__((packed));
static_assert(sizeof(Read10) == 10);

class Write10 : public CDB {
 public:
  explicit Write10(uint32_t lba, uint16_t transfer_length) : CDB(WRITE_10) {
    lba_ = hton(lba);
    transfer_length_ = hton(transfer_length);
  }
 private:
  uint8_t reserved_ = 0;
  uint32_t lba_;
  uint8_t reserved_2_ = 0;
  uint16_t transfer_length_; // number of blocks
  uint8_t control_ = 0;
} __attribute__((packed));
static_assert(sizeof(Write10) == 10);

}
