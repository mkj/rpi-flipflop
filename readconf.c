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

static void strip_dos_newlines(char *buf)
{
	for ( ; *buf; buf++)
	{
		if (*buf == 0xd)
		{
			*buf = ' ';
		}
	}
}

static int mount_device(const char* device, const char* fstype)
{
	char mntpath[100], devpath[100];
	snprintf(mntpath, sizeof(mntpath), "/tmp/mnt/%s", device);
	snprintf(devpath, sizeof(devpath), "/%s", device);
	if (access(mntpath, F_OK) == 0)
	{
		return 1;
	}

	char syspath[200];
	snprintf(syspath, sizeof(syspath), "/sys/class/block/%s/dev", device);
	int fd = open(syspath, O_RDONLY);
	if (fd < 0) {
		printf("Couldn't open %s. %s\n", syspath, strerror(errno));
		return 0;
	}
	char nodbuf[100];
	memset(nodbuf, 0x0, sizeof(nodbuf));
	if (read(fd, nodbuf, sizeof(nodbuf)) < 0)
	{
		printf("Couldn't read %s. %s\n", syspath, strerror(errno));
		close(fd);
		return 0;
	}
	close(fd);

	int maj, min;
	if (sscanf(nodbuf, "%d:%d", &maj, &min) != 2)
	{
		printf("Bad format from %s: '%s'\n", syspath, nodbuf);
		return 0;
	}

	if (mknod(devpath, 0700 | S_IFBLK, makedev(maj, min)) != 0)
	{
		if (errno != EEXIST)
		{
			printf("Couldn't create %s with %d:%d. %s\n",
				devpath, maj, min, strerror(errno));
			return 0;
		}
	}

	if (mount(devpath, mntpath, fstype, MS_RDONLY, "") != 0)
	{
		printf("Failed mounting %s at %s (type %s): %s\n",
			devpath, mntpath, fstype, strerror(errno));
		return 0;
	}

	return 1;
}

static char* read_conf(const char* conf_name)
{
	printf("Reading config %s\n", conf_name);
	char *work_name = strdup(conf_name);
	char *tw = work_name;
	char *device = NULL, *fstype = NULL, *path = NULL;

	device = strsep(&tw, ",");
	fstype = strsep(&tw, ",");
	path = strsep(&tw, ",");

	if (!(device && fstype && path))
	{
		printf("Bad config name.\n");
	}
	else
	{
		if (mount_device(device, fstype))
		{
			char confpath[421];
			snprintf(confpath, sizeof(confpath), "/tmp/mnt/%s/%s", device, path);
			// XXX here.
			// remember to null terminate it.
		}
	}
	free(work_name);
}

void follow_conf(struct flipflop_config *conf, int safe_mode)
{
    // keep following configs. 10 is more than sufficient.
    for (int i = 0; i < 10; i++)
    {
		char *next_conf = strdup(conf->bootmodes[safe_mode].next_settings);
		char *text = read_conf(next_conf);
		free(next_conf); 
		next_conf = NULL;
		parse_conf(text, conf);
		free(text);
    }
}

void parse_conf(const char* contents, struct flipflop_config *config)
{
	char *work = strdup(contents);
	strip_dos_newlines(work);

	// strsep on newlines. Ignore # comment

	free(work);
}
