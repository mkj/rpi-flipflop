#ifndef READCONF_H
#define READCONF_H

// that's three lines wrapped on my terminal.
#define MAX_FLIPFLOP_PATH 421

struct bootmode_config
{
	// partition number to boot
	int boot_partition; 
	// next settings file to load, prefixed by block device (as found in /sys/class/block)
	// For example "mmcblk0p1:vfat:flipflop.txt" or "mmcblk0p5:ext3:/conf/settings.txt" or "sda2:vfat:thing.txt"
	char next_settings[MAX_FLIPFLOP_PATH];
};

struct flipflop_config
{
	struct bootmode_config bootmodes[2]; // 0 normal, 1 safe
};

void parse_conf(const char* contents, struct flipflop_config *config);
void follow_conf(struct flipflop_config *conf, int safe_mode);

#endif // READCONF_H
