#ifndef READCONF_H
#define READCONF_H

struct bootmode_config
{
	// partition number to boot
	int boot_partition; 
	// next settings file to load, prefixed by partition number.
	// For example "1:flipflop.txt" or "5:/conf/settings.txt"
	char *next_settings; 
};

struct flipflop_config
{
	struct bootmode_config bootmodes[2];
};

void parse(const char* contents, struct flipflop_config *config);

#endif // READCONF_H
