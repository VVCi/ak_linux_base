#ifndef __TASK_LIST_H__
#define __TASK_LIST_H__

#include "ak.h"
#include "message.h"

#include "app_data.h"

#include "if_console.h"
#include "if_rf24.h"
#include "if_rf433.h"
#include "if_app.h"
#include "if_cpu_serial.h"
#include "if_usb_stick_rf24.h"

#include "task_console.h"
#include "task_if.h"
#include "task_debug_msg.h"
#include "task_sensor.h"
#include "task_sys.h"
#include "task_fw.h"

#include "link_phy.h"
#include "link_mac.h"
#include "link.h"

/** default if_des_type when get pool memory
 * this define MUST BE coresponding with app.
 */
#define AK_APP_TYPE_IF						IF_TYPE_APP_GW

enum {
	/* SYSTEM TASKS */
	AK_TASK_TIMER_ID,

	/* APP TASKS */
	GW_TASK_IF_CONSOLE_ID,
	GW_TASK_CONSOLE_ID,
	GW_TASK_IF_ID,
	GW_TASK_DEBUG_MSG_ID,
	GW_TASK_IF_APP_ID,
	GW_TASK_IF_CPU_SERIAL_ID,
	GW_TASK_SENSOR_ID,
	GW_TASK_SYS_ID,
	GW_TASK_FW_ID,
	GW_TASK_IF_USB_STICK_RF24_ID,

	/* LINK TASKS */
	GW_LINK_PHY_ID,
	GW_LINK_MAC_ID,
	GW_LINK_ID,

	/* EOT task ID */
	AK_TASK_LIST_LEN,
};

extern ak_task_t task_list[];

#endif //__TASK_LIST_H__
