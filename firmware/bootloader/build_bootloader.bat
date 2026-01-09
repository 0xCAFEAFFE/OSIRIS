@echo off
cd ./src
avr-gcc -g -Wall -Os -fno-split-wide-types -mrelax -mmcu=atmega328pb -DF_CPU=8000000L -DBAUD_RATE=28800 -DLED=D3 -DLED_START_FLASHES=0 -DOPTIBOOT_CUSTOMVER=100 -Wl,-T"../link_optiboot.ld" -Wl,--relax -nostartfiles -I"../inc" optiboot.c -o optiboot.elf
avr-objcopy -j .text -j .data -j .version --set-section-flags .version=alloc,load -O ihex optiboot.elf ../OSIRIS_BL.hex
avr-size optiboot.elf
del optiboot.elf
pause