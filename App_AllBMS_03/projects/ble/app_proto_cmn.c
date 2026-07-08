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
 * @file app_proto_cmn.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#include "app_proto_cmn.h"
#include "app_proto_evt.h"
#include "app_proto_cmd.h"
#include "string.h"
#include "app_user_config.h"
#include "app_flash.h"
#include "stdio.h"


uint16_t stored_crc  = 0xFFFF;


struct app_cmn_env_tag app_cmn_env=
{
    .cmd_stat   = AT_CMD_BUSY,
    .cur_cmd    = 0,
    .cfg_step   = AT_STAT_PARAM_CHECK,
    .cfg_pos    = 0,
    .conn_stat =  AT_STAT_DISCONN,
};

struct app_cmn_mst_env_tag app_cmn_mst_env=
{    
    .mtu        = 23,
    .tx_stat    = 0,
    .tx_hdl     = 0,
    .rx_hdl     = 0,
};

struct app_cmn_slv_env_tag app_cmn_slv_env=
{    
    .mtu        = 23,
    .tx_stat    = 0,
};

struct app_cmn_env_tag app_cmn_env;

uint8_t ble_mac[MAC_NUM] = {0};
uint8_t ble_name[BLE_NAME_LEN] = {0};


const  AT_cmd_func at_cmd_func[AT_END]=
{
                                 /* cmd_ID             string                   0,  exe function   */
        [AT_CMD_TEST]=          {AT_CMD_TEST,         "",               0,      0,  at_evt_hdl,     NULL},                                                           
        [AT_CMD_INFO]=          {AT_CMD_INFO,         "INFO",           4,      0,  NULL,           NULL},     
        [AT_CMD_BAUD]=          {AT_CMD_BAUD,         "BAUD",           4,      0,  baud_evt_hdl,   NULL}, 
        [AT_CMD_PARITY]=        {AT_CMD_PARITY,       "PARITY",         6,      0,  NULL,           NULL},
            
        [AT_CMD_ROLE]=          {AT_CMD_ROLE,         "ROLE",           4,      0,  NULL,           CFG_ROLE},                
        [AT_CMD_HWCTL]=         {AT_CMD_HWCTL,        "HWCTL",          5,      0,  NULL,           NULL},           
        [AT_CMD_NAME]=          {AT_CMD_NAME,         "NAME",           4,      0,  NULL,           (char*)ble_name}, 
        [AT_CMD_MAC]=           {AT_CMD_MAC,          "MAC",            3,      0,  NULL,           (char*)ble_mac}, 
                                
        [AT_CMD_TXPOWER]=       {AT_CMD_TXPOWER,      "TXPOWER",        7,      0,  NULL,           CFG_TXPOWER}, 
        [AT_CMD_BLE_MTU]=       {AT_CMD_BLE_MTU,      "MTU_CFG",        7,      0,  NULL,           CFG_MTU}, 
        [AT_CMD_BLE_DLE]=       {AT_CMD_BLE_DLE,      "DLE_CFG",        7,      0,  NULL,           CFG_DLE}, 
        [AT_CMD_BLE_PHY]=       {AT_CMD_BLE_PHY,      "PHY_CFG",        7,      0,  NULL,           CFG_PHY}, 
                                                                                            
        [AT_CMD_PAIR]=          {AT_CMD_PAIR,         "PAIR",           4,      0,  NULL,           CFG_PAIR}, 
        [AT_CMD_PPW]=           {AT_CMD_PPW,          "PPW",            3,      0,  NULL,           CFG_PPW},                                                                   
        [AT_CMD_BONDCLEAN]=     {AT_CMD_BONDCLEAN,    "BONDCLEAN",      9,      0,  NULL,           NULL}, 
        [AT_CMD_AUTH]=          {AT_CMD_AUTH,         "AUTH",           4,      0,  NULL,           CFG_AUTH}, 
                                                                                            
        [AT_CMD_SYSID]=         {AT_CMD_SYSID,        "SYSID",          5,      0,  NULL,           CFG_SYSID}, 
        [AT_CMD_MODEL]=         {AT_CMD_MODEL,        "MODEL",          5,      0,  NULL,           CFG_SYSID}, 
        [AT_CMD_SERIAL]=        {AT_CMD_SERIAL,       "SERIAL",         6,      0,  NULL,           CFG_SERIAL}, 
        [AT_CMD_FWVER]=         {AT_CMD_FWVER,        "FWVER",          5,      0,  NULL,           CFG_FWVER}, 
                        
        [AT_CMD_HWVER]=         {AT_CMD_HWVER,        "HWVER",          5,      0,  NULL,           CFG_HWVER}, 
        [AT_CMD_MNNAME]=        {AT_CMD_MNNAME,       "MNNAME",         6,      0,  NULL,           CFG_MNNAME}, 
        [AT_CMD_BATEN]=         {AT_CMD_BATEN,        "BATEN",          5,      0,  NULL,           CFG_BATEN}, 
        [AT_CMD_BATVAL]=        {AT_CMD_BATVAL,       "BATVAL",         6,      0,  NULL,           CFG_BATVAL}, 
        [AT_CMD_HIDEN]=         {AT_CMD_HIDEN,         "HIDEN",          5,      0,  NULL,           CFG_HIDEN}, 
            
        [AT_CMD_SVCUID]=        {AT_CMD_SVCUID,       "SVCUID",         6,      0,  NULL,           CFG_SVCUID}, 
        [AT_CMD_TXUID]=         {AT_CMD_TXUID,        "TXUID",          5,      0,  NULL,           CFG_TXUID}, 
        [AT_CMD_RXUID]=         {AT_CMD_RXUID,        "RXUID",          5,      0,  NULL,           CFG_RXUID},                                                                    
        [AT_CMD_ADVEN]=         {AT_CMD_ADVEN,        "ADVEN",          5,      0,  NULL,           CFG_ADVEN}, 
                
        [AT_CMD_ADVINTV]=       {AT_CMD_ADVINTV,      "ADVINTV",        7,      0,  NULL,           CFG_ADVINTV}, 
        [AT_CMD_ADVDURA]=       {AT_CMD_ADVDURA,      "ADVDURA",        7,      0,  NULL,           CFG_ADVDURA}, 
        [AT_CMD_ADVDATA]=       {AT_CMD_ADVDATA,      "ADVDATA",        7,      0,  NULL,           CFG_ADVDATA}, 
        [AT_CMD_ADVRESP]=       {AT_CMD_ADVRESP,      "ADVRESP",        7,      0,  NULL,           CFG_ADVRESP}, 

        [AT_CMD_IUS_SVCUID]=   {AT_CMD_IUS_SVCUID,    "IUS_SVCUID",         10,      0,  NULL,           NULL}, 
        [AT_CMD_IUS_CCUID]=    {AT_CMD_IUS_CCUID,     "IUS_CCUID",          9,      0,  NULL,           NULL}, 
        [AT_CMD_IUS_RCUID]=    {AT_CMD_IUS_RCUID,     "IUS_RCUID",          9,      0,  NULL,           NULL},                                                                    

        [AT_CMD_MIUS_SVCUID]=   {AT_CMD_MIUS_SVCUID,  "MIUS_SVCUID",         11,      0,  NULL,           NULL}, 
        [AT_CMD_MIUS_CCUID]=    {AT_CMD_MIUS_CCUID,   "MIUS_CCUID",          10,      0,  NULL,           NULL}, 
        [AT_CMD_MIUS_RCUID]=    {AT_CMD_MIUS_RCUID,   "MIUS_RCUID",          10,      0,  NULL,           NULL},                                                                    
        [AT_CMD_RSSI]=          {AT_CMD_RSSI,         "RSSI",           4,      0,  NULL,   NULL},         
				
        [AT_CMD_IBCUID]=        {AT_CMD_IBCUID,       "IBCUID",         6,      0,  NULL,           NULL}, 
        [AT_CMD_IBCMAJOR]=      {AT_CMD_IBCMAJOR,     "IBCMAJOR",       8,      0,  NULL,           NULL}, 
        [AT_CMD_IBCMINOR]=      {AT_CMD_IBCMINOR,     "IBCMINOR",       8,      0,  NULL,           NULL}, 
        [AT_CMD_IBCRSSI]=       {AT_CMD_IBCRSSI,      "IBCRSSI",        7,      0,  NULL,           NULL}, 
            
        [AT_CMD_CONNINTV]=      {AT_CMD_CONNINTV,     "CONNINTV",       8,      0,  NULL,           CFG_CONNINTV}, 
        [AT_CMD_CONNLATE]=      {AT_CMD_CONNLATE,     "CONNLATE",       8,      0,  NULL,           CFG_CONNLATE}, 
        [AT_CMD_CONNTIMEOUT]=   {AT_CMD_CONNTIMEOUT,  "CONNTIMEOUT",    11,     0,  NULL,           CFG_CONNTIMEOUT},                                                                 
        [AT_CMD_SCAN]=          {AT_CMD_SCAN,         "SCAN",           4,      0,  scan_evt_hdl,   CFG_SCAN}, 
            
        [AT_CMD_CONN]=          {AT_CMD_CONN,         "CONN",           4,      0,  conn_evt_hdl,   NULL}, 
        [AT_CMD_DISC]=          {AT_CMD_DISC,         "DISC",           4,      0,  disc_evt_hdl,   NULL},         

        [AT_CMD_ADDSVC]=       {AT_CMD_ADDSVC,       "ADDSVC",         6,      0,  addsvc_evt_hdl, NULL},
        
        [AT_CMD_AUTOADV]=       {AT_CMD_AUTOADV,      "AUTOADV",        7,      0,  NULL,           CFG_AUTOADV}, 
        [AT_CMD_AUTOCON]=       {AT_CMD_AUTOCON,      "AUTOCON",        7,      0,  NULL,           CFG_AUTOCON}, 
                                
        [AT_CMD_BLESEND]=       {AT_CMD_BLESEND,      "BLESEND",        7,      0,  blesend_evt_hdl, NULL},                                                                 
        [AT_CMD_SLEEP]=         {AT_CMD_SLEEP,        "SLEEP",          5,      0,  sleep_evt_hdl,  NULL}, 
        [AT_CMD_RESTORE]=       {AT_CMD_RESTORE,      "RESTORE",        7,      0,  NULL,           NULL}, 
        [AT_CMD_RESET]=         {AT_CMD_RESET,        "RESET",          5,      0,  reset_evt_hdl,  NULL}, 
                                                                                        
        [AT_CMD_EXIT]=          {AT_CMD_EXIT,         "EXIT",           5,      0,  NULL,           NULL}, 
        [AT_EVT_BLERECV]=       {AT_EVT_BLERECV,      "BLERECV",        7,      0,  blerecv_evt_hdl, NULL},      
        [AT_EVT_ERROR]=         {AT_EVT_ERROR,        "ERROR",          5,      0,  error_evt_hdl,  NULL},
                    
        [AT_CMD_CRC]=           {AT_CMD_CRC,          "CRC",            3,      0,  crc_evt_hdl,    NULL},
                //report        
        [AT_EVT_REPORT]=        {AT_EVT_REPORT,       "REPORT",         6,      0,  report_evt_hdl, NULL},
        [AT_EVT_RX_HDL]=        {AT_EVT_RX_HDL,       "RX_HDL",         6,      0,  rx_hdl_evt_hdl, NULL},
        [AT_EVT_TX_HDL]=        {AT_EVT_TX_HDL,       "TX_HDL",         6,      0,  tx_hdl_evt_hdl, NULL},
            
        [AT_EVT_MTU_IND]=       {AT_EVT_MTU_IND,      "MTU_IND",        7,      0,  mtu_ind_evt_hdl, NULL},
        [AT_CMD_ATTSEND]=       {AT_CMD_ATTSEND,      "ATTSEND",        7,      0,  NULL,            NULL},
        [AT_EVT_ATTRECV]=       {AT_EVT_ATTRECV,      "ATTRCV",         6,      0,  attrecv_evt_hdl, NULL},
        [AT_EVT_SENDACK]=       {AT_EVT_SENDACK,      "SENDACK",        7,      0,  sendack_evt_hdl, NULL},
                          
        //[AT_END]=                {AT_END,              NULL,             0,      0,  NULL,           NULL}, 
};  


uint8_t asc2_to_byte1(uint8_t  byte1, uint8_t byte2)
{
    uint8_t ret_val = 0;
    if(byte1<='9')
    {
       ret_val =  (byte1  - '0');
    } 
    else if(byte1<='F')
    {
        ret_val =  (byte1  - 'A') + 10;
    } 
    else if(byte1<='f')   
    {
        ret_val =  (byte1  - 'a') + 10;
    }     

    ret_val = ret_val <<4;
    if(byte2<='9')
    {
       ret_val +=  (byte2  - '0');
    } 
    else if(byte2<='F')
    {
        ret_val +=  (byte2  - 'A') + 10;
    } 
    else if(byte2<='f')   
    {
        ret_val +=  (byte2  - 'a') + 10;
    }   
    return ret_val;
}
void charArrayToByteArray(char *charArray, int size, uint8_t *byteArray)
{
    for(int i = 0; i<size/2; i++)
    {
        byteArray[i] =asc2_to_byte1(charArray[i * 2] , charArray[i * 2 +1]);
    }
}
void byteArrayToCharArray(uint8_t *byteArray, int size, char *charArray) 
{
    for (int i = 0; i < size; i++) {
        sprintf((void *)&charArray[i * 2], "%02x", byteArray[i]);
    }
}

void app_start_check_param_crc(void)
{
    if(app_cmn_env.cmd_stat == AT_CMD_IDLE)
    { 
        /// obtain store crc
        stored_crc = get_store_crc();
        if(stored_crc == 0xFFFF)
        {
            app_cmn_env.cfg_step = AT_STAT_PARAM_CFG;
            return;
        }      
        
        /// obtain ble crc
        at_send_cmd(at_cmd_func[AT_CMD_CRC].cmd_idx, 0,  0);
    }
}    

void app_start_cfg_param(void)
{  
    if(app_cmn_env.cmd_stat == AT_CMD_IDLE)
    {    
        for(int i = app_cmn_env.cfg_pos; i<AT_END; i++ )
        {
            app_cmn_env.cfg_pos++;
            if(i == AT_CMD_ADDSVC)
            {
							extern void app_user_svc0_init(void);
                app_user_svc0_init();
                break;
            }    
						if(i == AT_CMD_TEST)
            {
                at_send_cmd(AT_CMD_RESTORE, 0, 0);
                break;
            }    
            if( (at_cmd_func[i].init_val != NULL) && (at_cmd_func[i].cmd_str != NULL))
            {
                at_send_cmd(at_cmd_func[i].cmd_idx, at_cmd_func[i].init_val,  strlen(at_cmd_func[i].init_val));
                break;
            }    
        }
    
        if(app_cmn_env.cfg_pos >= AT_END)
        {
            //obtain ble cfg param crc
            at_send_cmd(at_cmd_func[AT_CMD_CRC].cmd_idx, 0,  0);

        }
    }   
} 



uint8_t at_evt_str_search(char *p, uint16_t len)
{
    uint8_t cmd_idx = 0;
    for(uint8_t i=1; at_cmd_func[i].cmd_idx != AT_END; i++)
    {    
        if(!strncmp(p, at_cmd_func[i].cmd_str, at_cmd_func[i].cmd_str_len))
        {
            cmd_idx = i;
            break;
        }        
    }
    return cmd_idx;
}


void app_at_parser(uint8_t byte)
{
    static char handle_data[512];
    static uint16_t handle_data_len = 0;
    handle_data[handle_data_len] = byte;
    handle_data_len ++;
    
    if((handle_data[handle_data_len-1] == '\n') && (handle_data[handle_data_len-2]  == '\r'))
    {
        /// handle this line
        uint8_t pos =0;
        while(pos < handle_data_len -2)
        {
            if(handle_data[pos] == '+')
            {
                uint8_t equal_pos = pos;
                uint8_t equal_found = 0;;
                for(; equal_pos< handle_data_len -2; equal_pos++)
                {
                    if(handle_data[equal_pos] == '=')
                    {
                        equal_found = 1;
                        break;
                    }    
                }
               
                uint8_t evt_id =  at_evt_str_search(&handle_data[pos+1], handle_data_len - pos - 2);
                if(equal_found )
                {
                   
                    if(at_cmd_func[evt_id].evt_handler != NULL)
                    {
                        uint16_t param_len = handle_data_len - 2 - equal_pos - 1;
                        at_cmd_func[evt_id].evt_handler(&handle_data[equal_pos+1], param_len); 
                    }    
                }    
                memset(handle_data, 0, handle_data_len);
                handle_data_len = 0;              
                return;
            }
            else if( (handle_data[pos] == 'A') && (handle_data[pos+1] == 'T') )
            {
                app_cmn_env.cmd_stat = AT_CMD_IDLE;

                at_cmd_func[AT_CMD_TEST].evt_handler(NULL, 0); 
                memset(handle_data, 0, handle_data_len);
                handle_data_len = 0;
                return;
            }
            else if( (handle_data[pos] == 'O' && handle_data[pos+1] == 'K') )
            {
                app_cmn_env.cmd_stat = AT_CMD_IDLE;
                memset(handle_data, 0, handle_data_len);    
                handle_data_len = 0;    
                app_cmn_env.cur_cmd = 0;
                return;
            }
            else
            {
                pos++;
            }    
        } 
        memset(handle_data, 0, handle_data_len);
        handle_data_len = 0;   
    }
}

