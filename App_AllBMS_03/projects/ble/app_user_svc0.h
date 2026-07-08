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
 * @file app_user_svc0.h
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */

#include "n32wb43x.h"


#ifndef APP_USER_SVC0_H_
#define APP_USER_SVC0_H_

// att prop 
#define     PROP_READ                                    1 << 9
#define     PROP_WRITE_COMMNAD                           1 << 10
#define     PROP_WRITE_REQ                               1 << 11
#define     PROP_NTF                                     1 << 12
#define     PROP_IND                                     1 << 13

// ext att prop
#define     PROP_UUID_128                               1 << 14
#define     PROP_UUID_16                                0 << 13
#define     PROP_RI                                     1 << 15  

/// rdtss Service Attributes Indexes
enum
{
    RDTSS_IDX_SVC,
    
    RDTSS_IDX_WRITE_CHAR,
    RDTSS_IDX_WRITE_VAL,
    RDTSS_IDX_WRITE_CFG,
    
    RDTSS_IDX_NTF_CHAR,
    RDTSS_IDX_NTF_VAL,
    RDTSS_IDX_NTF_CFG,
    
    RDTSS_IDX_NB,
};

/// Internal 128bits UUID service description
struct attm_desc_128
{
    /// 128 bits UUID LSB First
    uint8_t uuid[0x10];
    /// Attribute Permissions (@see enum attm_perm_mask)
    uint16_t perm;
    /// Attribute Extended Permissions (@see enum attm_value_perm_mask)
    uint16_t ext_perm;
    /// Attribute Max Size
    /// note: for characteristic declaration contains handle offset
    /// note: for included service, contains target service handle
    uint16_t max_size;
};

void app_user_svc0_init(void);
void app_user_svc0_recv(uint8_t att_idx, char* data, uint16_t len);
void app_user_svc0_send(uint8_t* data, uint16_t len);

#endif ///APP_USER_SVC0_H_
