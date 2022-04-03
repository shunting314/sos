<style>
table {
  border-collapse: collapse;
}

table, th, td {
  border: 1px solid;
}
</style>

IDE/ATA device is deprecated by SATA (serial ATA). But since we use the IDE device simulated by QEMU as the disk, we'll investigate how to program an IDE device.

A system can have 0 to 4 ATA buses. The first and second bus are called primary ATA bus and secondary ATA bus respectively. Each bus can have a master IDE device and a slave IDE device attached to it. The name of 'master'/'slave' is a bit confusing since the master device and slave device operate almost identically.

Each bus has a range of IO ports and a range of control ports. The span of the IO ports range and control ports range are 8 bytes and 2 bytes respectively. The default base address for IO ports and control ports for the primary/secondary buses are illustrated as the table below.

|Bus                |IO ports range   |Control ports range |
| ----------------- | --------------- | ------------------ |
|Primary ATA Bus    | 0x1F0 - 0x1F7   | 0x3F6 - 0x3F7      |
|Secondary ATA Bus  | 0x170 - 0x177   | 0x376 - 0x377      |

Each port are associated with device internal registers. Some ports may access differnt registers depending on if it's read or written. While most ports are 8bit port, the data port is an 16 bit port. That means we read/write 2 bytes at one time. The table below describes each port.

(The table below assumes LBA28 mode. TODO: describe LBA28 v.s. LBA48)

| Offset from I/O Base | Direction | Function         | Description                | Param Size |
| -------------------- | --------- | ---------------- | -------------------------- | ---------- |
| 0                    | R/W       | Data Register    | R/W data                   | 16-bit     |
| 1                    | R         | Error register   | Can read error code for the last ATA command executed | 8-bit |
| 2                    | R/W       | Sector count Reg | Number of sectors to R/W (0 means 256)   | 8-bit      |
| 3                    | R/W       | LBA low register | low byte of LBA            | 8-bit      |
| 4                    | R/W       | LBA mid register | mid byte of LBA            | 8-bit      |
| 5                    | R/W       | LBA high register| high byte of LBA           | 8-bit      |
| 6                    | R/W       | Drive Register   | Detail below               | 8-bit      |
| 7                    | R         | Status Register  | Read status of the device  | 8-bit      |
| 7                    | W         | Command Register | Send command to ATA device | 8-bit      |

Drive Register

| Bit | Desc |
| --- | ---  |
| 0-3 | Bits 24-27 of LBA |
| 4   | 0 for master device and 1 for slave device |
| 5   | always set |
| 6   | Set for LBA addressing; Clear for CHS addressing |
| 7   | always set |

The table below describes useful commands.

| Value | Description   |
| ---   | ---           |
| 0x20  | READ SECTORS  |
| 0x30  | WRITE SECTORS |
| 0xE7  | Flush Write Cache |

Refer to 'ATA PIO Mode - osdev' for more details.

# References
- [PATA/IDE - Wikipedia](https://en.wikipedia.org/wiki/Parallel_ATA#IDE_and_ATA-1)
- [PIO - Wikipedia](https://en.wikipedia.org/wiki/Programmed_input%E2%80%93output). The counterpart of PIO is DMA.
- [ATA PIO Mode - osdev](https://wiki.osdev.org/ATA_PIO_Mode)
