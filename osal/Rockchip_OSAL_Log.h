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


#ifndef ROCKCHIP_OSAL_LOG
#define ROCKCHIP_OSAL_LOG

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ROCKCHIP_LOG_OFF
#define ROCKCHIP_LOG
#endif

#ifndef ROCKCHIP_LOG_TAG
#define ROCKCHIP_LOG_TAG    "ROCKCHIP_LOG"
#endif

///#define ROCKCHIP_TRACE_ON

#define ROCKCHIP_DEBUG_ON

#ifdef ROCKCHIP_TRACE_ON
#define ROCKCHIP_TRACE
#endif

#ifdef ROCKCHIP_DEBUG_ON
#define ROCKCHIP_DEBUG
#endif
typedef enum _LOG_LEVEL
{
    ROCKCHIP_LOG_TRACE,
    ROCKCHIP_LOG_INFO,
    ROCKCHIP_LOG_WARNING,
    ROCKCHIP_LOG_ERROR,
    ROCKCHIP_LOG_DEBUG
} ROCKCHIP_LOG_LEVEL;

#ifdef ROCKCHIP_LOG
#define Rockchip_OSAL_Log(a, ...)    ((void)_Rockchip_OSAL_Log(a, ROCKCHIP_LOG_TAG, __VA_ARGS__))
#else
#define Rockchip_OSAL_Log(a, ...)                                                \
    do {                                                                \
            ((void)_Rockchip_OSAL_Log(a, ROCKCHIP_LOG_TAG, __VA_ARGS__)); \
    } while (0)
#endif

#ifdef ROCKCHIP_TRACE
#define FunctionIn() omx_trace("IN")
#define FunctionOut() omx_trace("OUT")
#else
#define FunctionIn() ((void *)0)
#define FunctionOut() ((void *)0)
#endif

#define omx_info(format, ...)        _Rockchip_OSAL_Log(ROCKCHIP_LOG_INFO, ROCKCHIP_LOG_TAG, "%s(%d): " format "\n",__FUNCTION__, __LINE__, ##__VA_ARGS__)
#define omx_trace(format, ...)       _Rockchip_OSAL_Log(ROCKCHIP_LOG_TRACE, ROCKCHIP_LOG_TAG, "%s(%d): " format "\n",__FUNCTION__, __LINE__, ##__VA_ARGS__)
#define omx_err(format, ...)         _Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, ROCKCHIP_LOG_TAG, "%s(%d): " format "\n",__FUNCTION__, __LINE__, ##__VA_ARGS__)
#define omx_warn(format, ...)        _Rockchip_OSAL_Log(ROCKCHIP_LOG_WARNING, ROCKCHIP_LOG_TAG, "%s(%d): " format "\n",__FUNCTION__, __LINE__, ##__VA_ARGS__)
#define omx_dbg(format, ...)         _Rockchip_OSAL_Log(ROCKCHIP_LOG_DEBUG, ROCKCHIP_LOG_TAG, "%s(%d): " format "\n",__FUNCTION__, __LINE__, ##__VA_ARGS__)

extern void _Rockchip_OSAL_Log(ROCKCHIP_LOG_LEVEL logLevel, const char *tag, const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif
