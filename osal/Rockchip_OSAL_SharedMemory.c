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
 * @file        Rockchip_OSAL_SharedMemory.c
 * @brief
 * @author      csy(csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2013.11.26 : Create
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "vpu.h"

#include "Rockchip_OSAL_SharedMemory.h"
#include "Rockchip_OSAL_Mutex.h"

#include <linux/rockchip_ion.h>
#include <linux/ion.h>



#define ROCKCHIP_LOG_OFF
#include "Rockchip_OSAL_Log.h"

static int mem_cnt = 0;

struct ROCKCHIP_SHAREDMEM_LIST;
typedef struct _ROCKCHIP_SHAREDMEM_LIST {
    ion_user_handle_t              ion_hdl;
    OMX_PTR                        mapAddr;
    OMX_U32                        allocSize;
    OMX_BOOL                       owner;
    struct _ROCKCHIP_SHAREDMEM_LIST *pNextMemory;
} ROCKCHIP_SHAREDMEM_LIST;

typedef struct _ROCKCHIP_SHARED_MEMORY {
    int         fd;
    ROCKCHIP_SHAREDMEM_LIST *pAllocMemory;
    OMX_HANDLETYPE         hSMMutex;
} ROCKCHIP_SHARED_MEMORY;


#define ION_FUNCTION                (0x00000001)
#define ION_DEVICE                  (0x00000002)
#define ION_CLINET                  (0x00000004)
#define ION_IOCTL                   (0x00000008)

static int ion_ioctl(int fd, int req, void *arg)
{
    int ret = ioctl(fd, req, arg);
    if (ret < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_ioctl %x failed with code %d: %s\n", req,
                          ret, strerror(errno));
        return -errno;
    }
    return ret;
}

static int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
                     unsigned int flags, ion_user_handle_t *handle)
{
    int ret;
    struct ion_allocation_data data = {
        .len = len,
        .align = align,
        .heap_id_mask = heap_mask,
        .flags = flags,
    };

    if (handle == NULL)
        return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
    if (ret < 0)
        return ret;
    *handle = data.handle;
    return ret;
}

static int ion_free(int fd, ion_user_handle_t handle)
{
    struct ion_handle_data data = {
        .handle = handle,
    };
    return ion_ioctl(fd, ION_IOC_FREE, &data);
}

static int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
                   int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
    int ret;
    struct ion_fd_data data = {
        .handle = handle,
    };

    if (map_fd == NULL)
        return -EINVAL;
    if (ptr == NULL)
        return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_MAP, &data);
    if (ret < 0)
        return ret;
    *map_fd = data.fd;
    if (*map_fd < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "map ioctl returned negative fd\n");
        return -EINVAL;
    }
    *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
    if (*ptr == MAP_FAILED) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "mmap failed: %s\n", strerror(errno));
        return -errno;
    }
    return ret;
}

int ion_import(int fd, int share_fd, ion_user_handle_t *handle)
{
    int ret;
    struct ion_fd_data data = {
        .fd = share_fd,
    };

    if (handle == NULL)
        return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_IMPORT, &data);
    if (ret < 0)
        return ret;
    *handle = data.handle;
    return ret;
}

OMX_HANDLETYPE Rockchip_OSAL_SharedMemory_Open()
{
    ROCKCHIP_SHARED_MEMORY *pHandle = NULL;
    int  IONClient = 0;

    pHandle = (ROCKCHIP_SHARED_MEMORY *)malloc(sizeof(ROCKCHIP_SHARED_MEMORY));
    Rockchip_OSAL_Memset(pHandle, 0, sizeof(ROCKCHIP_SHARED_MEMORY));
    if (pHandle == NULL)
        goto EXIT;

    IONClient = open("/dev/ion", O_RDWR);

    if (IONClient <= 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_client_create Error: %d", IONClient);
        Rockchip_OSAL_Free((void *)pHandle);
        pHandle = NULL;
        goto EXIT;
    }

    pHandle->fd = IONClient;

    Rockchip_OSAL_MutexCreate(&pHandle->hSMMutex);

EXIT:
    return (OMX_HANDLETYPE)pHandle;
}

void Rockchip_OSAL_SharedMemory_Close(OMX_HANDLETYPE handle)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pDeleteElement = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Rockchip_OSAL_MutexLock(pHandle->hSMMutex);
    pCurrentElement = pSMList = pHandle->pAllocMemory;

    while (pCurrentElement != NULL) {
        pDeleteElement = pCurrentElement;
        pCurrentElement = pCurrentElement->pNextMemory;

        if (munmap(pDeleteElement->mapAddr, pDeleteElement->allocSize))
            Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_unmap fail");

        pDeleteElement->mapAddr = NULL;
        pDeleteElement->allocSize = 0;

        if (pDeleteElement->owner)
            ion_free(pHandle->fd, pDeleteElement->ion_hdl);
        pDeleteElement->ion_hdl = -1;

        Rockchip_OSAL_Free(pDeleteElement);

        mem_cnt--;
        Rockchip_OSAL_Log(ROCKCHIP_LOG_TRACE, "SharedMemory free count: %d", mem_cnt);
    }

    pHandle->pAllocMemory = pSMList = NULL;
    Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);

    Rockchip_OSAL_MutexTerminate(pHandle->hSMMutex);
    pHandle->hSMMutex = NULL;

    close(pHandle->fd);

    pHandle->fd = -1;

    Rockchip_OSAL_Free(pHandle);

EXIT:
    return;
}
static int ion_custom_op(int ion_client, int op, void *op_data)
{
    struct ion_custom_data data;
    int err;
    data.cmd = op;
    data.arg = (unsigned long)op_data;
    err = ioctl(ion_client, ION_IOC_CUSTOM, &data);
    if (err < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ION_IOC_CUSTOM (%d) failed with error - %s", op, strerror(errno));
        return err;
    }
    return err;
}

OMX_PTR Rockchip_OSAL_SharedMemory_Alloc(OMX_HANDLETYPE handle, OMX_U32 size, MEMORY_TYPE memoryType)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle         = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList         = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pElement        = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    ion_user_handle_t      ion_hdl = -1;
    OMX_PTR                pBuffer         = NULL;
    unsigned int mask;
    unsigned int flag;
    int err = 0;
    int map_fd = -1;

    if (pHandle == NULL)
        goto EXIT;

    pElement = (ROCKCHIP_SHAREDMEM_LIST *)malloc(sizeof(ROCKCHIP_SHAREDMEM_LIST));
    Rockchip_OSAL_Memset(pElement, 0, sizeof(ROCKCHIP_SHAREDMEM_LIST));
    pElement->owner = OMX_TRUE;

    switch (memoryType) {
    case SECURE_MEMORY:
        mask = ION_HEAP(ION_CMA_HEAP_ID);
        flag = 0;
        break;
    case SYSTEM_MEMORY:
        mask =  ION_HEAP(ION_VMALLOC_HEAP_ID);;
        flag = 0;
        break;
    default:
        pBuffer = NULL;
        goto EXIT;
        break;
    }

    err = ion_alloc((int)pHandle->fd, size, 4096, mask, flag, (ion_user_handle_t *)&ion_hdl);

    if (err < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_alloc Error: %d", ion_hdl);
        Rockchip_OSAL_Free((OMX_PTR)pElement);
        goto EXIT;
    }

    err = ion_map((int)pHandle->fd, ion_hdl, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, (off_t)0, (unsigned char**)&pBuffer, &map_fd);

    if (err) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_map Error");
        ion_free(pHandle->fd, ion_hdl);
        Rockchip_OSAL_Free((OMX_PTR)pElement);
        pBuffer = NULL;
        goto EXIT;
    }

    pElement->ion_hdl = ion_hdl;
    pElement->mapAddr = pBuffer;
    pElement->allocSize = size;
    pElement->pNextMemory = NULL;

    Rockchip_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        pHandle->pAllocMemory = pSMList = pElement;
    } else {
        pCurrentElement = pSMList;
        while (pCurrentElement->pNextMemory != NULL) {
            pCurrentElement = pCurrentElement->pNextMemory;
        }
        pCurrentElement->pNextMemory = pElement;
    }
    Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);

    mem_cnt++;
    Rockchip_OSAL_Log(ROCKCHIP_LOG_TRACE, "SharedMemory alloc count: %d", mem_cnt);

EXIT:
    return pBuffer;
}

void Rockchip_OSAL_SharedMemory_Free(OMX_HANDLETYPE handle, OMX_PTR pBuffer)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle         = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList         = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pDeleteElement  = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Rockchip_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->mapAddr == pBuffer) {
        pDeleteElement = pSMList;
        pHandle->pAllocMemory = pSMList = pSMList->pNextMemory;
    } else {
        while ((pCurrentElement != NULL) && (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr != pBuffer))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr == pBuffer)) {
            pDeleteElement = pCurrentElement->pNextMemory;
            pCurrentElement->pNextMemory = pDeleteElement->pNextMemory;
        } else {
            Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
            Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);

    if (munmap(pDeleteElement->mapAddr, pDeleteElement->allocSize)) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_unmap fail");
        goto EXIT;
    }
    pDeleteElement->mapAddr = NULL;
    pDeleteElement->allocSize = 0;

    if (pDeleteElement->owner)
        ion_free(pHandle->fd, (ion_user_handle_t)pDeleteElement->ion_hdl);
    pDeleteElement->ion_hdl = -1;

    Rockchip_OSAL_Free(pDeleteElement);

    mem_cnt--;
    Rockchip_OSAL_Log(ROCKCHIP_LOG_TRACE, "SharedMemory free count: %d", mem_cnt);

EXIT:
    return;
}

OMX_PTR Rockchip_OSAL_SharedMemory_Map(OMX_HANDLETYPE handle, OMX_U32 size, int ion_hdl)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pElement = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    OMX_PTR pBuffer = NULL;
    int err = 0;
    int map_fd = -1;

    if (pHandle == NULL)
        goto EXIT;

    pElement = Rockchip_OSAL_Malloc(sizeof(ROCKCHIP_SHAREDMEM_LIST));
    Rockchip_OSAL_Memset(pElement, 0, sizeof(ROCKCHIP_SHAREDMEM_LIST));

    if (ion_hdl == -1) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_alloc Error: %d", ion_hdl);
        Rockchip_OSAL_Free((void*)pElement);
        goto EXIT;
    }

    err = ion_map(pHandle->fd, (ion_user_handle_t)ion_hdl, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, (off_t)0, (unsigned char**)&pBuffer, &map_fd);

    if (pBuffer == NULL) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_map Error");
        ion_free(pHandle->fd, (ion_user_handle_t)ion_hdl);
        Rockchip_OSAL_Free((void*)pElement);
        goto EXIT;
    }

    pElement->ion_hdl = ion_hdl;
    pElement->mapAddr = pBuffer;
    pElement->allocSize = size;
    pElement->pNextMemory = NULL;

    Rockchip_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        pHandle->pAllocMemory = pSMList = pElement;
    } else {
        pCurrentElement = pSMList;
        while (pCurrentElement->pNextMemory != NULL) {
            pCurrentElement = pCurrentElement->pNextMemory;
        }
        pCurrentElement->pNextMemory = pElement;
    }
    Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);

    mem_cnt++;
    Rockchip_OSAL_Log(ROCKCHIP_LOG_TRACE, "SharedMemory alloc count: %d", mem_cnt);

EXIT:
    return pBuffer;
}

void Rockchip_OSAL_SharedMemory_Unmap(OMX_HANDLETYPE handle, int ionfd)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pDeleteElement = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Rockchip_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->ion_hdl == ionfd) {
        pDeleteElement = pSMList;
        pHandle->pAllocMemory = pSMList = pSMList->pNextMemory;
    } else {
        while ((pCurrentElement != NULL) && (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->ion_hdl != ionfd))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->ion_hdl == ionfd)) {
            pDeleteElement = pCurrentElement->pNextMemory;
            pCurrentElement->pNextMemory = pDeleteElement->pNextMemory;
        } else {
            Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
            Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);

    if (munmap(pDeleteElement->mapAddr, pDeleteElement->allocSize)) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion_unmap fail");
        goto EXIT;
    }
    pDeleteElement->mapAddr = NULL;
    pDeleteElement->allocSize = 0;
    pDeleteElement->ion_hdl = -1;

    Rockchip_OSAL_Free(pDeleteElement);

    mem_cnt--;
    Rockchip_OSAL_Log(ROCKCHIP_LOG_TRACE, "SharedMemory free count: %d", mem_cnt);

EXIT:
    return;
}

int Rockchip_OSAL_SharedMemory_VirtToION(OMX_HANDLETYPE handle, OMX_PTR pBuffer)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle         = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList         = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pFindElement    = NULL;
    if (pHandle == NULL || pBuffer == NULL)
        goto EXIT;

    Rockchip_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->mapAddr == pBuffer) {
        pFindElement = pSMList;
    } else {
        while ((pCurrentElement != NULL) && (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr != pBuffer))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr == pBuffer)) {
            pFindElement = pCurrentElement->pNextMemory;
        } else {
            Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
            Rockchip_OSAL_Log(ROCKCHIP_LOG_WARNING, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);

EXIT:
    return pFindElement->ion_hdl;
}

OMX_PTR Rockchip_OSAL_SharedMemory_IONToVirt(OMX_HANDLETYPE handle, int ion_fd)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle         = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList         = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pFindElement    = NULL;
    OMX_PTR pBuffer = NULL;
    if (pHandle == NULL || ion_fd == -1)
        goto EXIT;

    Rockchip_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->ion_hdl == ion_fd) {
        pFindElement = pSMList;
    } else {
        while ((pCurrentElement != NULL) && (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->ion_hdl != ion_fd))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((ROCKCHIP_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->ion_hdl == ion_fd)) {
            pFindElement = pCurrentElement->pNextMemory;
        } else {
            Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);
            Rockchip_OSAL_Log(ROCKCHIP_LOG_WARNING, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Rockchip_OSAL_MutexUnlock(pHandle->hSMMutex);

    pBuffer = pFindElement->mapAddr;

EXIT:
    return pBuffer;
}
static OMX_S32 check_used_heaps_type()
{
    // TODO, use property_get
    if (!VPUClientGetIOMMUStatus()) {
        return ION_HEAP(ION_CMA_HEAP_ID);
    } else {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_TRACE, "USE ION_SYSTEM_HEAP");
        return ION_HEAP(ION_VMALLOC_HEAP_ID);
    }

    return 0;
}
OMX_S32 Rockchip_OSAL_SharedMemory_getPhyAddress(OMX_HANDLETYPE handle, int share_fd, OMX_U32 *phyaddress)
{
    ROCKCHIP_SHARED_MEMORY  *pHandle = (ROCKCHIP_SHARED_MEMORY *)handle;
    ROCKCHIP_SHAREDMEM_LIST *pSMList = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pElement = NULL;
    ROCKCHIP_SHAREDMEM_LIST *pCurrentElement = NULL;
    OMX_PTR pBuffer = NULL;
    int err = 0;
    ion_user_handle_t  ion_handle = 0;
    struct ion_phys_data phys_data;
    if (check_used_heaps_type() == ION_HEAP(ION_CMA_HEAP_ID)) {
        err = ion_import(pHandle->fd, share_fd, &ion_handle);
        if (err) {
            Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "ion import failed, share fd %d\n", share_fd);
            return err;
        }
        phys_data.handle = ion_handle;
        err = ion_custom_op(pHandle->fd, ION_IOC_GET_PHYS, &phys_data);
        *phyaddress = phys_data.phys;
        ion_free(pHandle->fd, ion_handle);
    } else {
        *phyaddress = share_fd;
    }
    return 0;
}
