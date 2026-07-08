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
 * @file app_proto_cmd.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#include "app_proto_cmd.h"
#include "app_proto_cmn.h"
#include "string.h"
#include "app_uart.h"
#include "app_io.h"
#include "app_user_config.h"

uint8_t   send_buf[256] ={0};
uint16_t  send_len = 0;

const uint8_t at_cmd_head[10] = {"AT+"};
const uint8_t AT_cmd_head_len = 3; 
const uint8_t at_cmd_end[10] = {"\r\n"};
const uint8_t AT_cmd_end_len = 2; 

void at_send_question(uint8_t cmd_idx)
{
    app_pullup_wakeup_io();
    
    while(!app_get_stauts_io());
    
    app_cmn_env.cur_cmd = cmd_idx;
    uint16_t fillin_pos = 0;
    memcpy(send_buf + fillin_pos, at_cmd_head, AT_cmd_head_len);
    fillin_pos += AT_cmd_head_len;

    memcpy(send_buf + fillin_pos, at_cmd_func[cmd_idx].cmd_str, at_cmd_func[cmd_idx].cmd_str_len);
 
    fillin_pos += at_cmd_func[cmd_idx].cmd_str_len;

    send_buf[fillin_pos] = '?';
    fillin_pos += 1;
     
    
    memcpy(send_buf + fillin_pos, at_cmd_end, AT_cmd_end_len);
    fillin_pos += AT_cmd_end_len;
   

    app_send_byte_array(send_buf, fillin_pos); 
    app_cmn_env.cmd_stat = AT_CMD_BUSY;
    
    app_pulldown_wakeup_io();
}


void at_send_cmd(uint8_t cmd_idx, char * cmd_data, uint16_t cmd_data_len)
{
    app_pullup_wakeup_io();
    
    while(!app_get_stauts_io());
    
    app_cmn_env.cur_cmd = cmd_idx;
    uint16_t fillin_pos = 0;
    memcpy(send_buf + fillin_pos, at_cmd_head, AT_cmd_head_len);
    fillin_pos += AT_cmd_head_len;
    memcpy(send_buf + fillin_pos, at_cmd_func[cmd_idx].cmd_str, at_cmd_func[cmd_idx].cmd_str_len);
    fillin_pos += at_cmd_func[cmd_idx].cmd_str_len;
    if(cmd_data_len)
    {    
        send_buf[fillin_pos] = '=';
        fillin_pos += 1;
        memcpy(send_buf + fillin_pos, cmd_data, cmd_data_len);
        fillin_pos += cmd_data_len;
    }     
    
    memcpy(send_buf + fillin_pos, at_cmd_end, AT_cmd_end_len);
    fillin_pos += AT_cmd_end_len;
   
    app_send_byte_array(send_buf, fillin_pos); 
    app_cmn_env.cmd_stat = AT_CMD_BUSY;
    
    app_pulldown_wakeup_io();
}


void at_start_scan(void)
{
    //start scan
    at_send_cmd(AT_CMD_SCAN, (void *)BLE_SCAN_EN, strlen(BLE_SCAN_EN));
}    

void at_stop_scan(void)
{
    at_send_cmd(AT_CMD_SCAN, (void *)BLE_SCAN_DIS, strlen(BLE_SCAN_DIS));
}    

void at_blesend(uint8_t * data, uint16_t len)
{
//    app_cmn_mst_env.tx_stat  = TX_STAT_BUSY;
//    static char send_data_char_buf[512];
//    byteArrayToCharArray(data, len, send_data_char_buf);
//    at_send_cmd(AT_CMD_BLESEND, send_data_char_buf, len*2);
extern	void app_user_svc0_send(uint8_t* data, uint16_t len);
    app_user_svc0_send(data, len);
}    
