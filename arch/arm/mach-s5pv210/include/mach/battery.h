#ifndef __BATTERY_H_
#define __BATTERY_H_

typedef enum
{
	PM_CHARGER_NULL,
	PM_CHARGER_TA,
	PM_CHARGER_USB_CABLE, //after enumeration
	PM_CHARGER_USB_INSERT,// when usb is connected.
	PM_CHARGER_DEFAULT
} charging_device_type;

#endif
