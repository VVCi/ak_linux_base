#ifndef __IF_CPU_SERIAL_H__
#define __IF_CPU_SERIAL_H__

#include <stdint.h>

#include "message.h"

#include "app.h"
#include "app_data.h"
#include "app_dbg.h"

extern q_msg_t gw_task_if_cpu_serial_mailbox;
extern void* gw_task_if_cpu_serial_entry(void*);

#endif //__IF_CPU_SERIAL_H__
