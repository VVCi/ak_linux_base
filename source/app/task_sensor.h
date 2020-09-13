#ifndef __TASK_SENSOR_H__
#define __TASK_SENSOR_H__

#include "message.h"

extern q_msg_t gw_task_sensor_mailbox;
extern void* gw_task_sensor_entry(void*);

#endif //__TASK_SENSOR_H__
