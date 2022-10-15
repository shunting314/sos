// header for mass storage device
#pragma once

#include <assert.h>

#define CBW_SIGNATURE 0x43425355 // 'USBC' in little endian
#define CSW_SIGNATURE 0x53425355 // 'USBS' in little endian

struct CommandBlockWrapper {
  // TODO: assume LUN (logical unit number) to be 0 for now. We should pass LUN
  // in.
  explicit CommandBlockWrapper(
    uint32_t _tag,
    uint32_t _transfer_length,
    bool in_request,
    uint8_t _cmd_len,
    uint8_t* cmdptr) :
      tag(_tag),
      transfer_length(_transfer_length),
      flags(in_request ? 0x80 : 0),
      cmd_len(_cmd_len) {
    for (int i = 0; i < cmd_len; ++i) {
      cmd[i] = cmdptr[i];
    }
    for (int i = cmd_len; i < 16; ++i) {
      cmd[i] = 0;
    }
  }

  uint32_t signature = CBW_SIGNATURE;
  uint32_t tag;
  // length of data in bytes. Not count CBW/CSW.
  uint32_t transfer_length;
  // bit 7: 0 = out, 1 = in
  uint8_t flags;
  uint8_t lun = 0;
  uint8_t cmd_len; // valid range [1, 16]
  // the command data block (CDB). When cmd_len < 16, the device still
  // receives the whole 16 bytes of the cmd array. The bytes beyond
  // cmd_len will be ignored by the device.
  uint8_t cmd[16];
} __attribute__((packed));

static_assert(sizeof(CommandBlockWrapper) == 31);

struct CommandStatusWrapper {
  void print() const {
    printf("CSW: SIG 0x%x, tag 0x%x, data_residue %d, status %d\n", signature, tag, data_residue, status);
  }

  uint32_t signature = CSW_SIGNATURE;
  uint32_t tag;
  uint32_t data_residue;
  uint8_t status; // 0 for success
} __attribute__((packed));

static_assert(sizeof(CommandStatusWrapper) == 13);
