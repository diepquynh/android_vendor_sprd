#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <poll.h>


#define INFO(x...) fprintf(stdout, ##x)
#define BATTERY_STATUS_PATH "/sys/class/power_supply/battery/status"
#define BATTERY_HEALTH_PATH "/sys/class/power_supply/battery/health"
#define BATTERY_VOLTAGE_PATH "/sys/class/power_supply/battery/voltage_now"
#define BATTERY_0_PATH "/sys/class/power_supply/battery/battery_0"
#define BATTERY_1_PATH "/sys/class/power_supply/battery/battery_1"
#define BATTERY_HW_SWITCH_POINT_PATH "/sys/class/power_supply/battery/hw_switch_point"
#define BATTERY_STOP_CHARGE_PATH "/sys/class/power_supply/battery/stop_charge"


/* read the information of battery */
static int readFromFile( const char* path, char* buf, size_t size)
{
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        INFO("error open battey  property file:%s\n", strerror(errno));
        return -1;
    }

    ssize_t count = read(fd, buf, size);
    if (count > 0) {
        while (count > 0 && buf[count-1] == '\n')
            count--;
        buf[count] = '\0';
    } else {
        buf[0] = '\0';
    }

    close(fd);
    return count;
}

/*write the exact data to some battery property item */
static int writeToFile(const char* path, char* buf, size_t size)
{
    int fd = open(path, O_WRONLY, 0);
    if (fd == -1) {
        INFO("error open battey  property file:%s\n", strerror(errno));
        return -1;
    }
    ssize_t count = write(fd, buf, size);
    if (count > 0) {
        INFO("write %s success\n", path);
        return count;
    } else {
        INFO("write %s failed\n", path);
        return count;
    }

    close(fd);
    return count;
}

static int read_battery_status()
{
    char buf[20];
    readFromFile(BATTERY_STATUS_PATH, buf, sizeof(buf));
    printf("Current status of the battery  %s\n", buf);
    return 0;
}
static int read_battery_health()
{
    char buf[20];
    readFromFile(BATTERY_HEALTH_PATH, buf, sizeof(buf));
    printf("Current condition of the battery   %s\n", buf);
    return 0;
}

static int read_battery_voltage()
{
    char buf[20];
    readFromFile(BATTERY_VOLTAGE_PATH, buf, sizeof(buf));
    printf("Current voltage of the battery   %s\n", buf);
    return 0;
}

static int set_battery_0()
{
    char buf[10] = "0x3A01068";  /*adc_voltage_table[0][] =  {928, 4200} */
    writeToFile(BATTERY_0_PATH, buf, sizeof(buf));
    return 0;
}

static int set_battery_1()
{
    char buf[10] = "0x31C0E10"; /*  adc_voltage_table[1][] =  {796, 3600},*/
    writeToFile(BATTERY_1_PATH, buf, sizeof(buf));
    return 0;
}

static int set_hw_swtich_point()
{
    char buf[10] = "0x15"; /* 20 (between 0 and 36) */
    writeToFile(BATTERY_HW_SWITCH_POINT_PATH, buf, sizeof(buf));
    return 0;
}

static int set_stop_charge()
{
    char buf[10] = "0x0"; /* ,  0 mean stop to charge */
    writeToFile(BATTERY_STOP_CHARGE_PATH, buf, sizeof(buf));
    return 0;
}


static void usage(void)
{
    INFO("Usage:\n");
    INFO("  utest_battery  status\n");
    INFO("  utest_battery  health\n");
    INFO("  utest_battery  voltage\n");
    INFO("  utest_battery  set_battery_0\n");
    INFO("  utest_battery  set_battery_1\n");
    INFO("  utest_battery  set_hw_switch_point\n");
    INFO("  utest_battery  stop_charge\n");
}

int main(int argc, char **argv)
{
    char *cmd;
    int rval = -EINVAL;

    if (argc < 2) {
        usage();
        return rval;
    }

    cmd = argv[1];
    argc--;
    argv++;
    INFO("utest battery -- %s\n", cmd);

    if (strcmp(cmd, "status") == 0) {
        rval = read_battery_status();
    }
    else if (strcmp(cmd, "health") == 0) {
        rval = read_battery_health();
    }
    else if (strcmp(cmd, "voltage") == 0) {
        rval = read_battery_voltage();
    }
    else if(strcmp(cmd, "set_battery_0") == 0){
        rval = set_battery_0();
    }
    else if(strcmp(cmd, "set_battery_1") == 0){
        rval = set_battery_1();
    }
    else if(strcmp(cmd, "set_hw_switch_point") == 0){
        rval = set_hw_swtich_point();
    }
    else if(strcmp(cmd, "stop_charge") == 0){
        rval = set_stop_charge();
    }
    if (rval == -EINVAL) {
        usage();
    }

    return rval;
}
