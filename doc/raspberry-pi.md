# Hardware Info
I'm using a RaspberryPi 4. As 2023 December, the latest version is RaspberryPi 5.

## Network Controller
lspci does not show the ethernet / wifi controllers. Most likely on RaspberryPi, ethernet/wifi controllers are not PCI devices.

hwinfo command shows that the wireless controller is using driver called brcmfmac. According to [this](https://wiki.debian.org/brcmfmac), brcmfmac is Broadcom's driver. That implies RaspberryPi 4's internal wifi controller is from Broadcom. If one wants to use a wifi controller from another brand, one easy way is to try USB wifi controller.

# Reference
- [official website](https://www.raspberrypi.com/)
- [Getting started](https://www.raspberrypi.com/documentation/computers/getting-started.html)
  - raspberry pi will first look for an (micro) SD card and then a USB drive, to find bootable media.
  - It's recommended to use RaspberryPi Imager to burn the OS to SD card or usb drive. RespberryPi Imager is like a customized version of balenaEtcher for RespberryPi.
    It ships with or download from internet the OS images.
- [Apple USB keyboad Doesn't work with RaspberryPi because of USB power issue](https://forums.raspberrypi.com/viewtopic.php?t=12676)
- [Instructions to install Case Fan](https://www.youtube.com/watch?v=BP44pCxQWAY)
- [camera hardware](https://www.raspberrypi.com/documentation/accessories/camera.html): contains instructions to install the hardware.
- [camera sofeware](https://www.raspberrypi.com/documentation/computers/camera_software.html). A long doc. To start, try to run libcamera-hello .
