#ifndef _BATTERY_H_
#define _BATTERY_H_

#define BATTERY_STATUS_UNKNOWN 0x10f5
#define BATTERY_STATUS_CHARGING 0x10f6
#define BATTERY_STATUS_DISCHARGING 0x10f7
#define BATTERY_STATUS_NOT_CHARGING 0x10f8
#define BATTERY_STATUS_FULL 0x10f9
#define BATTERY_HEALTH_UNKNOWN 0x10fa
#define BATTERY_HEALTH_GOOD 0x10fb
#define BATTERY_HEALTH_OVERHEAT 0x10fc
#define BATTERY_HEALTH_DEAD 0x10fd
#define BATTERY_HEALTH_OVER_VOLTAGE 0x10fe
#define BATTERY_HEALTH_UNSPECIFIED_FAILURE 0x10ff

enum gFieldID{
    // members
    mAcOnline = 0,
    mUsbOnline,
    mBatteryStatus,
    mBatteryHealth,
    mBatteryPresent,
    mBatteryLevel,
    mBatteryVoltage,
    mBatteryTemperature,
    mBatteryTechnology,
    mBatteryEnd,
};

extern int battery_status_init(void);
extern  void * battery_status_update(void * cookie);
extern int battery_ac_online(void);
extern int battery_usb_online(void);
extern int battery_capacity(void);
extern int battery_status(void);

#endif
