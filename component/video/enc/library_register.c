/*
 *
 * Copyright 2013 rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file    library_register.c
 * @brief
 * @author    Csy (csy@rock-chips.com)
 * @version    1.0.0
 * @history
 *   2013.11.27 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "Rockchip_OSAL_Memory.h"
#include "Rockchip_OSAL_ETC.h"
#include "library_register.h"
#include "Rockchip_OSAL_Log.h"


OSCL_EXPORT_REF int Rockchip_OMX_COMPONENT_Library_Register(RockchipRegisterComponentType **rockchipComponents)
{
    FunctionIn();

    if (rockchipComponents == NULL)
        goto EXIT;

    Rockchip_OSAL_Strcpy(rockchipComponents[0]->componentName, RK_OMX_COMPONENT_H264_ENC);
    Rockchip_OSAL_Strcpy(rockchipComponents[0]->roles[0], RK_OMX_COMPONENT_H264_ENC_ROLE);
    rockchipComponents[0]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[1]->componentName, RK_OMX_COMPONENT_VP8_ENC);
    Rockchip_OSAL_Strcpy(rockchipComponents[1]->roles[0], RK_OMX_COMPONENT_VP8_ENC_ROLE);
    rockchipComponents[1]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[2]->componentName, RK_OMX_COMPONENT_HEVC_ENC);
    Rockchip_OSAL_Strcpy(rockchipComponents[2]->roles[0], RK_OMX_COMPONENT_HEVC_ENC_ROLE);
    rockchipComponents[2]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;
EXIT:
    FunctionOut();

    return MAX_COMPONENT_NUM;
}
