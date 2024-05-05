// header for mass storage device
#pragma once

#include <assert.h>
#include <stdlib.h>
#include <kernel/usb/usb_device.h>

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

  bool success() const {
    return status == 0;
  }
  uint32_t signature = CSW_SIGNATURE;
  uint32_t tag;
  uint32_t data_residue;
  uint8_t status; // 0 for success
} __attribute__((packed));

static_assert(sizeof(CommandStatusWrapper) == 13);

// #define FIXED_TAG 0x06180618
#ifdef FIXED_TAG
#define GENERATE_TAG FIXED_TAG
#else
#define GENERATE_TAG rand()
#endif

// TODO: instead of using template, use a controller driver base class
template <typename ControllerDriver>
class MassStorageDevice : public USBDevice<ControllerDriver> {
 public:
  explicit MassStorageDevice(ControllerDriver* driver = nullptr) : USBDevice<ControllerDriver>(driver) { }

  void readCapacity() {
    scsi::ReadCapacity10 cmd;
    scsi::ReadCapacity10Response resp;
    handleInCommand((uint8_t*) &cmd, sizeof(cmd), (uint8_t*) &resp, sizeof(resp));
    blockSize_ = resp.block_length_in_bytes();
    totalNumBlocks_ = resp.returned_logical_block_address() + 1;
    printf("block size %d, total number of blocks %d\n", blockSize_, totalNumBlocks_);
  }

  // assumes buf has enough capacity to store nblock's of data
  void readBlocks(uint32_t lba_start, uint32_t nblock, uint8_t* buf) {
    assert(lba_start >= 0);
    assert(nblock > 0);
    if (lba_start + nblock - 1 >= totalNumBlocks_) {
      printf("readBlocks lba_start %d, nblock %d, totalNumBlocks_ %d\n", lba_start, nblock, totalNumBlocks_);
      assert(false && "readBlocks out of range");
    }

    scsi::Read10 cmd(lba_start, nblock);
    handleInCommand((uint8_t*) &cmd, sizeof(cmd), buf, nblock * blockSize_);
  }

  // assumes buf has enough capacity to store nblock's of data
  void writeBlocks(uint32_t lba_start, uint32_t nblock, const uint8_t* buf) {
    assert(lba_start >= 0);
    assert(nblock > 0);
    assert(lba_start + nblock - 1 < totalNumBlocks_);
    scsi::Write10 cmd(lba_start, nblock);
    handleOutCommand((uint8_t*) &cmd, sizeof(cmd), buf, nblock * blockSize_);
  }

  /*
   * Handle a SCSI command that read content from the device.
   */
  void handleInCommand(uint8_t* cmdptr, int cmdlen, uint8_t* payloadPtr, int payloadLen) {
    CommandBlockWrapper cbw(
      GENERATE_TAG,
      payloadLen,
      true, // in request
      cmdlen,
      cmdptr);
    bulkSend((uint8_t*) &cbw, sizeof(cbw));
    bulkRecv(payloadPtr, payloadLen);

    CommandStatusWrapper csw;
    bulkRecv((uint8_t*) &csw, sizeof(csw));
    assert(csw.signature == CSW_SIGNATURE);
    assert(csw.tag == cbw.tag);
    assert(csw.success());
  }

  /*
   * Handle a SCSI command that send content to the device.
   */
  void handleOutCommand(uint8_t* cmdptr, int cmdlen, const uint8_t* payloadPtr, int payloadLen) {
    CommandBlockWrapper cbw(
      GENERATE_TAG,
      payloadLen,
      false, // out request
      cmdlen,
      cmdptr);
    bulkSend((uint8_t*) &cbw, sizeof(cbw));
    bulkSend(payloadPtr, payloadLen);

    CommandStatusWrapper csw;
    bulkRecv((uint8_t*) &csw, sizeof(csw));
    assert(csw.signature == CSW_SIGNATURE);
    assert(csw.tag == cbw.tag);
    assert(csw.success());
  }

  void bulkSend(const uint8_t *buf, int bufsize) {
    this->controller_driver_->bulkSend(this, this->bulkOut_, this->bulkOutDataToggle_, buf, bufsize);
  }

  void bulkRecv(uint8_t *buf, int bufsize) {
    this->controller_driver_->bulkRecv(this, this->bulkIn_, this->bulkInDataToggle_, buf, bufsize);
  }
 public:
  uint32_t blockSize() const {
    return blockSize_;
  }
 private:
  uint32_t blockSize_;
  uint32_t totalNumBlocks_;
};
