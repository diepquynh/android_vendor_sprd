#ifndef _BATTERY_H
#define _BATTERY_H

#define USB_ON_LINE      7
#define USB_OFF_LINE     8
#define KEY_LONG_PRESS   9
#define KEY_SHORT_PRESS  10

#define THRESHOLD_CLOSE_POWER          3500000   // 3.40v
#define LCD_BRIGHTNESS_ON              "on"
#define LCD_BRIGHTNESS_OFF             "off"

#define LCD_BACKLIGHT_BRIGHTNESS_PATH  "/sys/class/backlight/sprd_backlight/brightness"

#define LED_IOCTL_SET_COLOR  _IOW(0xB1,0x11,int)
#define LED_IOCTL_SET_TIME   _IOW(0xB1,0x12,int)

#define ERROR(x...)        log_write(3, "<3>alarm: " x)

#endif
