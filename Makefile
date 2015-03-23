CONFIG_CROSS_COMPILE="/home/matt/rpibin/arm-bcm2708-linux-gnueabi-"
CC=$(CONFIG_CROSS_COMPILE)cc
CFLAGS=-Wall -Os  -std=c99
LD=$(CONFIG_CROSS_COMPILE)ld
LDFLAGS=-static

all: flipflop.img

flipflop.img: flipflop 
	dd if=/dev/zero bs=1M count=1 of=$@
	mkfs.vfat $@
	mcopy -i $@ flipflop ::/init.exe

flipflop: flipflop.o readconf.o

try: 
	QEMU_AUDIO_DRV=none qemu-system-arm -kernel ~/mnt/drpi/kernel-qemu -cpu arm1176 -m 256 -M versatilepb -no-reboot -serial stdio -append " console=ttyAMA0 root=/dev/sda panic=1 ro init=/init.exe" -hda flipflop.img  -nographic

