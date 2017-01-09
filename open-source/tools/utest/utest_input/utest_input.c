#include <linux/input.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <poll.h>

static int         time_seconds          =  0;                     /* test time*/
static char*   seqfile                        = NULL;

#define INFO(x...)	                                                    fprintf(stdout, ##x)
#define KEY_DEV_PATH                                            "/dev/input/event1"
#define TOUCHSCREEN_DEV_PATH                     "/dev/input/event2"


static void usage(void)
{
    INFO("Usage:\n");
    INFO("  utest_input key  [-r seconds] \n");
    INFO("  utest_input  key -f seqfile]\n");
}

/* interpret command argument option*/
void  process_options(int argc, char **argv)
{
    int opt = 0;

    while ((opt = getopt (argc, argv, "r:f:")) != -1) {
        switch (opt) {
        case 'r': /* indicate test time*/
            time_seconds = atoi(optarg);
            break;
        case 'f': /* indicate the input should like the requested list */
            seqfile = strdup(optarg);
            break;
        default:
            break;
        }
    }
}

  /* test for keypad-r option */
int  keytest_option_r()
{
    struct timespec ts_start, ts_end;
    struct pollfd fds[1];
    struct input_event event;
    int fd_kbd = open(KEY_DEV_PATH, O_RDWR);

    if(fd_kbd <= 0) {
        INFO("error open keyboard:%s\n", strerror(errno));
        return -1;
    }

    fds[0].fd = fd_kbd;

    /* test start */
    if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
         return -EIO;
    }

    while(ts_end.tv_sec - ts_start.tv_sec <= time_seconds)    /*if test time is out*/
    {
        fds[0].events = POLLIN;
        poll(fds, 1, 5000);    /*poll for 5000  ms*/
        if(fds[0].revents & POLLIN){
            if( read(fd_kbd, &event, sizeof(struct input_event)) <= 0){
                INFO("read error!\n");
                return -1;
            }
            INFO("  event.type = %d\n", event.type);
            INFO("  event.code = %d\n", event.code);
            INFO("  event.value= %d\n", event.value);

        }
        else{
            INFO("there is no key event with 5 seconds\n");
        }

        if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0) {
             return -EIO;
        }
    }

    close(fd_kbd);
    INFO("utest input(key -r) finished\n");
    return 0;
}

  /* test for kepad -f option */
int keytest_option_f()
{
    struct pollfd fds[1];
    struct input_event event;
    int next_volup = 0;
    int next_voldown = 0;
    int next_camara = 0;
    int test_finished = 0;
    int fd_kbd = open(KEY_DEV_PATH, O_RDWR);

    if(fd_kbd <= 0) {
        INFO("error open keyboard:%s\n", strerror(errno));
        return -1;
    }

    if(0 == strcmp(seqfile, "seqfile")){
        INFO("hit the key as the following sequence:\n");
        INFO("  FUNCTION(KEY_CODE), VALUE(DOWN:1,UP:0;\n");
        INFO("  KEY_POWER(116),     DOWN(1);\n");
        INFO("  KEY_POWER(116),     UP(0);\n");
        INFO("  KEY_VOLUP(24),      DOWN(1);\n");
        INFO("  KEY_VOLUP(24),      UP(0);\n");
        INFO("  KEY_VOLDOWN(8),     DOWN(1);\n");
        INFO("  KEY_VOLDOWN(8),     UP(0);\n");
        INFO("  KEY_CAMERA(9),      DOWN(1);\n");
        INFO("  KEY_CAMERA(9),      UP(0);\n");
    }
    else{
        usage();
        return 0;
    }

    fds[0].fd = fd_kbd;

    while(1)
    {
        fds[0].events = POLLIN;
        poll(fds, 1, -1);

        if(fds[0].revents & POLLIN)
        {
            if( read(fd_kbd, &event, sizeof(struct input_event)) <= 0)
            {
                INFO("read error!\n");
                return -1;
            }
            /* print out the input action */
            if(event.code && event.value)
                INFO("  eventcode:%d down\n", event.code);
            else if(event.code && !event.value)
                INFO("  eventcode:%d up\n", event.code);

            if(event.type == 1 && event.value == 1){
                /* test if the input order is exactly accord with the requested sequence*/
                if(event.code == 116)
                    next_volup = 1;
                else if(next_volup == 1 && event.code == 24)
                    next_voldown = 1;
                else if(next_voldown == 1 && event.code == 8)
                    next_camara = 1;
                else if(next_camara == 1 && event.code == 9){
                    test_finished = 1; /* test passed */
                    INFO("  eventcode:%d up\n", event.code);
                    break;
                }
                else{
                    INFO("input wrong,please quit and test again\n");
                    break;
                }
            }
        }
    }

    close(fd_kbd);
    if( test_finished == 1)
    INFO("utest input(key -f) finished\n");
    return 0;
}

/* test for touchscreen -r option */
int touchtest_option_r()
{
    struct timespec ts_start, ts_end;
    struct timeval sr_start, sr_end;
    struct pollfd fds[1];
    struct input_event event;

    double duration;
    double temp_rate = 0.0;
    int maximum_rate = 0;
    int minimum_rate =1000000;
    int average_rate = 0;
    int total_rate = 0;
    int int_temp_rate = 0;
    int report_count = 0;
    int rate_count = 0;

    int fd_kbd = open(TOUCHSCREEN_DEV_PATH, O_RDWR);

    if(fd_kbd <= 0) {
        INFO("error open touchscreen:%s\n", strerror(errno));
        return -1;
    }

    fds[0].fd = fd_kbd;

    /* test start */
    if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
        return -EIO;
    }

    while(ts_end.tv_sec - ts_start.tv_sec <= time_seconds)/*if test time is out*/
    {
        fds[0].events = POLLIN;
        poll(fds, 1, 5000); /*poll for 5000  ms*/

        if(fds[0].revents & POLLIN)
        {
            if( read(fd_kbd, &event, sizeof(struct input_event)) <= 0){
                INFO("read error!\n");
                return -1;
            }
            INFO("  event.type = %d\n", event.type);
            INFO("  event.code = %d\n", event.code);
            INFO("  event.value= %d\n", event.value);

            report_count++;
            if(event.type == 1 && event.value == 1){
                sr_start = event.time; /* start to sample current touch event */
                report_count =  0;
            }
            else if(event.type == 1 && event.value == 0){
                sr_end =  event.time; /* stop to sample current touch event */
                duration = sr_end.tv_sec - sr_start.tv_sec + (sr_end.tv_usec - sr_start.tv_usec)/1000000.0; /* current touch event sample time */
                temp_rate = 1.0 * report_count /duration; /* current touch event sample rate */
                int_temp_rate = temp_rate; /* change int double to int */
                INFO("  curren sample : %d times/s\n", int_temp_rate);

                total_rate += int_temp_rate;
                rate_count++;

                /* find out maximum sample rate */
                if(int_temp_rate  > maximum_rate)
                    maximum_rate =  int_temp_rate;

                /* find out minimum sample rate */
                if(int_temp_rate < minimum_rate)
                    minimum_rate   = int_temp_rate;
            }
        }
        else
         INFO("there is no touchscreen event with 5 seconds\n");

        if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0)
            return -EIO;
    }

    average_rate = total_rate/rate_count;

    INFO("  maximum_rate:   %d times/s\n",  maximum_rate);
    INFO("  minimum_rate:   %d times/s\n",  minimum_rate);
    INFO("  average_rate:    %d times/s\n",  average_rate);

    close(fd_kbd);
    INFO("utest input(touch -r) finished\n");
    return 0;
}

  /* test for touchscreen -f option */
int touchtest_option_f()
{
    struct pollfd fds[1];
    struct input_event event;
    int left_top = 0;
    int left_bottom = 0;
    int right_top = 0;
    int right_bottom = 0;
    int middle = 0;
    int left_x = 0;
    int right_x = 0;
    int top_y = 0;
    int bottom_y = 0;
    int middle_x = 0;
    int middle_y = 0;
    int test_finished = 0;
    int input_wrong = 0;
    int fd_kbd = open(TOUCHSCREEN_DEV_PATH, O_RDWR);

    if(fd_kbd <= 0) {
        INFO("error open touchsreen:%s\n", strerror(errno));
        return -1;
    }

    if(0 == strcmp(seqfile, "seqfile")){
        INFO("touch the screen position as the following sequence:\n");
        INFO("  POSITION, VALUE(DOWN:1,UP:0;\n");
        INFO("  LEFT_TOP,     DOWN(1);\n");
        INFO("  LEFT_TOP,     UP(0);\n");
        INFO("  LEFT_BOTTOM,      DOWN(1);\n");
        INFO("  LEFT_BOTTOM,      UP(0);\n");
        INFO("  RIGHT_TOP,     DOWN(1);\n");
        INFO("  RIGHT_TOP,     UP(0);\n");
        INFO("  RIGHT_BOTTOM,      DOWN(1);\n");
        INFO("  RIGHT_BOTTOM,      UP(0);\n");
        INFO("  MIDDLE,      DOWN(1);\n");
        INFO("  MIDDLE,      UP(0);\n");
    }
    else{
        usage();
        return 0;
    }

    fds[0].fd = fd_kbd;

    while(1)
    {
        fds[0].events = POLLIN;
        poll(fds, 1, -1);

        if(fds[0].revents & POLLIN)
        {
            if( read(fd_kbd, &event, sizeof(struct input_event)) <= 0)
            {
                INFO("read error!\n");
                return -1;
            }
            if(event.type == 3 ){ /*  the touch event */
                INFO("touch position: %d  %d\n", event.code, event.value);
                switch(event.code)
                {
                case 53: /* the x coordinate of current touch event*/
                    if(0 <= event.value && 50 >= event.value)
                        left_x = 1;
                    else if(450 <= event.value && 500 >= event.value)
                        right_x = 1;
                    else if(210 <= event.value && 260 >= event.value)
                        middle_x = 1;

                    break;
                case 54: /* the y coordinate of current touch event */
                {
                    if(0 <= event.value && 50 >= event.value)
                        top_y = 1;
                    else if(760 <= event.value && 860 >= event.value)
                        bottom_y = 1;
                    else if( 330 <= event.value && 400 >= event.value)
                        middle_y = 1;

                    /*   if the position of  touch event is accord with the requested sequence */
                    if(left_x ==1 && top_y == 1) /* left_top */
                        left_bottom = 1;
                    else if(left_bottom == 1 && left_x ==1 && bottom_y == 1) /* after left_top is left_bottom */
                        right_top = 1;
                    else if(right_top == 1 && right_x == 1 && top_y == 1) /* after left_bottom is right_top */
                        right_bottom = 1;
                    else if(right_bottom == 1 && right_x == 1 && bottom_y == 1) /* after right_top is right_bottom */
                        middle = 1;
                    else if(middle == 1 && middle_x == 1 && middle_y == 1)  /* after right_bottom is middle */
                        test_finished = 1;
                    else
                        input_wrong = 1;  /* touch sequence wrong */

                    left_x = 0;
                    right_x = 0;
                    top_y = 0;
                    bottom_y = 0;
                    break;
                }
                default:
                    break;
                }

                if(test_finished == 1)
                break;
                if(input_wrong ==1){
                INFO("input wrong,please quit and test again\n");
                break;
                }
            }
        }
    }

    close(fd_kbd);
    if( test_finished == 1)
    INFO("utest input(touch -f) finished\n");
    return 0;
}

int do_keytest()
{
    int rval = -EINVAL;
    if(time_seconds != 0)
        rval = keytest_option_r();
    else if(seqfile != NULL)
        rval = keytest_option_f();

    return rval;
}

int do_touchtest()
{
    int rval = -EINVAL;
    if(time_seconds != 0)
        rval = touchtest_option_r();
    else if(seqfile != NULL)
        rval = touchtest_option_f();

    return rval;
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
    INFO("utest input -- %s\n", cmd);

    if (strcmp(cmd, "key") == 0) {
        process_options(argc, argv);
        rval = do_keytest();
    }
    else if (strcmp(cmd, "touch") == 0) {
        process_options(argc, argv);
        rval = do_touchtest();
    }

    if (rval == -EINVAL) {
        usage();
    }

    return rval;
}
