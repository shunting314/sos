#pragma once

/*
 * Definitions specific to rtl8188ee.
 */
#define REG_SYS_FUNC_EN 0x0002
#define REG_APS_FSMCO 0x0004

#define REG_SYS_CLKR 0x0008
#define REG_9346CR 0x000A

#define REG_RSV_CTRL 0x001C

#define REG_EFUSE_CTRL 0x0030

#define REG_GPIO_MUXCFG 0x0040

#define REG_HSIMR 0x0058

#define REG_AFE_XTAL_CTRL_EXT 0x0078

#define REG_XCK_OUT_CTRL 0x007c
#define REG_HIMR 0x00B0

#define REG_HISR 0x00B4

#define REG_HIMRE 0x00B8

#define REG_HISRE 0x00BC

#define REG_EFUSE_ACCESS 0x00CF

#define REG_CR 0x0100

#define REG_TRXDMA_CTRL 0x010C

#define REG_C2HEVT_CLEAR 0x01AF

#define REG_MCUTST_1 0x01c0

#define REG_PCIE_CTRL_REG 0x0300

#define REG_INT_MIG 0x0304

#define REG_WATCH_DOG 0x0368

#define REG_TX_RPT_CTRL 0x04EC
#define REG_TX_RPT_TIME 0x04F0

#define REG_TCR 0x0604
#define REG_RCR 0x0608
#define REG_RXFLTMAP2 0x06A4

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

#define RCR_APPFCS (1 << 31)
#define RCR_APP_MIC (1 << 30)
#define RCR_APP_ICV (1 << 29)
#define RCR_APP_PHYST_RXFF (1 << 28)
#define RCR_HTC_LOC_CTRL (1 << 14)
#define RCR_AMF (1 << 13)
#define RCR_ACF (1 << 12)
#define RCR_ADF (1 << 11)
#define RCR_AICV (1 << 9)
#define RCR_ACRC32 (1 << 8)
#define RCR_AB (1 << 3)
#define RCR_AM (1 << 2)
#define RCR_APM (1 << 1)

#define CFENDFORM (1 << 9)

#define MSR (REG_CR + 2)
