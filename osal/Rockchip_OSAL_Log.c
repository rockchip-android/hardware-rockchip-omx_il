/*
 *
 * Copyright 2013 Rockchip Electronics Co. LTD
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
 * @file        Rockchip_OSAL_Log.h
 * @brief
 * @author      csy(csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2013.11.26 : Create
 */


#include <utils/Log.h>

#include "Rockchip_OSAL_Log.h"
#include <cutils/properties.h>
#include <string.h>

void _Rockchip_OSAL_Log(ROCKCHIP_LOG_LEVEL logLevel, const char *tag, const char *msg, ...)
{
    va_list argptr;

    va_start(argptr, msg);

    switch (logLevel) {
    case ROCKCHIP_LOG_TRACE: {
#ifdef ROCKCHIP_TRACE
        __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, argptr);
#else
        char value[PROPERTY_VALUE_MAX];
        if (property_get("dump_omx_log", value, NULL)
            && (!strcmp("1", value) || !strcasecmp("true", value))) {
            __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, argptr);
        }
#endif
    }
    break;
    case ROCKCHIP_LOG_DEBUG:
#ifdef ROCKCHIP_DEBUG
        __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, argptr);
#endif
        break;
    case ROCKCHIP_LOG_INFO:
//#ifdef ROCKCHIP_INFO
        __android_log_vprint(ANDROID_LOG_INFO, tag, msg, argptr);
//#endif
        break;
    case ROCKCHIP_LOG_WARNING:
#ifdef ROCKCHIP_WARN
        __android_log_vprint(ANDROID_LOG_WARN, tag, msg, argptr);
#endif
        break;
    case ROCKCHIP_LOG_ERROR:
        __android_log_vprint(ANDROID_LOG_ERROR, tag, msg, argptr);
        break;
    default:
        __android_log_vprint(ANDROID_LOG_VERBOSE, tag, msg, argptr);
    }

    va_end(argptr);
}
