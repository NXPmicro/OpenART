/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BOARD_FLASH__
#define __BOARD_FLASH__


#define NOR_CMD_LUT_SEQ_IDX_READ_NORMAL        0
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST          1
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD     2
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS         3
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE        4
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR        5
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_SINGLE 6
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD   7
#define NOR_CMD_LUT_SEQ_IDX_READID             8
#define NOR_CMD_LUT_SEQ_IDX_WRITESTATUSREG     9
#define NOR_CMD_LUT_SEQ_IDX_ENTERQPI           10
#define NOR_CMD_LUT_SEQ_IDX_EXITQPI            11
#define NOR_CMD_LUT_SEQ_IDX_READSTATUSREG      12
#define NOR_CMD_LUT_SEQ_IDX_ERASECHIP          13

#define CUSTOM_LUT_LENGTH 60
#define FLASH_QUAD_ENABLE 0x40
#define FLASH_BUSY_STATUS_POL 1
#define FLASH_BUSY_STATUS_OFFSET 0
#define FLASH_ERROR_STATUS_MASK 0x0e
#define FLASH_ADDR_LENGTH_28 (24)

#endif