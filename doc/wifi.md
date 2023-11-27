# Wireshark
- [wireshark - wikipedia](https://en.wikipedia.org/wiki/Wireshark)
  - wireshark is very similar to tcpdump but has a GUI and integrates sorting and filtering options.
  - wireshark has a terminal version called tshark

Here are a few things about inspecting 802.11 management frames with wireshark on MacOS
1. in wireshark GUI 'capture options', the 'monitor' checkbox for the inspected interface should be checked.
2. run airport to switch the interested interface to monitor mode. MacOS already has a tool called airport installed at /System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport . Run `sudo airport <interface-name> sniff 1` to switch the interface to monitored mode. Refer to [how to put MacOS X wireless adapter in monitor mode](https://unix.stackexchange.com/questions/48671/how-to-put-mac-os-x-wireless-adapter-in-monitor-mode) in more details.
