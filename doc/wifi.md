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

