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
- [How Does A Computer Detect Wifi? - superuser](https://superuser.com/questions/1470252/how-does-a-computer-detect-wifi): This doc refers to the ieee 802.11 spec!
- [Beacon Frame - wikipedia](https://en.wikipedia.org/wiki/Beacon_frame)
- [IEEE 802 - wikipedia](https://en.wikipedia.org/wiki/IEEE_802)
- [IEEE 802.11 - wikipedia](https://en.wikipedia.org/wiki/IEEE_802.11)
- [802.11 Wireless Networks: the definitive guide](https://paginas.fe.up.pt/~ee05005/tese/arquivos/ieee80211.pdf)
- [Linux firmware - gentoo wiki](https://wiki.gentoo.org/wiki/Linux_firmware)
- [About firmware - lfs](https://www.linuxfromscratch.org/blfs/view/svn/postlfs/firmware.html): This doc mentions various firmware file can be found at: https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain . I found the firmware file for rtl8188ee (https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain/rtlwifi/rtl8188efw.bin).

# Note

- BSSID or basic service set ID represnts mac address of the AP (access point) in infrastructure BSS.

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

- change `rtl_op_remove_interface` to dummy return cause wifi not work!

