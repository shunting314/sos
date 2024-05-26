# Wireshark
- [wireshark - wikipedia](https://en.wikipedia.org/wiki/Wireshark)
  - wireshark is very similar to tcpdump but has a GUI and integrates sorting and filtering options.
  - wireshark has a terminal version called tshark

Here are a few things about inspecting 802.11 management frames with wireshark on MacOS
1. in wireshark GUI 'capture options', the 'monitor' checkbox for the inspected interface should be checked.
2. run airport to switch the interested interface to monitor mode. MacOS already has a tool called airport installed at /System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport . Run `sudo airport <interface-name> sniff 1` to switch the interface to monitored mode. Refer to [how to put MacOS X wireless adapter in monitor mode](https://unix.stackexchange.com/questions/48671/how-to-put-mac-os-x-wireless-adapter-in-monitor-mode) in more details.

# Reference
- [WiFi - Wikipedia](https://en.wikipedia.org/wiki/Wi-Fi)
- [skbuff in linux - clemson university](https://people.computing.clemson.edu/~westall/853/notes/skbuff.pdf)
- [Linux Kernel code cross reference](https://elixir.bootlin.com/linux/latest/source)
- [802.11 Frame Types](https://en.wikipedia.org/wiki/802.11_Frame_Types): mainly describe the frame control word in an 802.11 frame.
- [Frame check sequence - wikipedia](https://en.wikipedia.org/wiki/Frame_check_sequence): it's used in both ethernet frame and 802.11 frame
- [Beacon Frame - wikipedia](https://en.wikipedia.org/wiki/Beacon_frame)
- [IEEE 802 - wikipedia](https://en.wikipedia.org/wiki/IEEE_802)
- [IEEE 802.11 - wikipedia](https://en.wikipedia.org/wiki/IEEE_802.11)
- [How Does A Computer Detect Wifi? - superuser](https://superuser.com/questions/1470252/how-does-a-computer-detect-wifi): This doc refers to an old version of the ieee 802.11 spec!
- [802.11 Spec](https://ieeexplore.ieee.org/document/9363693): Need create an IEEE account to download for free. This is a 4000+ pages large doc!
- [802.11 Wireless Networks: the definitive guide](https://paginas.fe.up.pt/~ee05005/tese/arquivos/ieee80211.pdf)
- [Linux firmware - gentoo wiki](https://wiki.gentoo.org/wiki/Linux_firmware)
- [About firmware - lfs](https://www.linuxfromscratch.org/blfs/view/svn/postlfs/firmware.html): This doc mentions various firmware file can be found at: https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain . I found the firmware file for rtl8188ee (https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain/rtlwifi/rtl8188efw.bin).
- [How to learn mac80211 - StackOverflow](https://stackoverflow.com/questions/7157181/how-to-learn-the-structure-of-linux-wireless-drivers-mac80211)
  - the [slides](https://wireless.wiki.kernel.org/_media/en/developers/documentation/mac80211.pdf) referred to in the answer gives a good high level overview of wireless network infrastructure in linux.
- [MSDU](https://en.wikipedia.org/wiki/MAC_service_data_unit)
- [Frame aggregation](https://en.wikipedia.org/wiki/Frame_aggregation)
- [WPA](https://en.wikipedia.org/wiki/Wi-Fi_Protected_Access): WEP has been deprecated by WPA/WPA2/WPA3.
- [linux dma doc](https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt): we may need convert physical address to dma address if the mapping between them is not an identity mapping.
- [iommu - wikipedia](https://en.wikipedia.org/wiki/Input%E2%80%93output_memory_management_unit)
- [printk formats in linux](https://www.kernel.org/doc/Documentation/printk-formats.txt): in kernel %p by default will hash the pointer. Use %px to print the original address.
- [IOMMU SWIOTLB](https://wiki.gentoo.org/wiki/IOMMU_SWIOTLB): from this article, we can think SWIOTLB as a software version of hardware IOMMU. SWIOTLB is more flexible than IOMMU.
- [A more dynamic software I/O TLB - lwn](https://lwn.net/Articles/940973/): Some devices can only access low address memory. With SWIOTLB, buffers are reserved in low address. For memory that a device can not access, a buffer from low memory need to be obtained for the device to access and data need to be 'bounced' between these 2 memory spaces.

# Note

- BSSID or basic service set ID represnts mac address of the AP (access point) in infrastructure BSS.

- uapi folder in linux kernel means those files are user visible. Check [this](https://stackoverflow.com/questions/18858190/whats-in-include-uapi-of-kernel-source-project).

- skip calling `enable_interrupt` in `rtl_pci_start` will cause network fail. Can no longer ssh onto the machine!

- Add non-rate-limited log to `rtl88ee_enable_interrupt` / `rtl88ee_disable_interrupt` will flood dmesg since the interrupt handler will disable interrupt at the beginning and enable it in the end.

- linux still works after disabling the following features
  - by default linux rtl8188ee driver uses MSI interrupt mode. But forcing pin-based interrupt mode still works. So we don't need bother figuring out how to support MSI interrupt mode immediately.
  - in `rtl_pci_probe` -> `_rtl_pci_find_adapter` -> `rtl_pci_parse_configuration` a pci register with offset 0x70f is written. This can only be done with PCIe since PCI only support 256 bytes configuration space. However, skipping this write still work on debian.. Not urgent to support PCIe ATM.
  - disabling `rtl88ee_hal_cfg.write_readback` in sw.c
  - change all functions in led.c, ps.c to dummy return
  - skip calling `rtl_pci_init_aspm`, `rtl_init_rfkill` in `rtl_pci_probe`
  - skip calling `rtl_regd_init` in `rtl_init_core`.
  - skip calling `_rtl88ee_poweroff_adapter`, `_rtl88ee_set_media_status` in `rtl88ee_card_disable`
  - change all callbacks used in `_rtl_init_deferred_work` to dummy return function
  - change `rtl_op_bss_info_changed`, `rtl_op_stop` to dummy return

- Linux wifi STOP working after making the following changes:
  - change `rtl_op_remove_interface` to dummy
  - comment out the call to `rtl88e_phy_set_bw_mode_callback`

- `ieee80211_is_action`: return true if the frame is an action frame. Action frame
   is a kind of management frame.

- Verified on my toshiba, `rtl_op_sta_add` is called to add the associated AP. The printed mac address is the AP's rather than my laptop's.

- When running debian on my toshiba, using swiotlb is necessary since the host has 6GB memory while the wifi adaptor can only access the low 4GB.
