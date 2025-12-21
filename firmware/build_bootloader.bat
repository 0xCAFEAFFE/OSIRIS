cd ./bootloader
avr-gcc -g -Wall -Os -fno-split-wide-types -mrelax -mmcu=atmega328pb -DF_CPU=8000000L -DBAUD_RATE=28800 -DLED=D3 -DLED_START_FLASHES=0 -DOPTIBOOT_CUSTOMVER=100 -Wl,-Tlink_optiboot.ld -Wl,--relax -nostartfiles optiboot.c -o optiboot.elf
avr-objcopy -j .text -j .data -j .version --set-section-flags .version=alloc,load -O ihex optiboot.elf optiboot.hex
avr-size -C optiboot.elf
pause