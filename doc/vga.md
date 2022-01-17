# VGA Text Mode
VGA has a graphicis mode and a text mode. We use the text mode when the OS boots. GUI can be implemented with the graphics mode though.

In text model, VGA can be accessed from memory at location 0xB8000. This memory region can be viewed as 80x25 (80 columns, 25 rows) array of 2 bytes. Each of these 2 bytes have the lower byte reprensents the character and higher byte represents the foreground and background color.

# Reference
- [A comparison between VGA, DVI and HDMI](https://monopricesupport.kayako.com/article/124-what-is-the-difference-between-dvi-hdmi-and-vga)
- [VGA on wikipedia](https://en.wikipedia.org/wiki/Video_Graphics_Array)
- [VGA Text Mode on wikipedia](https://en.wikipedia.org/wiki/VGA_text_mode)
- [Color Palette](https://en.wikipedia.org/wiki/List_of_8-bit_computer_hardware_graphics#4-bit_RGBI_palettes)
