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

    Rockchip_OSAL_Strcpy(rockchipComponents[0]->componentName, RK_OMX_COMPONENT_H264_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[0]->roles[0], RK_OMX_COMPONENT_H264_DEC_ROLE);
    rockchipComponents[0]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[1]->componentName, RK_OMX_COMPONENT_MPEG4_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[1]->roles[0], RK_OMX_COMPONENT_MPEG4_DEC_ROLE);
    rockchipComponents[1]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[2]->componentName, RK_OMX_COMPONENT_H263_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[2]->roles[0], RK_OMX_COMPONENT_H263_DEC_ROLE);
    rockchipComponents[2]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[3]->componentName, RK_OMX_COMPONENT_FLV_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[3]->roles[0], RK_OMX_COMPONENT_FLV_DEC_ROLE);
    rockchipComponents[3]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[4]->componentName, RK_OMX_COMPONENT_MPEG2_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[4]->roles[0], RK_OMX_COMPONENT_MPEG2_DEC_ROLE);
    rockchipComponents[4]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[5]->componentName, RK_OMX_COMPONENT_RMVB_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[5]->roles[0], RK_OMX_COMPONENT_RMVB_DEC_ROLE);
    rockchipComponents[5]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;


    Rockchip_OSAL_Strcpy(rockchipComponents[6]->componentName, RK_OMX_COMPONENT_VP8_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[6]->roles[0], RK_OMX_COMPONENT_VP8_DEC_ROLE);
    rockchipComponents[6]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[7]->componentName, RK_OMX_COMPONENT_VC1_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[7]->roles[0], RK_OMX_COMPONENT_VC1_DEC_ROLE);
    rockchipComponents[7]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[8]->componentName, RK_OMX_COMPONENT_WMV3_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[8]->roles[0], RK_OMX_COMPONENT_WMV3_DEC_ROLE);
    rockchipComponents[8]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[9]->componentName, RK_OMX_COMPONENT_VP6_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[9]->roles[0], RK_OMX_COMPONENT_VP6_DEC_ROLE);
    rockchipComponents[9]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[10]->componentName, RK_OMX_COMPONENT_HEVC_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[10]->roles[0], RK_OMX_COMPONENT_HEVC_DEC_ROLE);
    rockchipComponents[10]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[11]->componentName, RK_OMX_COMPONENT_MJPEG_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[11]->roles[0], RK_OMX_COMPONENT_MJPEG_DEC_ROLE);
    rockchipComponents[11]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[12]->componentName, RK_OMX_COMPONENT_H264_DRM_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[12]->roles[0], RK_OMX_COMPONENT_H264_DEC_ROLE);
    rockchipComponents[12]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[13]->componentName, RK_OMX_COMPONENT_VP9_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[13]->roles[0], RK_OMX_COMPONENT_VP9_DEC_ROLE);
    rockchipComponents[13]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[14]->componentName, RK_OMX_COMPONENT_HEVC_DRM_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[14]->roles[0], RK_OMX_COMPONENT_HEVC_DEC_ROLE);
    rockchipComponents[14]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[15]->componentName, RK_OMX_COMPONENT_MPEG4_DRM_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[15]->roles[0], RK_OMX_COMPONENT_MPEG4_DEC_ROLE);
    rockchipComponents[15]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[16]->componentName, RK_OMX_COMPONENT_MPEG2_DRM_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[16]->roles[0], RK_OMX_COMPONENT_MPEG2_DEC_ROLE);
    rockchipComponents[16]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[17]->componentName, RK_OMX_COMPONENT_VP9_DRM_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[17]->roles[0], RK_OMX_COMPONENT_VP9_DEC_ROLE);
    rockchipComponents[17]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

    Rockchip_OSAL_Strcpy(rockchipComponents[18]->componentName, RK_OMX_COMPONENT_VP8_DRM_DEC);
    Rockchip_OSAL_Strcpy(rockchipComponents[18]->roles[0], RK_OMX_COMPONENT_VP8_DEC_ROLE);
    rockchipComponents[18]->totalRoleNum = MAX_COMPONENT_ROLE_NUM;

EXIT:
    FunctionOut();

    return MAX_COMPONENT_NUM;
}
