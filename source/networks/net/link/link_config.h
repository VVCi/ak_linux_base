#ifndef __LINK_CONFIG_H__
#define __LINK_CONFIG_H__

#include "ak.h"

#define LINK_PDU_BUF_SIZE			384
#define LINK_PDU_POOL_SIZE			8

#define LINK_PHY_FRAME_SEND_TO_INTERVAL		50 /* 50 ms */

#define LINK_PHY_FRAME_REV_TO_INTERVAL		25 /* 25 ms */

/* DEBUG */
#define LINK_DBG_SIG_EN		0
#define LINK_DBG_DATA_EN	0
#define LINK_DBG_EN			0

#if (LINK_DBG_SIG_EN == 1)
#define LINK_DBG_SIG(fmt, ...)       printf("-LSIG-> " fmt, ##__VA_ARGS__)
#else
#define LINK_DBG_SIG(fmt, ...)
#endif

#if (LINK_DBG_DATA_EN == 1)
#define LINK_DBG_DATA(fmt, ...)       printf(fmt, ##__VA_ARGS__)
#else
#define LINK_DBG_DATA(fmt, ...)
#endif

#if (LINK_DBG_EN == 1)
#define LINK_DBG(fmt, ...)       printf(fmt, ##__VA_ARGS__)
#else
#define LINK_DBG(fmt, ...)
#endif

#endif //__LINK_CONFIG_H__
