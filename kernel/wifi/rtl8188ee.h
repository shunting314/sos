#pragma once

/*
 * Definitions specific to rtl8188ee.
 */
#define BIT(i) (1 << (i))

#define REG_SYS_FUNC_EN 0x0002
#define REG_APS_FSMCO 0x0004

#define REG_SYS_CLKR 0x0008
#define REG_9346CR 0x000A

#define REG_RSV_CTRL 0x001C
#define REG_RF_CTRL 0x001F
#define REG_EFUSE_CTRL 0x0030

#define REG_GPIO_MUXCFG 0x0040
#define REG_LEDCFG0 0x004C

#define REG_HSIMR 0x0058

#define REG_AFE_XTAL_CTRL_EXT 0x0078

#define REG_XCK_OUT_CTRL 0x007c
#define REG_MCUFWDL 0x0080
#define REG_HIMR 0x00B0

#define REG_HISR 0x00B4

#define REG_HIMRE 0x00B8

#define REG_HISRE 0x00BC

#define REG_EFUSE_ACCESS 0x00CF

#define REG_CR 0x0100
#define REG_PBP 0x0104

#define REG_TRXDMA_CTRL 0x010C
#define REG_TRXFF_BNDY 0x0114

#define REG_C2HEVT_CLEAR 0x01AF

#define REG_MCUTST_1 0x01c0
#define REG_LLT_INIT 0x01E0

#define REG_RQPN 0x0200
#define REG_TDECTRL 0x0208
#define REG_RQPN_NPQ 0x0214
#define REG_PCIE_CTRL_REG 0x0300

#define REG_INT_MIG 0x0304

#define REG_WATCH_DOG 0x0368

#define REG_HWSEQ_CTRL 0x0423
#define REG_RRSR 0x0440
#define REG_TX_RPT_CTRL 0x04EC
#define REG_TX_RPT_TIME 0x04F0

#define REG_TCR 0x0604
#define REG_RCR 0x0608
#define REG_RX_DRVINFO_SZ 0x060F
#define REG_NAV_CTRL 0x0650
#define REG_SECCFG 0x0680
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

#define REG_TXPKTBUF_BCNQ_BDNY 0x0424
#define REG_TXPKTBUF_MGQ_BDNY 0x0425
#define REG_TXPAUSE 0x0522

#define REG_CAMCMD 0x0670

// mac address
#define REG_MACID 0x0610

#define RFPGA0_XA_HSSIPARAMETER1 0x820
#define RFPGA0_XA_HSSIPARAMETER2 0x824
#define RFPGA0_XA_LSSIPARAMETER 0x840
#define RFPGA0_XA_RFINTERFACEOE 0x860
#define RFPGA0_XB_RFINTERFACEOE 0x864
#define RFPGA0_XAB_RFINTERFACESW 0x870
#define RFPGA0_XCD_RFINTERFACESW 0x874
#define RFPGA0_XAB_RFPARAMETER 0x878
#define RFPGA0_XA_LSSIREADBACK 0x8a0

#define TRANSCEIVEA_HSPI_READBACK 0x8b8

#define RCONFIG_RAM64x16 0xb2c

#define ROFDM0_TRXPATHENABLE 0xc04
#define ROFDM0_TRMUXPAR 0xc08
#define ROFDM0_XBRXIQIMBALANCE 0xc1c
#define ROFDM0_AGCRSSITABLE 0xc78
#define ROFDM0_XBTXIQIMBALANCE 0xc88
#define ROFDM0_XDTXAFE 0xc9c
#define ROFDM0_RXIQEXTANTA 0xca0

#define RFPGA0_IQK 0xe28

#define RTX_IQK_TONE_A 0xe30
#define RRX_IQK_TONE_A 0xe34
#define RTX_IQK_PI_A 0xe38
#define RRX_IQK_PI_A 0xe3c
#define RTX_IQK 0xe40
#define RRX_IQK 0xe44
#define RIQK_AGC_PTS 0xe48
#define RIQK_AGC_RSP 0xe4c

#define RTX_POWER_BEFORE_IQK_A 0xe94
#define RTX_POWER_AFTER_IQK_A 0xe9c

#define RRX_POWER_BEFORE_IQK_A_2 0xea4
#define RRX_POWER_AFTER_IQK_A_2 0xeac

#define ROFDM0_XARXIQIMBALANCE 0xc14
#define ROFDM0_ECCATHRESHOLD 0xc4c
#define ROFDM0_XATXIQIMBALANCE 0xc80
#define ROFDM0_XCTXAFE 0xc94

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

#define EEPROM_RF_ANTENNA_OPT_88E 0xC9
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

#define _LLT_NO_ACTIVE 0x0
#define _LLT_WRITE_ACCESS 0x1
#define _LLT_READ_ACCESS 0x2

#define _LLT_INIT_DATA(x) ((x) & 0xFF)
#define _LLT_INIT_ADDR(x) (((x) & 0xFF) << 8)
#define _LLT_OP(x) (((x) & 0x3) << 30)
#define _LLT_OP_VALUE(x) (((x) >> 30) & 0x3)

#define POLLING_LLT_THRESHOLD 20

#define MCUFWDL_RDY (1 << 1)
#define FWDL_CHKSUM_RPT (1 << 2)
#define WINTINI_RDY (1 << 6)

#define RF_EN BIT(0)
#define RF_RSTB BIT(1)
#define RF_SDMRSTB BIT(2)

#define FEN_BBRSTB BIT(0)
#define FEN_BB_GLB_RSTN BIT(1)
#define FEN_DIO_PCIE BIT(5)
#define FEN_PCIEA BIT(6)
#define FEN_PPLL BIT(7)

#define RFPGA0_RFMOD 0x800
#define BCCKEN 0x1000000
#define BOFDMEN 0x2000000

#define BRFSI_RFENV 0x10

#define B3WIREDATALENGTH 0x800
#define B3WIREADDREAALENGTH 0x400

#define RATR_1M 0x1
#define RATR_2M 0x2
#define RATR_55M 0x4
#define RATR_11M 0x8
#define RATR_6M 0x10
#define RATR_9M 0x20
#define RATR_12M 0x40
#define RATR_18M 0x80
#define RATR_24M 0x100
#define RATR_36M 0x200
#define RATR_48M 0x400
#define RATR_54M 0x800

#define RATE_ALL_CCK (RATR_1M | RATR_2M | RATR_55M | RATR_11M)

#define RATE_ALL_OFDM_AG (RATR_6M | RATR_9M | RATR_12M | RATR_18M | \
        RATR_24M | RATR_36M | RATR_48M | RATR_54M) 

#define SCR_TXENCENABLE BIT(2)
#define SCR_RXDECENABLE BIT(3)
#define SCR_TXBCUSEDK BIT(6)
#define SCR_RXBCUSEDK BIT(7)

#define RF_TXPA_G1 0x31
#define RF_TXPA_G2 0x32
#define RF_RCK_OS 0x30
#define RF_WE_LUT 0xEF

#define BLSSIREADADDRESS 0x7f800000
#define BLSSIREADEDGE 0x80000000
#define BLSSIREADBACKDATA 0xfffff
