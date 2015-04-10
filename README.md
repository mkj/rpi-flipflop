# Raspberry Pi Flipflop

This is a replacement for the old kernel\_emergency.img that old Raspberry Pi 
firmwares used, see http://elinux.org/RPI_safe_mode .

It lets you toggle booting two different partitions (kernels/distros/images)
depending on the state of GPIO pin 3.

You specify a "normal partition" and a "safe partition". Usually the Raspberry Pi
will boot the normal partion. If GPIO pin 3 (pin 5 on the physical header) is
held to ground it will boot the safe partition. 

I am using this in conjunction with a hardware watchdog that toggles the "safe boot"
GPIO pin every few hours https://github.com/mkj/pihelp . That ensures that I 
can access the device even if I break one of the partitions - I have a fallback
rescue partition.

Matt Johnston <matt@ucc.asn.au>

## Installing

- Build flipflop by running `make`. I've been cross-compiling from a x86 Ubuntu box,
  I assume building on the Raspberry Pi itself should work too. The output is `flipflop.initramfs`

- Create a SD card with a few partitions
 - mmcblk0p1 - vfat, perhaps 40MB. I'll call this the "first boot partition", it has 
   a Linux kernel and the flipflop initramfs (and other boot files).

 - "normal boot" partition, has all the boot files for the normal distro.
 - "safe" partition, also has all the boot files but for the safe distro. Its 
   cmdline.txt probably points at a different root partition to normal.
 - The root partitions for each of the normal/safe partitions
 - Anything else (including config file partition, see below)

- Set up a vfat mmcblk0p1 partition with the contents of 
  https://github.com/raspberrypi/firmware/tree/master/boot 
  or /boot from recent Raspbian. 
  ```
COPYING.linux           cmdline.txt         issue.txt 
LICENCE.broadcom        config.txt          kernel.img 
LICENSE.oracle          fixup.dat           kernel7.img 
bcm2708-rpi-b-plus.dtb  fixup_cd.dat        overlays/
bcm2708-rpi-b.dtb       fixup_x.dat         start.elf 
bcm2709-rpi-2-b.dtb     flipflop.initramfs  start_cd.elf 
bootcode.bin            start_x.elf         flipflop.txt
```

- Copy `flipflop.initramfs` to the first boot partition. Edit config.txt and add a line
  ```
initramfs flipflop.initramfs followkernel
```

- Edit flipflop.txt to customise the config. An example to boot mmcblk0p5 in normal mode
  or mmcblk0p8 in safe mode
  ```
normal_bootpart 5
safe_bootpart 8
```
  You can also point it at further config files to parse - this is useful if you want
  to avoid ever editing your mmcblk0p1 for safety.
  ```
normal_nextconf mmcblk0p2:vfat:nextconf.txt
safe_nextconf mmcblk0p2:vfat:flipflop.txt
```
  The default configuration is hardcoded near the top of [flipflop.c](flipflop.c)

- Install normal distros to your normal and safe partitions and it should work.

## How it works

Flipflop boots a Linux kernel that runs flipflop as /init in an initramfs.
It looks at the GPIO pins then sets the Raspberry Pi's reboot parameter 
`/sys/module/bcm2708/parameters/reboot_part` as appropriate. 
Then it reboots and the device boots whichever partition you chose. It adds a few seconds
to the boot time.

This works the same as [NOOBS](https://github.com/raspberrypi/noobs) - it uses the same
parameter to automatically boot the installed distro if the "safe mode" pin isn't held. 
