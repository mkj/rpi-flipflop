#define _BSD_SOURCE
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mount.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "readconf.h"
#include "flipflop.h"

static char* read_conf(const char* conf_name)
{
	char *ret = NULL;
	printf("Reading config %s\n", conf_name);
	char *work_name = strdup(conf_name);
	char *free_work_name = work_name;
	char *device = NULL, *fstype = NULL, *path = NULL;

	device = strsep(&work_name, ":");
	fstype = strsep(&work_name, ":");
	path = strsep(&work_name, ":");

	if (!(device && fstype && path))
	{
		printf("Bad config name.\n");
	}
	else
	{
		if (mount_device(device, fstype))
		{
			char confpath[MAX_FLIPFLOP_PATH];
			snprintf(confpath, sizeof(confpath), "/tmp/mnt/%s/%s", device, path);
			int fd = open(confpath, O_RDONLY);
			if (fd >= 0)
			{
				char buffer[1000];
				memset(buffer, 0x0, sizeof(buffer));
				int r = read(fd, buffer, sizeof(buffer)-1);
				if (r > 0)
				{
					// Success
					ret = strdup(buffer);
					printf("Parsed %s\n", confpath);
				}
				else
				{
					if (r == 0)
					{
						printf("Empty file %s\n", confpath);
					}
					else
					{
						printf("Problem reading %s: %s\n", confpath, strerror(errno));
					}

				}
				close(fd);
			}
			else
			{
				printf("Couldn't open '%s': %s\n", confpath, strerror(errno));

			}
		}
	}
	free(free_work_name);
	return ret;
}

void follow_conf(struct flipflop_config *conf, int safe_mode)
{
    // follow config files. 10 is more than sufficient.
    for (int i = 0; i < 10; i++)
    {
		char *next_conf = conf->bootmodes[safe_mode].next_settings;
		char *text = read_conf(next_conf);
		if (text)
		{
			parse_conf(text, conf);
		}
		free(text);
    }
}

void update_bootpart(const char* val, int* part)
{
	int v = atoi(val);
	if (v > 0)
	{
		*part = v;
	}
	else
	{
		printf("Bad boot partition '%s', should be a number >= 1\n", val);
	}
}

void update_nextconf(const char* val, char* next_settings)
{
	char *work = strdup(val);
	char *free_work = work;
	char *dev = strsep(&work, ":");
	char *fstype = strsep(&work, ":");
	char *file = strsep(&work, ":");

	if (strcmp(val, "-") == 0)
	{
		// blank, ignore it
	}
	else if (dev && fstype && file && *dev && *fstype && *file)
	{
		snprintf(next_settings, MAX_FLIPFLOP_PATH, "%s", val);
	}
	else
	{
		printf("Bad next_settings '%s': should be for example 'mmcblk0p1:vfat:flipflop.txt'\n",
			val);
	}
	free(free_work);
}

void parse_conf(const char* contents, struct flipflop_config *conf)
{
	char *work = strdup(contents);
	char *free_work = work;

	// ignore dos newlines
	for (char *line = work; line; line = strsep(&work, "\n\r"))
	{
		if (*line == '\0' || *line == '#' || *line == '!')
		{
			// empty line, dos newline, or comment
			continue;
		}

		char *workline = strdup(line);
		char *free_workline = workline;

		char* key = strsep(&workline, " ");
		char* value = strsep(&workline, " ");

		if (!(key && value))
		{
			printf("Bad config line '%s'\n", line);
		}

		if (strcmp(key, "normal_bootpart") == 0)
		{
			update_bootpart(value, &conf->bootmodes[0].boot_partition);
		} 
		else if (strcmp(key, "safe_bootpart") == 0)
		{
			update_bootpart(value, &conf->bootmodes[1].boot_partition);
		} 
		else if (strcmp(key, "normal_nextconf") == 0)
		{
			update_nextconf(value, conf->bootmodes[0].next_settings);
		} 
		else if (strcmp(key, "safe_nextconf") == 0)
		{
			update_nextconf(value, conf->bootmodes[1].next_settings);
		} 
		else
		{
			printf("Unknown config parameter '%s'\n", line);
		}
		free(free_workline);
	}

	free(free_work);
}
