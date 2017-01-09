#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <mach/regs_ana.h>
#include <mach/adi_hal_internal.h>
#include <linux/time.h>
#include <linux/delay.h>
#define VIBRATOR_REG_UNLOCK	(0xA1B2)
#define VIBRATOR_REG_LOCK	(~VIBRATOR_REG_UNLOCK)
#define VIBRATOR_STABLE_LEVEL	(2)
#define VIBRATOR_INIT_LEVEL	(11)	//init level must larger than stable level
#define	VIBRATOR_INIT_STATE_CNT	(10)

static struct work_struct vibrator_work;
static struct hrtimer vibe_timer;
static spinlock_t vibe_lock;
static int vibe_state;
static int engtest=0;


static void set_vibrator(int on)
{
	ANA_REG_SET(ANA_VIBR_WR_PROT, VIBRATOR_REG_UNLOCK);	//unlock vibrator registor
	if(on == 0){
		if(engtest==1) return;
		ANA_REG_AND(ANA_VIBRATOR_CTRL0, ~(VIBR_PD_SET | VIBR_PD_RST));
		ANA_REG_OR(ANA_VIBRATOR_CTRL0, VIBR_PD_SET);
	}else{
		ANA_REG_AND(ANA_VIBRATOR_CTRL0, ~(VIBR_PD_SET | VIBR_PD_RST));
		ANA_REG_OR(ANA_VIBRATOR_CTRL0, VIBR_PD_RST);
	}
	ANA_REG_SET(ANA_VIBR_WR_PROT, VIBRATOR_REG_LOCK);	//lock vibrator registor
}
static void vibrator_hw_init(void)
{
	ANA_REG_SET(ANA_VIBR_WR_PROT, VIBRATOR_REG_UNLOCK);	//unlock vibrator registor

	ANA_REG_OR(ANA_VIBRATOR_CTRL0, VIBR_RTC_EN);	
	ANA_REG_AND(ANA_VIBRATOR_CTRL0, ~VIBR_BP_EN);	//enable new version,so VIBR_V_BP is disable.

	ANA_REG_MSK_OR(ANA_VIBRATOR_CTRL0, (VIBRATOR_INIT_LEVEL << VIBR_INIT_V_SHIFT), VIBR_INIT_V_MSK); //set init current level
	ANA_REG_MSK_OR(ANA_VIBRATOR_CTRL0, (VIBRATOR_STABLE_LEVEL << VIBR_STABLE_V_SHIFT), VIBR_STABLE_V_MSK); //set stable current level
	ANA_REG_SET(ANA_VIBRATOR_CTRL1, VIBRATOR_INIT_STATE_CNT);	//set convert count
		
	ANA_REG_SET(ANA_VIBR_WR_PROT, VIBRATOR_REG_LOCK);	//lock vibrator registor
}
static void update_vibrator(struct work_struct *work)
{
	set_vibrator(vibe_state);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	unsigned long	flags;

	spin_lock_irqsave(&vibe_lock, flags);
	hrtimer_cancel(&vibe_timer);

	if (value == 0)
		vibe_state = 0;
	else {
		value = (value > 15000 ? 15000 : value);
		vibe_state = 1;
		hrtimer_start(&vibe_timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&vibe_lock, flags);

	schedule_work(&vibrator_work);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		return r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	vibe_state = 0;
	schedule_work(&vibrator_work);
	return HRTIMER_NORESTART;
}

static struct timed_output_dev sprd_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

ssize_t vibratortest_status_show(struct class *class, struct class_attribute *attr, char *buf) 
{ 

    return 0; 
} 

ssize_t vibratortest_status_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count) 
{ 
       int type = buf[0]; 
      if(type=='1') 
      {
      engtest=1;
     set_vibrator(1);
    }
         else 
         {
           engtest=0;
            set_vibrator(0); 
         }

       return count; 
} 
ssize_t flashled_show(struct class *class, struct class_attribute *attr, char *buf) 
{ 

    return 0; 
} 

ssize_t flashled_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count) 
{ 
       int type = buf[0]; 
      if(type=='1') 
      {
       
      gpio_request(135,"gpio135");
			gpio_direction_output(135,1);
			gpio_set_value(135,1);

			gpio_request(144,"gpio144");
			gpio_direction_output(144,1);
			gpio_set_value(144,1); 
       
    }
         else 
         {
           gpio_request(135,"gpio135");
			gpio_direction_output(135,0);
			gpio_set_value(135,0);
			gpio_request(144,"gpio144");
			gpio_direction_output(144,0);
			gpio_set_value(144,0);
         }

       return count; 
} 



static struct class_attribute vibratortest_class_attrs[] = { 
        __ATTR(status, 0664, vibratortest_status_show, vibratortest_status_store), 
        __ATTR(flashled, 0664, flashled_show, flashled_store), 
        __ATTR_NULL, 
}; 


struct class vibratortest_class = { 
    .name           = "vibratortest", 
    .owner          = THIS_MODULE, 
    .class_attrs    = vibratortest_class_attrs, 
}; 
void __init sprd_init_vibrator(void)
{
	vibrator_hw_init();	//vibrator hardware init

	INIT_WORK(&vibrator_work, update_vibrator);

	spin_lock_init(&vibe_lock);
	vibe_state = 0;
	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;
        class_register(&vibratortest_class);
	timed_output_dev_register(&sprd_vibrator);
}

module_init(sprd_init_vibrator);
MODULE_DESCRIPTION("sprd timed output vibrator device");
MODULE_LICENSE("GPL");

