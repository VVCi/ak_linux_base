#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ak.h"
#include "timer.h"

#include "app.h"
#include "app_if.h"
#include "app_dbg.h"
#include "app_data.h"

#include "task_list.h"
#include "task_sensor.h"

q_msg_t gw_task_sensor_mailbox;

void* gw_task_sensor_entry(void*) {
	wait_all_tasks_started();

	APP_DBG("[STARTED] gw_task_sensor_entry\n");

	ak_msg_t* msg;

	while (1) {

		/* get messge */
		msg = ak_msg_rev(GW_TASK_SENSOR_ID);

		switch (msg->header->sig) {

		default:
			break;
		}

		/* free message */
		ak_msg_free(msg);
	}

	return (void*)0;
}
