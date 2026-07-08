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
 * @file app_proto_cmn.h
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */

#ifndef APP_PROTO_CMN_H_
#define APP_PROTO_CMN_H_
#include "n32wb43x.h"

#define AT_CMD_IDLE   0
#define AT_CMD_BUSY   1


#define AT_STAT_PARAM_CHECK         0
#define AT_STAT_PARAM_CFG           1
#define AT_STAT_PARAM_CHECK_DONE    2

#define AT_STAT_DISCONN             0
#define AT_STAT_CONN                1

#define BLE_NAME_LEN                20
#define MAC_NUM                     13

struct app_cmn_env_tag
{
   uint8_t cmd_stat;
   uint8_t cur_cmd;
   uint8_t cfg_step;
   uint8_t cfg_pos;
   uint8_t conn_stat;
};  

#define TX_STAT_IDLE                0
#define TX_STAT_BUSY                1

struct app_cmn_mst_env_tag
{
    uint16_t mtu;
    uint8_t tx_stat;
    uint8_t rx_hdl;
    uint8_t tx_hdl; 
};


struct app_cmn_slv_env_tag
{
    uint16_t mtu;
    uint8_t tx_stat;
};


typedef enum{
    AT_CMD_TEST = 0,     /* AT */
    AT_CMD_INFO,  
    AT_CMD_BAUD,         /* uart baud rate*/
    AT_CMD_PARITY,       /* uart parity  */
    
    AT_CMD_ROLE ,//4     /* ble role(master or slave) */  
    AT_CMD_HWCTL,        /* uart hardware control */    
    AT_CMD_NAME,         /* Set name */
    AT_CMD_MAC,

    
    AT_CMD_TXPOWER, //8  /* rf tx power */    
    AT_CMD_BLE_MTU,      /* set ble mtu */
    AT_CMD_BLE_DLE,      /* enable ble DLE mode */
    AT_CMD_BLE_PHY,      /* set ble phy */   
    
    AT_CMD_PAIR, //12    /* pair mode */
    AT_CMD_PPW,          /* pair pin */
    AT_CMD_BONDCLEAN,    /* bond_clean */    
    AT_CMD_AUTH,         /* air pass word */
    
    AT_CMD_SYSID,//16    /* system ID */
    AT_CMD_MODEL,        /* moudle id*/
    AT_CMD_SERIAL,       /* serial */
    AT_CMD_FWVER,        /* firmware version */
    
    AT_CMD_HWVER, //20   /* hardware version */
    AT_CMD_MNNAME,       /* mannufacturer name */   
    AT_CMD_BATEN,        /* enbale battery server*/
    AT_CMD_BATVAL,       /* battery server value*/  
    
		AT_CMD_HIDEN,	//24	 /* enbale hid server*/
    AT_CMD_SVCUID,       /* server_uuid */
    AT_CMD_TXUID,        /* tx uuid */
    AT_CMD_RXUID,        /* rx uuid */   
		
    AT_CMD_ADVEN, //28   /* adv enbale  */   
    AT_CMD_ADVINTV,      /* adv interval */    
    AT_CMD_ADVDURA,      /* adv duration */
    AT_CMD_ADVDATA,      /* adv data */
		
    AT_CMD_ADVRESP,//32  /* adv respond data */   
		AT_CMD_IUS_SVCUID,   /* ota server uuid for ble upgrade */
		AT_CMD_IUS_CCUID,
		AT_CMD_IUS_RCUID,  
		
		AT_CMD_MIUS_SVCUID, //36 /* ota server uuid for MCU upgrade */
		AT_CMD_MIUS_CCUID,
		AT_CMD_MIUS_RCUID,
		AT_CMD_RSSI,	

    AT_CMD_IBCUID, //40  /* ibeacon uuid */
    AT_CMD_IBCMAJOR,     /* ibeacon major */
    AT_CMD_IBCMINOR,     /* ibeacon minor */
    AT_CMD_IBCRSSI,      /* ibeacon RSSI */
    
    AT_CMD_CONNINTV,//44 /* connect interval */
    AT_CMD_CONNLATE,     /* connect_latency */
    AT_CMD_CONNTIMEOUT,  /* connect_timeout */   
    AT_CMD_SCAN,         /* start or stop ble scan */
    
    AT_CMD_CONN,   //48  /* start connect ble*/
    AT_CMD_DISC,         /* disconnect ble  */    
    AT_CMD_ADDSVC,       /* add new private service  */
    AT_CMD_AUTOADV,
    
    AT_CMD_AUTOCON, //52    
    AT_CMD_BLESEND,      /* send ble data */
    AT_CMD_SLEEP,        /* force enter sleep mode */
    AT_CMD_RESTORE,      /* reset to factory mode */   
		
    AT_CMD_RESET,   //56 /* reset module */       
    AT_CMD_EXIT,         /* exit AT mode to passthough*/   		
    AT_EVT_BLERECV,		
    AT_EVT_ERROR,      
		
    AT_CMD_CRC,   //60 		
    AT_EVT_REPORT,
    AT_EVT_RX_HDL,
    AT_EVT_TX_HDL,  	
		
    AT_EVT_MTU_IND, 	//64		
    AT_EVT_ATTRECV,
    AT_CMD_ATTSEND,
    AT_EVT_SENDACK,   
		
    AT_EVT_PHY_IND,  //68  
    AT_EVT_DLE_IND,  	
    AT_EVT_PARAM_IND,  
    AT_CMD_FLASH_WR, 
		
    AT_CMD_FLASH_RD, //72 
		AT_CMD_DTM_2W,
		AT_CMD_DTM_HCI,    
    AT_END,
}AT_Cmd;


typedef struct {
    AT_Cmd cmd_idx;       /* cmd id */
    char* cmd_str;        /* cmd string */
    uint8_t cmd_str_len;  /* cmd string length*/
    uint8_t cmd_type;     /* cmd prop tyep*/
    uint32_t (*evt_handler)(char *ptr, uint16_t len);  /* Exe cmd handler */
    char * init_val;      /* initial value */
}AT_cmd_func;

extern uint8_t ble_mac[MAC_NUM];
extern uint8_t ble_name[BLE_NAME_LEN];

extern const  AT_cmd_func at_cmd_func[];
extern struct app_cmn_env_tag app_cmn_env;
extern struct app_cmn_mst_env_tag app_cmn_mst_env;
extern struct app_cmn_slv_env_tag app_cmn_slv_env;
extern struct ble_params_t ble_info;

uint8_t asc2_to_byte1(uint8_t  byte1, uint8_t byte2);
void charArrayToByteArray(char *charArray, int size, uint8_t *byteArray);
void byteArrayToCharArray(uint8_t *byteArray, int size, char *charArray) ;
void app_at_parser(uint8_t byte);
void app_start_check_param_crc(void);
void app_start_cfg_param(void);  

#endif ///APP_PROTO_CMN_H_

