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
 * @file app_user_config.h
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#ifndef _APP_USER_CONFIG_H_
#define _APP_USER_CONFIG_H_

/* Public typedef -----------------------------------------------------------*/


#define DEFAULT_CFG 

///CFG_ROLE 
#define BLE_ROLE_SLAVE          "0"
#define BLE_ROLE_MASTER         "1"
#define BLE_ROLE_BEACON         "2"

///CFG_TXPOWER
#define TX_POWER_0_DBM          "0"
#define TX_POWER_Neg2_DBM       "1"
#define TX_POWER_Neg4_DBM       "2"
#define TX_POWER_Neg8_DBM       "3"
#define TX_POWER_Neg15_DBM      "4"
#define TX_POWER_Neg20_DBM      "5"
#define TX_POWER_Pos2_DBM       "6"
#define TX_POWER_Pos3_DBM       "7"
#define TX_POWER_Pos4_DBM       "8"
#define TX_POWER_Pos6_DBM       "9"

///CFG_DLE 
#define BLE_DLE_ON              "1"
#define BLE_DLE_OFF             "0"

///CFG_PHY
#define BLE_PHY_UNCHANGE        "0"
#define BLE_PHY_1M              "1"
#define BLE_PHY_2M              "2"
#define BLE_PHY_CODE_125K       "3"
#define BLE_PHY_CODE_500K       "4"

///CFG_PAIR 
#define BLE_PAIR_NO             "0"
#define BLE_PAIR_JUSTWORK       "1"
#define BLE_PAIR_PASSKEY        "2"

///CFG_ADVEN           
#define BLE_ADV_EN              "1"
#define BLE_ADV_DIS             "0"

///CFG_SCANEN
#define BLE_SCAN_EN             "1"
#define BLE_SCAN_DIS            "0"


//CFG_AUTOADV
#define BLE_AUTOADV_EN           "1"
#define BLE_AUTOADV_DIS          "0"

//CFG_AUTOCONN
#define BLE_AUTOCONN_EN           "1"
#define BLE_AUTOCONN_DIS          "0"

#define CFG_SLAVE                 1 
//#define CFG_MASTER                 1 

#if(CFG_MASTER)
#define CFG_DEV_NAME                        //"ble_master"
#define CFG_DEV_MAC                         "015633223301"
#define CFG_MTU                             "0"//"128"                /*mtu [23-255]*/
#define CFG_ROLE                            BLE_ROLE_MASTER              
#define CFG_TXPOWER                         TX_POWER_Pos6_DBM
                    
#define CFG_DLE                             BLE_DLE_OFF
#define CFG_PHY                             BLE_PHY_UNCHANGE
                    
#define CFG_PAIR                            BLE_PAIR_NO //BLE_PAIR_JUSTWORK
#define CFG_PPW                             DEFAULT_CFG//"123456"            /*when set AT+PAIR=BLE_PAIR_PASSKEY work*/
#define CFG_AUTH                            "0"
                    
#define CFG_SYSID                           //"123457"
#define CFG_MODEL                           //"MD_123457"  
#define CFG_SERIAL                          //"SN_100001"    
#define CFG_FWVER                           //"0.1"
#define CFG_HWVER                           //"0.1"
#define CFG_MNNAME                          //"NationS"
    
#define CFG_BATEN                           //"1"               
#define CFG_BATVAL                          //"80"

#define CFG_SVCUID                          "FEC0"
#define CFG_TXUID                           "FEC1"
#define CFG_RXUID                           "FEC2"
    
#define CFG_ADVEN                           //BLE_ADV_DIS          
#define CFG_ADVINTV                         //"150"
#define CFG_ADVDURA                         //DEFAULT_CFG
#define CFG_ADVDATA                         //"03190000"
#define CFG_ADVRESP                         //DEFAULT_CFG

#define CFG_CONNINTV                        //"40"
#define CFG_CONNLATE                        //"0"
#define CFG_CONNTIMEOUT                     //"600"    
        
#define CFG_SCAN                            BLE_SCAN_DIS                     

#define CFG_AUTOADV                         //BLE_AUTOADV_EN
#define CFG_AUTOCON                         "012233443301"//"112233443311"//BLE_AUTOADV_DIS

#define DST_DEV_NAME                         "123456" //"NS_AT_module"
#define DST_DEV_NAME_LEN                     6 //12

#elif (CFG_SLAVE)
#define CFG_DEV_NAME                        "HTBMS"
#define CFG_DEV_MAC                         "104466664401"
#define CFG_MTU                             "255"                /*mtu [23-255]*/
#define CFG_ROLE                            BLE_ROLE_SLAVE         
#define CFG_TXPOWER                         TX_POWER_Pos6_DBM
                    
#define CFG_DLE                             BLE_DLE_ON
#define CFG_PHY                             BLE_PHY_UNCHANGE
                    
#define CFG_PAIR                            BLE_PAIR_NO//BLE_PAIR_JUSTWORK
#define CFG_PPW                             DEFAULT_CFG//"123456"            /*when set AT+PAIR=BLE_PAIR_PASSKEY work*/
#define CFG_AUTH                            "0"
                    
#define CFG_SYSID                           "123457"
#define CFG_MODEL                           "MD_123457"  
#define CFG_SERIAL                          "SN_100001"    
#define CFG_FWVER                           "0.1"
#define CFG_HWVER                           "0.1"
#define CFG_MNNAME                          "NationS"
    
#define CFG_BATEN                           "1"               
#define CFG_BATVAL                          "80"

#define CFG_HIDEN                           "0"

#define CFG_SVCUID                          "FEC0"
#define CFG_TXUID                           "FEC1"
#define CFG_RXUID                           "FEC2"
    
#define CFG_ADVEN                           BLE_ADV_EN//BLE_ADV_DIS          
#define CFG_ADVINTV                         "100"
#define CFG_ADVDURA                         DEFAULT_CFG
#define CFG_ADVDATA                         "03190000"
#define CFG_ADVRESP                         DEFAULT_CFG

#define CFG_CONNINTV                        "40"
#define CFG_CONNLATE                        "0"
#define CFG_CONNTIMEOUT                     "600"    
        
#define CFG_SCAN                            NULL//BLE_SCAN_DIS                     

#define CFG_AUTOADV                         BLE_AUTOADV_EN
#define CFG_AUTOCON                         NULL//BLE_AUTOADV_DIS
#endif 

#endif // _APP_USER_CONFIG_H_

