#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "flipflop.h"
#include "readconf.h"

/* What to boot if flipflop encounters an error */
static const int FLIPFLOP_DEFAULT_NORMAL = 5;

/* Config used by default */
static const char *default_config =
"normal_bootpart 5\n"
"normal_nextconf mmcblk0p1:vfat:flipflop.txt\n"
"safe_bootpart 2\n"
"safe_nextconf -\n";

static const int SAFEMODE_GPIO = 3;

static const char* CMDLINE_NORMAL = "flipflop_force_normal";
static const char* CMDLINE_SAFE = "flipflop_force_safe";

static void fail(const char* msg)
{
    printf("%s\n", msg);
    for (int i = 15; i > 1; i--)
    {
        printf("\rWill reboot in %d seconds    ", i);
        fflush(stdout);
        sleep(1);
    }
    printf("\rWill reboot in 1 second     ");
    fflush(stdout);
    sleep(1);
    reboot(RB_AUTOBOOT);
    exit(1);
}

void failed_ensure(const char *desc, int line)
{
    char str[2000];
    snprintf(str, sizeof(str), "Failure. %s. Line %d, %s", desc, line, strerror(errno));
    fail(str);
}

void setup_fs()
{
    ENSURE(mount("tmpfs", "/", "tmpfs", MS_REMOUNT, "") == 0, "mount / tmpfs");
    ENSURE(chdir("/") == 0, "chdir /");
    ENSURE(mkdir("/tmp", 0700) == 0, "mkdir /tmp");
    ENSURE(mkdir("/tmp/mnt", 0700) == 0, "mkdir /tmp/mnt");
    ENSURE(mkdir("/sys", 0700) == 0, "mkdir /sys");
    ENSURE(mkdir("/proc", 0700) == 0, "mkdir /proc");

    ENSURE(mkdir("/dev", 0700) == 0, "mkdir /dev");
    ENSURE(mount("sysfs", "/", "sysfs", 0, "") == 0, "mount /sys");
    ENSURE(mount("proc", "/proc", "proc", 0, "") == 0, "mount /proc");
}

/* Reads a GPIO pin to determine if it's safe mode */
static int get_gpio_safemode()
{
    /* Avoids failing if gpio files don't exist for some reason */

    int fd;
    char buf[200];
    ssize_t len;
    int ret = 0;
    // set up pin
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0)
    {
        printf("Couldn't open /sys/class/gpio/export, %s\n", strerror(errno));
        return 0;
    }
    len = snprintf(buf, sizeof(buf), "%d", SAFEMODE_GPIO);
    ENSURE(write(fd, buf, len) == len, "write gpio export");
    ENSURE(close(fd) == 0, "close gpio export"); 
    fd = -1;

    // read it
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", SAFEMODE_GPIO);
    fd = open(buf, O_RDONLY);
    if (fd < 0)
    {
        printf("Couldn't open %s, %s\n", buf, strerror(errno));
        return 0;
    }
    ENSURE(read(fd, buf, 1) == 1, "read gpio pin");
    ENSURE(close(fd) == 0, "close gpio pin");
    fd = -1;

    if (buf[0] == '1')
    {
        ret = 1;
    }
    else if (buf[0] != '0')
    {
        printf("Pin value 0x%x\n", buf[0]);
        fail("Unexpected gpio pin value");
    }

    // clear up
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0)
    {
        printf("Couldn't open /sys/class/gpio/unexport, %s\n", strerror(errno));
        return 0;
    }
    len = snprintf(buf, sizeof(buf), "%d", SAFEMODE_GPIO);
    ENSURE(write(fd, buf, len) == len, "write gpio unexport");
    ENSURE(close(fd) == 0, "close gpio unexport"); 
    fd = -1;

    return ret;
}

static int
force_cmdline_safemode(int existing)
{
    int ret = existing;
    char buf[1000];
    int fd;
    memset(buf, 0x0, sizeof(buf));

    ENSURE((fd = open("/proc/cmdline", O_RDONLY)) >= 0, "open /proc/cmdline");
    ENSURE(read(fd, buf, sizeof(buf)) >= 0, "read /proc/cmdline");
    if (strstr(buf, CMDLINE_SAFE))
    {
        printf("Command line override '%s', setting safe mode.\n", CMDLINE_SAFE);
        ret = 1;
    }
    if (strstr(buf, CMDLINE_NORMAL))
    {
        printf("Command line override '%s', setting normal mode.\n", CMDLINE_NORMAL);
        ret = 0;
    }
    ENSURE(close(fd) == 0, "close cmdline");
    return ret;
}

static void
set_boot_partition(int part)
{
    int fd;
    char buf[10];
    int len;
    len = snprintf(buf, sizeof(buf), "%d", part);

    ENSURE((fd = open("/sys/module/bcm2708/parameters/reboot_part", O_WRONLY)) >= 0, "open /sys/module/bcm2708/parameters/reboot_part");
    ENSURE(write(fd, buf, len) == len, "write reboot_part");
    close(fd);
}

int main(int argc, char ** argv)
{
    printf("Welcome to flipflop Linux.\n");

    /* If flipflop fails then try booting "normally" */
    set_boot_partition(FLIPFLOP_DEFAULT_NORMAL);

    setup_fs();

    int safe_mode = get_gpio_safemode();

    if (safe_mode)
    {
        printf("GPIO safe mode\n");
    }
    else
    {
        printf("GPIO normal mode\n");
    }

    safe_mode = force_cmdline_safemode(safe_mode);

    struct flipflop_config conf;
    memset(&conf, 0x0, sizeof(conf));
    parse_conf(default_config, &conf);
    follow_conf(&conf, safe_mode);

    int boot_part = conf.bootmodes[safe_mode].boot_partition;
    set_boot_partition(boot_part);

    printf("Ready to reboot to partition %d\n", boot_part);
    sleep(1);
    printf("Bye.\n");
    reboot(RB_AUTOBOOT);
    return 1;
}
