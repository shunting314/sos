#pragma once

#include <kernel/ioport.h>
#include <stdint.h>

enum IDECommand {
  READ_SECTORS = 0x20,
  WRITE_SECTORS = 0x30,
  FLUSH_WRITE_CACHE = 0xE7,
};

enum StatusBit {
  BSY = 7, // busy
};

#define SECTOR_SIZE 512

class IDEDevice {
 public:
  IDEDevice(uint16_t ioPortBase = 0, bool isSlave = false)
      : ioPortBase_(ioPortBase),
        isSlave_(isSlave),
        dataPort_(ioPortBase),
        errorRegPort_(ioPortBase + 1),
        sectCountPort_(ioPortBase + 2),
        lbaLowPort_(ioPortBase + 3),
        lbaMidPort_(ioPortBase + 4),
        lbaHighPort_(ioPortBase + 5),
        driveRegPort_(ioPortBase + 6),
        statusCommandPort_(ioPortBase + 7) {
  }

  // port base 0 means an invalid IDEDevice
  operator bool() const {
    return ioPortBase_ != 0;
  }

  void read(uint8_t* buf, int startSectorNo, int nSector);
  void write(const uint8_t* buf, int startSectorNo, int nSector);
  void flushWriteCache();
 private:
  void setLBA(int lba);
  void waitUntilNotBusy();

  Port16Bit dataPort_;
  Port8Bit errorRegPort_;
  Port8Bit sectCountPort_;
  Port8Bit lbaLowPort_;
  Port8Bit lbaMidPort_;
  Port8Bit lbaHighPort_;
  Port8Bit driveRegPort_;
  Port8Bit statusCommandPort_;

  uint16_t ioPortBase_;
  bool isSlave_;
};

static inline IDEDevice createMasterIDE() {
  return IDEDevice(0x1F0, false);
}

static inline IDEDevice createSlaveIDE() {
  return IDEDevice(0x1F0, true);
}
