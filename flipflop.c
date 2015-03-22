#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>



static void fail(const char* msg)
{
    printf("Problem: %s\n", msg);
    for (int i = 30; i > 2; i--)
    {
        printf("\rWill reboot in %d seconds    ", i);
        sleep(1);
    }
    printf("\rWill reboot in 1 second");
    sleep(1);
    exit(1);
}

int main(int argc, char ** argv)
{
    printf("Welcome to flipflop Linux.\n");

    if (mount("tmpfs", "/", "tmpfs", 0, "") != 0)
    {
        fail("Couldn't mount tmpfs");
    }

    if (mkdir("/tmp", 0700) != 0) {
        fail("Couldn't make /tmp");
    }
    if (mkdir("/sys", 0700) != 0) {
        fail("Couldn't make /sys");
    }

    if (mount("sysfs", "/", "sysfs", 0, "") != 0)
    {
        fail("Couldn't mount sysfs");
    }

    printf("Hooray!\n");
    while  (1)
    {
        sleep(2);
    }

    return 0;
}
