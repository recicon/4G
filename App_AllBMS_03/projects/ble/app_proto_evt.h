/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file app_proto_evt.h
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */

#ifndef APP_PROTO_EVT_H_
#define APP_PROTO_EVT_H_
#include "n32wb43x.h"

#define BLE_RECV_MAX 600

enum command_error_code
{
    CMD_PARSE_SUCCESS   = 0,
    CMD_HNADLER_ERROR,
    CMD_FRAME_ERROR,
    CMD_INVALID_ERROR,
    CMD_NULLFUN_ERROR,
};

//void app_evt_check(void);
void at_reset(void);
uint32_t blerecv_evt_hdl(char *p, uint16_t len);
uint32_t blesend_evt_hdl(char *p, uint16_t len);
uint32_t sleep_evt_hdl(char *p, uint16_t len);


uint32_t conn_evt_hdl(char *p, uint16_t len);
uint32_t disc_evt_hdl(char *p, uint16_t len);
uint32_t addsvc_evt_hdl(char *p, uint16_t len);
uint32_t baud_evt_hdl(char *p, uint16_t len);
uint32_t report_evt_hdl(char *p, uint16_t len);
uint32_t at_evt_hdl(char *p, uint16_t len);
uint32_t crc_evt_hdl(char *p, uint16_t len);
uint32_t scan_evt_hdl(char *p, uint16_t len);
uint32_t reset_evt_hdl(char *p, uint16_t len);
uint32_t error_evt_hdl(char *p, uint16_t len);
uint32_t tx_hdl_evt_hdl(char *p, uint16_t len);
uint32_t rx_hdl_evt_hdl(char *p, uint16_t len);
uint32_t mtu_ind_evt_hdl(char *p, uint16_t len);
uint32_t attrecv_evt_hdl(char *p, uint16_t len);
uint32_t sendack_evt_hdl(char *p, uint16_t len);
uint32_t conn_rssi_evt_hdl(char *p, uint16_t len);

extern uint8_t app_recv_val[BLE_RECV_MAX];
extern uint16_t app_recv_len; 

#endif ///APP_PROTO_EVT_H_

