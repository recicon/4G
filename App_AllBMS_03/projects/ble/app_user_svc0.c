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
 * @file app_user_svc0.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#include "app_user_svc0.h"
#include "app_proto_cmn.h"
#include "app_proto_cmd.h"
#include "stdio.h"

#define ATT_SERVICE_AM_SPEED_128          {0x01,0x10,0x2E,0xC7,0x8a,0x0E,  0x73,0x90,  0xE1,0x11,  0xC2,0x08,  0x60,0x27,0x00,0x00}     /*!< Service UUID */
#define ATT_CHAR_AM_SPEED_WRITE_128       {0xFB,0x34,0x9B,0x5F,0x80,0x0E,  0x00,0x80,  0x00,0x10,  0x00,0x00,  0xE1,0xFF,0x00,0x00}     /*!< Characteristic value UUID */
#define ATT_CHAR_AM_SPEED_NTF_128         {0xFB,0x34,0x9B,0x5F,0x80,0x0E,  0x00,0x80,  0x00,0x10,  0x00,0x00,  0xE2,0xFF,0x00,0x00}     /*!< Characteristic value UUID */

                                                 

// att prop 
#define     PROP_READ                                    1 << 9
#define     PROP_WRITE_CMD                               1 << 10
#define     PROP_WRITE_REQ                               1 << 11
#define     PROP_NTF                                     1 << 12
#define     PROP_IND                                     1 << 13

// ext att prop
#define     PROP_UUID_128                               1 << 14
#define     PROP_UUID_16                                0 << 13
#define     PROP_RI                                     1 << 15  


const struct attm_desc_128 app_user_att_db[RDTSS_IDX_NB] =
{
    /* Service Declaration */
    [RDTSS_IDX_SVC]         = {ATT_SERVICE_AM_SPEED_128,    PROP_READ,                          PROP_UUID_128,              0},
    
    /* Characteristic Declaration */    
    [RDTSS_IDX_WRITE_CHAR]  = {{0x03,0x28},                 PROP_READ | PROP_WRITE_REQ,         0,                          0},
    /* Characteristic Value */  
    [RDTSS_IDX_WRITE_VAL]   = {ATT_CHAR_AM_SPEED_WRITE_128, PROP_WRITE_CMD,                     PROP_RI | PROP_UUID_128,    251},
    /* Client Characteristic Configuration Descriptor */    
    [RDTSS_IDX_WRITE_CFG]   = {{0x01,0x29},                 PROP_READ | PROP_WRITE_REQ,         PROP_RI,                    20},
    
    /* Characteristic Declaration */    
    [RDTSS_IDX_NTF_CHAR]    = {{0x03,0x28},                 PROP_READ | PROP_WRITE_REQ,         0,                          0},
    /* Characteristic Value */  
    [RDTSS_IDX_NTF_VAL]     = {ATT_CHAR_AM_SPEED_NTF_128,   PROP_NTF,                           PROP_RI| PROP_UUID_128,     251},   
    /* Client Characteristic Configuration Descriptor */    
    [RDTSS_IDX_NTF_CFG]     = {{0x02,0x29},                 PROP_READ |PROP_WRITE_REQ,          PROP_RI,                    20},
};  




void app_user_svc0_init(void)
{
    static char send_data_char_buf[512];
    send_data_char_buf[0] = '0';
    send_data_char_buf[1] = ',';
    uint8_t *   data = (void *)&app_user_att_db[0].uuid[0];
    uint16_t data_len = sizeof(app_user_att_db);
    
    byteArrayToCharArray(data, data_len, send_data_char_buf+2);
    at_send_cmd(AT_CMD_ADDSVC, send_data_char_buf, data_len*2 +2);
}    



extern uint8_t app_recv_val[512];
extern uint16_t app_recv_len; 
void app_user_svc0_recv(uint8_t att_idx, char* data, uint16_t len)
{

    if(att_idx == RDTSS_IDX_WRITE_VAL)
    {    
        charArrayToByteArray(data, len, (uint8_t *)&app_recv_val[0]);
        app_recv_len = len/2;
    }    
}    


void app_user_svc0_send(uint8_t* data, uint16_t len)
{
    static char user_send_char_buf[512];
    user_send_char_buf[0] = '0';
    user_send_char_buf[1] = ',';
    user_send_char_buf[2] = '0'+ RDTSS_IDX_NTF_VAL;
    user_send_char_buf[3] = ',';;    
    byteArrayToCharArray(data, len, &user_send_char_buf[4]);

    at_send_cmd(AT_CMD_ATTSEND, user_send_char_buf, len*2 + 4);
}    
