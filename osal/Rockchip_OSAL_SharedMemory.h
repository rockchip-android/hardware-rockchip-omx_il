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
 * @file        Rockchip_OSAL_SharedMemory.h
 * @brief
 * @author      csy(csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2013.11.26 : Create
 */


#ifndef ROCKCHIP_OSAL_SHAREDMEMORY
#define ROCKCHIP_OSAL_SHAREDMEMORY

#include "OMX_Types.h"
#include "Rockchip_OSAL_Memory.h"

#ifdef __cplusplus
extern "C" {
#endif

OMX_HANDLETYPE Rockchip_OSAL_SharedMemory_Open();
void Rockchip_OSAL_SharedMemory_Close(OMX_HANDLETYPE handle);
OMX_PTR Rockchip_OSAL_SharedMemory_Alloc(OMX_HANDLETYPE handle, OMX_U32 size, MEMORY_TYPE memoryType);
void Rockchip_OSAL_SharedMemory_Free(OMX_HANDLETYPE handle, OMX_PTR pBuffer);
int Rockchip_OSAL_SharedMemory_VirtToION(OMX_HANDLETYPE handle, OMX_PTR pBuffer);
OMX_PTR Rockchip_OSAL_SharedMemory_IONToVirt(OMX_HANDLETYPE handle, int ion_fd);
OMX_S32 Rockchip_OSAL_SharedMemory_getPhyAddress(OMX_HANDLETYPE handle, int share_fd, OMX_U32 *phyaddress);

#ifdef __cplusplus
}
#endif

#endif

