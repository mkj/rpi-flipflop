CONFIG_CROSS_COMPILE=/home/matt/rpibin/arm-bcm2708-linux-gnueabi-
CC=$(CONFIG_CROSS_COMPILE)cc
CFLAGS=-Wall -Os  -std=c99 -ffunction-sections -fdata-sections
LD=$(CONFIG_CROSS_COMPILE)ld
LDFLAGS=-static -Wl,--gc-sections

all: flipflop.initramfs

flipflop.initramfs: flipflop Makefile
	mkdir -p sbin
	cp flipflop init
	echo init  | cpio -o -H newc > $@

# you could also put it on a vfat filesystem and boot it with 
# "root=mmcblk0pX rootfstype=vfat"
# but it's simpler to make an initramfs
# 
flipflop.img: flipflop 
	dd if=/dev/zero bs=1M count=1 of=$@
	mkfs.vfat $@
	mcopy -i $@ flipflop ::/init.exe
	mcopy -i $@ flipflop.txt ::/flipflop.txt

flipflop: flipflop.o readconf.o 

clean:
	-rm flipflop flipflop.initramfs flipflop.o readconf.o

try: 
	QEMU_AUDIO_DRV=none qemu-system-arm -initrd flipflop -kernel ~/mnt/drpi/kernel-qemu -cpu arm1176 -m 256 -M versatilepb -no-reboot -serial stdio -append " console=ttyAMA0 panic=1" -hda flipflop.img  -nographic

bb.img:
	dd if=/dev/zero bs=3M count=1 of=$@
	mkfs.vfat $@
	mcopy -i $@ ~/mnt/drpi/busybox.exe ::/busybox.exe


bb: 
	QEMU_AUDIO_DRV=none qemu-system-arm -kernel ~/mnt/drpi/kernel-qemu -cpu arm1176 -m 256 -M versatilepb -no-reboot -serial stdio -append " console=ttyAMA0 root=/dev/sda panic=1 ro init=/busybox.exe" -hda flipflop.img  -nographic

