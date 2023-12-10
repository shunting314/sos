#pragma once

/*
 * Definitions specific to rtl8188ee.
 */
#define REG_SYS_FUNC_EN 0x0002
#define REG_SYS_CLKR 0x0008
#define REG_9346CR 0x000A
#define REG_EFUSE_CTRL 0x0030
#define REG_HSIMR 0x0058
#define REG_HIMR 0x00B0
#define REG_HIMRE 0x00B8
#define REG_EFUSE_ACCESS 0x00CF
#define REG_C2HEVT_CLEAR 0x01AF
#define REG_PCIE_CTRL_REG 0x0300

#define FEN_ELDR (1 << 12)

#define ANA8M (1 << 1)
#define LOADER_CLK_EN (1 << 5)


// RX Ring dma address
#define REG_RX_DESA 0x0340

// TX Ring dma address
#define REG_BCNQ_DESA 0x0308
#define REG_MGQ_DESA 0x0318
#define REG_VOQ_DESA 0x0320
#define REG_VIQ_DESA 0x0328
#define REG_BEQ_DESA 0x0330
#define REG_BKQ_DESA 0x0338
#define REG_HQ_DESA 0x0310

// mac address
#define REG_MACID 0x0610

// IMR DW0
// Power Save Time Out Interrupt
#define IMR_PSTIMEOUT (1 << 29)
// HSISR Indicator (HSIMR & HSISR is true, this bit is set to 1) */
#define IMR_HSISR_IND_ON_INT (1 << 15)
// CPU to Host Command INT Status, Write 1 clear */
#define IMR_C2HCMD (1 << 10)
// High Queue DMA OK
#define IMR_HIGHDOK (1 << 7)
// Management Queue DMA OK
#define IMR_MGNTDOK (1 << 6)
// AC_BK DMA OK
#define IMR_BKDOK (1 << 5)
// AC_BE DMA OK
#define IMR_BEDOK (1 << 4)
// AC_VI DMA OK
#define IMR_VIDOK (1 << 3)
// AC_VO DMA OK
#define IMR_VODOK (1 << 2)
// Rx Descriptor Unavailable
#define IMR_RDU (1 << 1)
// Receive DMA OK
#define IMR_ROK (1 << 0)

// IMR DW1
#define IMR_RXFOVW (1 << 8)

// Host System Interrupt Mask Register
#define HSIMR_RON_INT_EN (1 << 6)
#define HSIMR_PDN_INT_EN (1 << 7)

#define EEPROM_MAC_ADDR 0xD0
