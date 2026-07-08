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
#include "main.h"
#include "app_uart.h"
#include "app_proto_evt.h"
#include "app_proto_cmn.h"
#include "app_proto_cmd.h"
#include "app_user_config.h"

#define GLOBAL_INT_DISABLE()        \
uint32_t ui32IntStatus = 0;         \
do{                                 \
    ui32IntStatus = __get_PRIMASK();\
    __set_PRIMASK(1);               \
}while(0)

#define GLOBAL_INT_RESTORE()     \
do{                              \
    __set_PRIMASK(ui32IntStatus);\
}while(0)



uint8_t get_fixed_keys(const uint8_t* pInData,uint8_t byInStrLen,char* pOutStr,uint8_t byOutStrLen)
{
     char str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                      "abcdefghijklmnopqrstuvwxyz" \
                      "0123456789+-";
    
    uint8_t str_len = sizeof(str)-1;
    uint8_t i=0,j=byOutStrLen,k=0;
    uint32_t iTemp=0;
    for( i = 0;i < byInStrLen&&i<byOutStrLen*6/8; i++)
    {
        if(0==i%3)
        {
           iTemp = 0; 
        }
        
        iTemp |= pInData[i]<<((i%3)*8);
        
        if(2==i%3)
        {
            for(k=0;k<4;k++)
            {
                if(j>0)
                    pOutStr[--j]=  str[(iTemp>>(k*6))%str_len];
            }
        }        
    }
    return byOutStrLen;
}

uint8_t compressUID_XOR(const uint8_t uid[12], uint8_t result[6]) {
    for(int i = 0; i < 6; i++) {
        result[i] = uid[i] ^ uid[i+6];
    }
    return 6;
}

void get_bleinfo(void)
{
    uint8_t UID[UID_LENGTH] = {0};
    uint8_t mac[6] = {0};
    uint8_t name_add[16] = {0};
    uint8_t name_len = 0;
    
    GetUID(UID);
    compressUID_XOR(UID, mac); //异或成6字节
    byteArrayToCharArray(mac, 6, ble_mac);
    memcpy(ble_name, CFG_DEV_NAME, sizeof(CFG_DEV_NAME)-1); 
    name_len = get_fixed_keys(mac, 6, name_add, 8);
    memcpy(ble_name+(sizeof(CFG_DEV_NAME)-1), name_add, name_len); 
    memcpy((uint8_t*)&g_tBleName.g_byBleName,ble_name,MAX_BLE_NAME>BLE_NAME_LEN?BLE_NAME_LEN:MAX_BLE_NAME);
}

/**
 * @brief  ble initialization
 */
void cfg_ble(void)
{
    app_cmn_env.cfg_step = AT_STAT_PARAM_CFG;
    get_bleinfo();
    while(app_cmn_env.cfg_step != AT_STAT_PARAM_CHECK_DONE)
    {
        /// obtain one byte and parser
        if(get_unhdl_data_len())
        {
            app_at_parser(fetch_one_byte_data());
        }
        
//        if(app_cmn_env.cfg_step == AT_STAT_PARAM_CHECK)
//        {
//            app_start_check_param_crc();
//        }  
//        else if(app_cmn_env.cfg_step == AT_STAT_PARAM_CFG)
        {
            app_start_cfg_param();
        }    
    }    
}   

void ble_loop(void)
{
  
        if(get_unhdl_data_len())
        {
            app_at_parser(fetch_one_byte_data());
        }
            
        if(app_cmn_env.conn_stat == AT_STAT_CONN)
        {
            //check if recv data
            if(app_recv_len != 0)
            {          
                memcpy(&g_sBleRtxMsg.rbuf[g_sBleRtxMsg.r_index],app_recv_val,app_recv_len);
                g_sBleRtxMsg.r_index = app_recv_len;
                app_recv_len = 0;
            }   
        } 
   
}    



void ble_init(void)
{

    app_uart_init();
    
    app_cmn_env.cmd_stat = AT_CMD_BUSY;

    app_io_init();
    
    app_reset_ble(); 
        
    cfg_ble();

	

}

/*@}*/
