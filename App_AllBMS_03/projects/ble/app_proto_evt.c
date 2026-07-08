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
 * @file app_proto_evt.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#include "app_proto_evt.h"
#include "app_proto_cmn.h"
#include "app_proto_cmd.h"

#include "string.h"
#include "app_user_config.h"
#include "app_uart.h"
#include "app_flash.h"
#include "app_user_svc0.h"
#include "n32wb43x_it.h"

uint8_t app_recv_val[BLE_RECV_MAX] = {0};
uint16_t app_recv_len = 0; 

#if(CFG_MASTER)
uint8_t found_dev_flag = 0;
char found_dev_mac[12] = {0};
#endif   

uint32_t blerecv_evt_hdl(char *p, uint16_t len)
{
    if(app_recv_len == 0)
    {
        charArrayToByteArray(p, len, app_recv_val);
        app_recv_len = len/2;
    } 
    
    return 0;
} 



uint32_t blesend_evt_hdl(char *p, uint16_t len)
{
    #if(CFG_MASTER)
    extern uint8_t send_idx;
    if(p[0] == '0')
    {
        send_idx++;
    }    
    app_cmn_mst_env.tx_stat = TX_STAT_IDLE;

    #else
    app_cmn_slv_env.tx_stat = TX_STAT_IDLE;
    #endif
    return 0;
}    

uint32_t sleep_evt_hdl(char *p, uint16_t len)
{
		return 0;
}    

uint32_t test_hdl(char *p, uint16_t len)
{
    return 0;
}    

uint32_t baud_evt_hdl(char *p, uint16_t len)
{
    return 0;
} 

uint32_t reset_evt_hdl(char *p, uint16_t len)
{
    return 0;
} 

uint32_t conn_evt_hdl(char *p, uint16_t len)
{
    #if(CFG_MASTER)
    app_cmn_mst_env.tx_stat = TX_STAT_IDLE;
    #else
    app_cmn_slv_env.tx_stat = TX_STAT_IDLE;
    #endif
    app_cmn_env.conn_stat = AT_STAT_CONN;
   
    return 0;
} 

uint32_t disc_evt_hdl(char *p, uint16_t len)
{
    app_cmn_env.conn_stat = AT_STAT_DISCONN;
    app_cmn_mst_env.tx_hdl = 0;
    app_cmn_mst_env.rx_hdl = 0;
    // app_timer_stop();

    return 0;
} 

uint32_t addsvc_evt_hdl(char *p, uint16_t len)
{
		return 0;
}    


uint32_t error_evt_hdl(char *p, uint16_t len)
{

    while(1);
//    return 0;
   
} 

uint32_t report_evt_hdl(char *p, uint16_t len)
{
    #if(CFG_MASTER)
    NS_LOG_DEBUG("report\r\n");
    uint16_t comma_pos =0;
    for( comma_pos = 0; comma_pos<len; comma_pos++)
    {
        if(p[comma_pos] == ',')
        {
           
            break;
        }    
    }
    NS_LOG_DEBUG("comma_pos =%x\r\n", comma_pos);
    NS_LOG_DEBUG("name :");
    for(int i = 0; i<comma_pos; i++)
    {
        NS_LOG_DEBUG("%c", p[i]);
    }
    NS_LOG_DEBUG("\r\n");
    
    NS_LOG_DEBUG("mac :");
    uint8_t mac_pos = comma_pos+1;
    for(int i = comma_pos+1; i<len; i++)
    {
        NS_LOG_DEBUG("%c", p[i]);
    }
    NS_LOG_DEBUG("\r\n");
    uint8_t name_len =  comma_pos;
    NS_LOG_DEBUG("name_len =%x, dst_len =%x\r\n", name_len, DST_DEV_NAME_LEN);
    if(name_len ==  DST_DEV_NAME_LEN)
    {
        if(strncmp((void *)p, (void *)DST_DEV_NAME, DST_DEV_NAME_LEN) == 0)
        {
            NS_LOG_DEBUG("found\r\n");
            at_stop_scan();
            found_dev_flag = 1;
            memcpy(found_dev_mac, &p[mac_pos], 12);
        }    
    }    
    #endif //CFG_MASTER
    return 0;
} 
uint32_t tx_hdl_evt_hdl(char *p, uint16_t len)
{
    #if(CFG_MASTER)
    delay_n_1ms(500); //TODO DEL
    charArrayToByteArray(p, 2, &app_cmn_mst_env.tx_hdl);
    NS_LOG_DEBUG("tx hdl = %x\r\n", app_cmn_mst_env.tx_hdl);
    app_timer_start();
    #endif //CFG_MASTER
    return 0;
} 

uint32_t mtu_ind_evt_hdl(char *p, uint16_t len)
{
    charArrayToByteArray(p, 4, (uint8_t *)&app_cmn_mst_env.mtu);
    return 0;
} 

uint32_t attrecv_evt_hdl(char *p, uint16_t len)
{
    uint8_t svc_idx = p[0] - '0';
    uint8_t att_idx = p[2] - '0';
    
    char * data = p+4;
    if(svc_idx == 0)
    {
        app_user_svc0_recv(att_idx, data, len-4);
    }    
    return 0;
} 
uint32_t sendack_evt_hdl(char *p, uint16_t len)
{
   
    #if(CFG_MASTER)
    extern uint8_t send_idx;
    if(p[0] == '0')
    {
        send_idx++;
    }    
    app_cmn_mst_env.tx_stat = TX_STAT_IDLE;
    #else 
    app_cmn_slv_env.tx_stat = TX_STAT_IDLE;  
    #endif
    return 0;
} 

uint32_t rx_hdl_evt_hdl(char *p, uint16_t len)
{
    charArrayToByteArray(p, 2, &app_cmn_mst_env.rx_hdl);

    return 0;
} 

uint32_t crc_evt_hdl(char *p, uint16_t len)
{
    extern uint16_t stored_crc;

    uint16_t ble_crc_value = 0;
    charArrayToByteArray(p, len, (void *)&ble_crc_value);

    if(app_cmn_env.cfg_step == AT_STAT_PARAM_CHECK)
    {

        if(stored_crc == ble_crc_value)
        {
            app_cmn_env.cfg_step = AT_STAT_PARAM_CHECK_DONE;
        } 
        else
        {
            at_send_cmd(AT_CMD_RESTORE, 0, 0);
            Delay_ms(200);

            app_cmn_env.cfg_step = AT_STAT_PARAM_CFG;
        }  
        return 0;
    } 
    else if(app_cmn_env.cfg_step == AT_STAT_PARAM_CFG)
    {
        ///store crc
        set_store_crc(ble_crc_value);
        app_cmn_env.cfg_step = AT_STAT_PARAM_CHECK_DONE;
        at_send_cmd(AT_CMD_RESET, 0, 0);
    }    
    return 0;
}    

uint32_t at_evt_hdl(char *p, uint16_t len)
{

    #if(CFG_MASTER)
    NS_LOG_DEBUG("P[0] =%x\r\n", p[0]);
    if(p[0] == '0')
    {
        if(found_dev_flag)
        {
            //start conn dev
            at_send_cmd(AT_CMD_AUTOCON, found_dev_mac, 12);
        }    
    }   
    #endif/// (CFG_MASTER)
    return 0;
}

uint32_t scan_evt_hdl(char *p, uint16_t len)
{
    #if(CFG_MASTER)    
    NS_LOG_DEBUG("scan_hdl\r\n");
    if(p[0] == '0')
    {
        if(found_dev_flag)
        {
            //start conn dev
            at_send_cmd(AT_CMD_AUTOCON, found_dev_mac, 12);
        }    
    }   
    #endif
    return 0;
}

uint32_t conn_rssi_evt_hdl(char *p, uint16_t len)
{
    return 0;
} 



