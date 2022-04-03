#include <kernel/ide.h>

void IDEDevice::waitUntilNotBusy() {
  while (true) {
    uint8_t status = statusCommandPort_.read();
    if (!((status >> BSY) & 0x1)) {
      break;
    }
  }
}

void IDEDevice::setLBA(int lba) {
  lbaLowPort_.write(lba & 0xFF);
  lbaMidPort_.write((lba >> 8) & 0xFF);
  lbaHighPort_.write((lba >> 16) & 0xFF);
  driveRegPort_.write((0xE0 | (isSlave_ << 4)) | ((lba >> 24) & 0xF));
}

void IDEDevice::read(uint8_t* buf, int startSectorNo, int nSector) {
  waitUntilNotBusy();
  sectCountPort_.write(nSector);
  setLBA(startSectorNo);
  statusCommandPort_.write(READ_SECTORS);
  waitUntilNotBusy();

  // the device interprets sector count 0 as 256
  if (nSector == 0) {
    nSector = 256;
  }
  for (int i = 0; i < nSector * SECTOR_SIZE; i += 2) {
    *((uint16_t*) &buf[i]) = dataPort_.read();
  }
}

void IDEDevice::flushWriteCache() {
  statusCommandPort_.write(FLUSH_WRITE_CACHE);
  waitUntilNotBusy();
}

void IDEDevice::write(const uint8_t* buf, int startSectorNo, int nSector) {
  waitUntilNotBusy();
  sectCountPort_.write(nSector);
  setLBA(startSectorNo);
  statusCommandPort_.write(WRITE_SECTORS);

  // the device interpreter sector count 0 as 256
  if (nSector == 0) {
    nSector = 256;
  }
  for (int i = 0; i < nSector * SECTOR_SIZE; i += 2) {
    // need some delay for each outw instruction
    waitUntilNotBusy();
    dataPort_.write(*((uint16_t*) &buf[i]));
  }
  // needed according to wiki 'ATA PIO Mode - osdev'
  // Here is copied from that wiki:
  // > On some drives it is necessary to "manually" flush the hardware
  // > write cache after every write command. This is done by sending
  // > the 0xE7 command to the Command Register (then waiting for BSY to clear).
  // > If a driver does not do this, then subsequent write commands can fail
  // > invisibly, or "temporary bad sectors" can be created on your disk.
  flushWriteCache();
}
