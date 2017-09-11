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
 * @file        Rockchip_OSAL_Android.cpp
 * @brief
 * @author      csy(csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2013.11.26 : Create
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <system/window.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <ui/GraphicBuffer.h>
#include <ui/Fence.h>
#include <media/hardware/HardwareAPI.h>
//#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <media/hardware/OMXPluginBase.h>
#include <media/hardware/MetadataBufferType.h>
#include "gralloc_priv_omx.h"
#include "Rockchip_OSAL_Mutex.h"
#include "Rockchip_OSAL_Semaphore.h"
#include "Rockchip_OMX_Baseport.h"
#include "Rockchip_OMX_Basecomponent.h"
#include "Rockchip_OMX_Macros.h"

#include "Rkvpu_OMX_Vdec.h"
#include "Rkvpu_OMX_Venc.h"
#include "Rockchip_OSAL_Android.h"
#undef  ROCKCHIP_LOG_TAG
#define ROCKCHIP_LOG_TAG    "ROCKCHIP_OSAL_Android"
#define ROCKCHIP_LOG_OFF
#include "Rockchip_OSAL_Log.h"
#include "vpu_mem_pool.h"
#include <cutils/properties.h>

enum {
    kFenceTimeoutMs = 1000
};

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif


OMX_ERRORTYPE Rockchip_OSAL_LockANBHandle(
    OMX_IN OMX_PTR handle,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
    gralloc_private_handle_t priv_hnd;
    Rockchip_OSAL_Memset(&priv_hnd, 0, sizeof(priv_hnd));
    Rockchip_get_gralloc_private((uint32_t*)handle, &priv_hnd);
    Rect bounds((uint32_t)((width + 31) & (~31)), (uint32_t)((height + 15) & (~15)));
    RockchipVideoPlane *vplanes = (RockchipVideoPlane *)planes;
    void *vaddr;

    omx_trace("%s: handle: 0x%x width %d height %d", __func__, handle, width, height);

    int usage = 0;

    switch (format) {
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420SemiPlanar:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
    default:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
    }

    if (mapper.lock(bufferHandle, usage, bounds, &vaddr) != 0) {
        omx_err("%s: mapper.lock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    vplanes[0].fd = priv_hnd.share_fd;
    vplanes[0].offset = 0;
    vplanes[0].addr = vaddr;
    vplanes[0].type = priv_hnd.type;
    vplanes[0].stride = priv_hnd.stride;

    omx_trace("%s: buffer locked: 0x%x", __func__, vaddr);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_getANBHandle(
    OMX_IN OMX_PTR handle,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
    gralloc_private_handle_t priv_hnd;
    Rockchip_OSAL_Memset(&priv_hnd, 0, sizeof(priv_hnd));
    Rockchip_get_gralloc_private((uint32_t*)handle, &priv_hnd);
    RockchipVideoPlane *vplanes = (RockchipVideoPlane *)planes;
    vplanes[0].fd = priv_hnd.share_fd;
    vplanes[0].offset = 0;
    vplanes[0].addr = NULL;
    vplanes[0].type = priv_hnd.type;
    vplanes[0].stride = priv_hnd.stride;
EXIT:
    FunctionOut();
    return ret;
}
OMX_ERRORTYPE Rockchip_OSAL_UnlockANBHandle(OMX_IN OMX_PTR handle)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;

    omx_trace("%s: handle: 0x%x", __func__, handle);

    if (mapper.unlock(bufferHandle) != 0) {
        omx_err("%s: mapper.unlock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    omx_trace("%s: buffer unlocked: 0x%x", __func__, handle);

EXIT:
    FunctionOut();

    return ret;
}

OMX_COLOR_FORMATTYPE Rockchip_OSAL_GetANBColorFormat(OMX_IN OMX_PTR handle)
{
    FunctionIn();

    OMX_COLOR_FORMATTYPE ret = OMX_COLOR_FormatUnused;
    gralloc_private_handle_t priv_hnd;
    Rockchip_OSAL_Memset(&priv_hnd, 0, sizeof(priv_hnd));
    Rockchip_get_gralloc_private((uint32_t*)handle, &priv_hnd);

    omx_trace("priv_hnd.format: 0x%x", priv_hnd.format);

    ret = Rockchip_OSAL_Hal2OMXPixelFormat(priv_hnd.format);

    omx_trace("ColorFormat: 0x%x", ret);

EXIT:
    FunctionOut();

    return ret;
}

OMX_U32 Rockchip_OSAL_GetANBStride(OMX_IN OMX_PTR handle)
{
    FunctionIn();

    OMX_U32 nStride = 0;
    gralloc_private_handle_t priv_hnd;
    Rockchip_OSAL_Memset(&priv_hnd, 0, sizeof(priv_hnd));
    Rockchip_get_gralloc_private((uint32_t*)handle, &priv_hnd);
    nStride = priv_hnd.stride;

EXIT:
    FunctionOut();

    return nStride;
}

OMX_U32 Get_Video_HorAlign(OMX_VIDEO_CODINGTYPE codecId, OMX_U32 width, OMX_U32 height)
{
    OMX_U32 stride = 0;;
    if (codecId == OMX_VIDEO_CodingHEVC) {
        stride = ((width + 255) & (~255)) | (256);
    } else if (codecId == OMX_VIDEO_CodingVP9) {
        stride = (width + 127) & (~127);
    } else {
        stride = ((width + 15) & (~15));
    }
    if (access("/dev/rkvdec", 06) == 0) {
        if (width > 1920 || height > 1088) {
            if (codecId == OMX_VIDEO_CodingAVC) {
                stride = ((width + 255) & (~255)) | (256);
            }
        }
    }
    return stride;
}

OMX_U32 Get_Video_VerAlign(OMX_VIDEO_CODINGTYPE codecId, OMX_U32 height)
{
    OMX_U32 stride = 0;;
    if (codecId == OMX_VIDEO_CodingHEVC) {
        stride = (height + 7) & (~7);
    } else if (codecId == OMX_VIDEO_CodingVP9) {
        stride = (height + 63) & (~63);
    } else {
        stride = ((height + 15) & (~15));
    }
    return stride;
}


OMX_ERRORTYPE Rockchip_OSAL_LockANB(
    OMX_IN OMX_PTR pBuffer,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = Rockchip_OSAL_LockANBHandle(pBuffer, width, height, format, planes);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_UnlockANB(OMX_IN OMX_PTR pBuffer)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ret = Rockchip_OSAL_UnlockANBHandle(pBuffer);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_LockMetaData(
    OMX_IN OMX_PTR pBuffer,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PTR pBuf;

    ret = Rockchip_OSAL_GetInfoFromMetaData((OMX_BYTE)pBuffer, &pBuf);
    if (ret == OMX_ErrorNone) {
        ret = Rockchip_OSAL_LockANBHandle(pBuf, width, height, format, planes);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_UnlockMetaData(OMX_IN OMX_PTR pBuffer)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PTR pBuf;

    ret = Rockchip_OSAL_GetInfoFromMetaData((OMX_BYTE)pBuffer, &pBuf);
    if (ret == OMX_ErrorNone)
        ret = Rockchip_OSAL_UnlockANBHandle(pBuf);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE useAndroidNativeBuffer(
    ROCKCHIP_OMX_BASEPORT      *pRockchipPort,
    OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_U32                nPortIndex,
    OMX_PTR                pAppPrivate,
    OMX_U32                nSizeBytes,
    OMX_U8                *pBuffer)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *temp_bufferHeader = NULL;
    unsigned int          i = 0;
    OMX_U32               width, height;
    OMX_U32               stride;
    RockchipVideoPlane      planes[MAX_BUFFER_PLANE];

    FunctionIn();

    if (pRockchipPort == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pRockchipPort->portState != OMX_StateIdle) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (CHECK_PORT_TUNNELED(pRockchipPort) && CHECK_PORT_BUFFER_SUPPLIER(pRockchipPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Rockchip_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Rockchip_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pRockchipPort->portDefinition.nBufferCountActual; i++) {
        if (pRockchipPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pRockchipPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pRockchipPort->bufferStateAllocate[i] = (BUFFER_STATE_ASSIGNED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = pBuffer;
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;

            width = pRockchipPort->portDefinition.format.video.nFrameWidth;
            height = pRockchipPort->portDefinition.format.video.nFrameHeight;
            Rockchip_OSAL_LockANB(temp_bufferHeader->pBuffer, width, height,
                                  pRockchipPort->portDefinition.format.video.eColorFormat, planes);
            pRockchipPort->extendBufferHeader[i].buf_fd[0] = planes[0].fd;
            pRockchipPort->extendBufferHeader[i].pYUVBuf[0] = planes[0].addr;
            Rockchip_OSAL_UnlockANB(temp_bufferHeader->pBuffer);
            omx_trace("useAndroidNativeBuffer: buf %d pYUVBuf[0]:0x%x (fd:%d)",
                              i, pRockchipPort->extendBufferHeader[i].pYUVBuf[0], planes[0].fd);

            pRockchipPort->assignedBufferNum++;
            if (pRockchipPort->assignedBufferNum == pRockchipPort->portDefinition.nBufferCountActual) {
                pRockchipPort->portDefinition.bPopulated = OMX_TRUE;
                /* Rockchip_OSAL_MutexLock(pRockchipComponent->compMutex); */
                Rockchip_OSAL_SemaphorePost(pRockchipPort->loadedResource);
                /* Rockchip_OSAL_MutexUnlock(pRockchipComponent->compMutex); */
            }
            *ppBufferHdr = temp_bufferHeader;
            ret = OMX_ErrorNone;

            goto EXIT;
        }
    }

    Rockchip_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_GetANBParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pRockchipComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch ((ROCKCHIP_OMX_INDEXTYPE)nIndex) {
    case OMX_IndexParamGetAndroidNativeBufferUsage: {
        GetAndroidNativeBufferUsageParams *pANBParams = (GetAndroidNativeBufferUsageParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;

        omx_trace("%s: OMX_IndexParamGetAndroidNativeBufferUsage", __func__);

        ret = Rockchip_OMX_Check_SizeVersion(pANBParams, sizeof(GetAndroidNativeBufferUsageParams));
        if (ret != OMX_ErrorNone) {
            omx_err("%s: Rockchip_OMX_Check_SizeVersion(GetAndroidNativeBufferUsageParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pRockchipComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /* NOTE: OMX_IndexParamGetAndroidNativeBuffer returns original 'nUsage' without any
         * modifications since currently not defined what the 'nUsage' is for.
         */
        pANBParams->nUsage |= (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP);
    }
    break;
    case OMX_IndexParamdescribeColorFormat: {
#ifndef LOW_VRESION
        RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
        DescribeColorFormatParams *pDescribeParams = (DescribeColorFormatParams *) ComponentParameterStructure;
        MediaImage *img = &pDescribeParams->sMediaImage;
        ROCKCHIP_OMX_BASEPORT *pRockchipPort = NULL;

        omx_trace("%s: OMX_IndexParamdescribeColorFormat", __func__);
        ret = Rockchip_OMX_Check_SizeVersion(pDescribeParams, sizeof(DescribeColorFormatParams));
        if (ret != OMX_ErrorNone) {
            omx_err("%s: Rockchip_OMX_Check_SizeVersion(DescribeColorFormatParams) is failed", __func__);
            goto EXIT;
        }
        pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
        pRockchipPort->bufferProcessType = BUFFER_COPY;
        switch (pDescribeParams->eColorFormat) {
        case OMX_COLOR_FormatYUV420SemiPlanar: {
            OMX_U32 stride = 0;
            OMX_U32 sliceHeight = 0;
            img->mType = MediaImage::MEDIA_IMAGE_TYPE_YUV;
            img->mNumPlanes = 3;
            img->mWidth = pDescribeParams->nFrameWidth;
            img->mHeight = pDescribeParams->nFrameHeight;
            pRockchipPort->bufferProcessType = BUFFER_COPY;
            omx_trace("OMX_IndexParamdescribeColorFormat OMX_COLOR_FormatYUV420SemiPlanar in");
            stride = img->mWidth;
            sliceHeight = img->mHeight;

            img->mBitDepth = 8;
            img->mPlane[MediaImage::Y].mOffset = 0;
            img->mPlane[MediaImage::Y].mColInc = 1;
            img->mPlane[MediaImage::Y].mRowInc = stride; //same as stride
            img->mPlane[MediaImage::Y].mHorizSubsampling = 1;
            img->mPlane[MediaImage::Y].mVertSubsampling = 1;
            img->mPlane[MediaImage::U].mOffset = stride * sliceHeight ;
            img->mPlane[MediaImage::U].mColInc = 2;           //interleaved UV
            img->mPlane[MediaImage::U].mRowInc = stride;
            img->mPlane[MediaImage::U].mHorizSubsampling = 2;
            img->mPlane[MediaImage::U].mVertSubsampling = 2;
            img->mPlane[MediaImage::V].mOffset = stride * sliceHeight + 1;
            img->mPlane[MediaImage::V].mColInc = 2;           //interleaved UV
            img->mPlane[MediaImage::V].mRowInc = stride;
            img->mPlane[MediaImage::V].mHorizSubsampling = 2;
            img->mPlane[MediaImage::V].mVertSubsampling = 2;
        }
        break;
        case OMX_COLOR_FormatYUV420Planar: {
            OMX_U32 stride = 0;
            OMX_U32 sliceHeight = 0;
            img->mType = MediaImage::MEDIA_IMAGE_TYPE_YUV;
            img->mNumPlanes = 3;
            img->mWidth = pDescribeParams->nFrameWidth;
            img->mHeight = pDescribeParams->nFrameHeight;
            omx_trace("OMX_IndexParamdescribeColorFormat OMX_COLOR_FormatYUV420SemiPlanar in");
            stride = img->mWidth;
            sliceHeight = img->mHeight;
            img->mBitDepth = 8;
            img->mPlane[MediaImage::Y].mOffset = 0;
            img->mPlane[MediaImage::Y].mColInc = 1;
            img->mPlane[MediaImage::Y].mRowInc = stride; //same as stride
            img->mPlane[MediaImage::Y].mHorizSubsampling = 1;
            img->mPlane[MediaImage::Y].mVertSubsampling = 1;
            img->mPlane[MediaImage::U].mOffset = stride * sliceHeight;
            img->mPlane[MediaImage::U].mColInc = 2;           //interleaved UV
            img->mPlane[MediaImage::U].mRowInc = stride / 2;
            img->mPlane[MediaImage::U].mHorizSubsampling = 2;
            img->mPlane[MediaImage::U].mVertSubsampling = 2;
            img->mPlane[MediaImage::V].mOffset = stride * sliceHeight + stride * sliceHeight / 4;
            img->mPlane[MediaImage::V].mColInc = 2;           //interleaved UV
            img->mPlane[MediaImage::V].mRowInc = stride / 2;
            img->mPlane[MediaImage::V].mHorizSubsampling = 2;
            img->mPlane[MediaImage::V].mVertSubsampling = 2;
        }
        break;
        default:
            omx_err("OMX_IndexParamdescribeColorFormat default in");
            img->mType = MediaImage::MEDIA_IMAGE_TYPE_UNKNOWN;
            return OMX_ErrorNone;
        }
#endif
    }
    break;
    default: {
        omx_err("%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
    break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_SetANBParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pRockchipComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch ((ROCKCHIP_OMX_INDEXTYPE)nIndex) {
    case OMX_IndexParamEnableAndroidBuffers: {
        RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
        EnableAndroidNativeBuffersParams *pANBParams = (EnableAndroidNativeBuffersParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        ROCKCHIP_OMX_BASEPORT *pRockchipPort = NULL;

        omx_trace("%s: OMX_IndexParamEnableAndroidNativeBuffers", __func__);

        ret = Rockchip_OMX_Check_SizeVersion(pANBParams, sizeof(EnableAndroidNativeBuffersParams));
        if (ret != OMX_ErrorNone) {
            omx_err("%s: Rockchip_OMX_Check_SizeVersion(EnableAndroidNativeBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pRockchipComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
        if (CHECK_PORT_TUNNELED(pRockchipPort) && CHECK_PORT_BUFFER_SUPPLIER(pRockchipPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /* ANB and DPB Buffer Sharing */
        if (pVideoDec->bStoreMetaData != OMX_TRUE) {

            pVideoDec->bIsANBEnabled = pANBParams->enable;
            pRockchipPort->portDefinition.nBufferCountActual = 16;
            pRockchipPort->portDefinition.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCrCb_NV12;
            omx_trace("OMX_IndexParamEnableAndroidBuffers set buffcount %d", pRockchipPort->portDefinition.nBufferCountActual);
            /*
                this is temp way to avoid android.media.cts.ImageReaderDecoderTest rk decoder test

            */
            if (pRockchipPort->bufferProcessType == BUFFER_COPY) {
                if ((pVideoDec->codecId != OMX_VIDEO_CodingH263) && (pRockchipPort->portDefinition.format.video.nFrameWidth >= 176)) {
                    pRockchipPort->bufferProcessType = BUFFER_ANBSHARE;
                }
            }
        }

        omx_trace("portIndex = %d,pRockchipPort->bufferProcessType =0x%x", portIndex, pRockchipPort->bufferProcessType);
        if ((portIndex == OUTPUT_PORT_INDEX) &&
            ((pRockchipPort->bufferProcessType & BUFFER_ANBSHARE) == BUFFER_ANBSHARE)) {
            if (pVideoDec->bIsANBEnabled == OMX_TRUE) {
                pRockchipPort->bufferProcessType = BUFFER_SHARE;
                pRockchipPort->portDefinition.nBufferCountActual = 22;
                pRockchipPort->portDefinition.nBufferCountMin = 18;
                if (pRockchipPort->portDefinition.format.video.nFrameWidth 
                        * pRockchipPort->portDefinition.format.video.nFrameHeight > 1920 * 1088) {
                    pRockchipPort->portDefinition.nBufferCountActual = 14;
                    pRockchipPort->portDefinition.nBufferCountMin = 10;
                }
                if (pRockchipPort->portDefinition.format.video.nFrameWidth <= 1280) {
                    pRockchipPort->portDefinition.nBufferCountActual = 25;
                    pRockchipPort->portDefinition.nBufferCountMin = 21;
                }
                pRockchipPort->portDefinition.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCrCb_NV12;
                omx_trace("OMX_IndexParamEnableAndroidBuffers & bufferProcessType change to BUFFER_SHARE");
            }
            Rockchip_OSAL_Openvpumempool(pRockchipComponent, portIndex);
        }
    }
    break;

    case OMX_IndexParamUseAndroidNativeBuffer: {
        RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
        UseAndroidNativeBufferParams *pANBParams = (UseAndroidNativeBufferParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        ROCKCHIP_OMX_BASEPORT *pRockchipPort = NULL;
        android_native_buffer_t *pANB;
        OMX_U32 nSizeBytes;

        omx_trace("%s: OMX_IndexParamUseAndroidNativeBuffer, portIndex: %d", __func__, portIndex);

        ret = Rockchip_OMX_Check_SizeVersion(pANBParams, sizeof(UseAndroidNativeBufferParams));
        if (ret != OMX_ErrorNone) {
            omx_err("%s: Rockchip_OMX_Check_SizeVersion(UseAndroidNativeBufferParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pRockchipComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
        if (CHECK_PORT_TUNNELED(pRockchipPort) && CHECK_PORT_BUFFER_SUPPLIER(pRockchipPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if (pRockchipPort->portState != OMX_StateIdle) {
            omx_err("%s: Port state should be IDLE", __func__);
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        pANB = pANBParams->nativeBuffer.get();

        /* MALI alignment restriction */
        nSizeBytes = ALIGN(pANB->width, 16) * ALIGN(pANB->height, 16);
        nSizeBytes += ALIGN(pANB->width / 2, 16) * ALIGN(pANB->height / 2, 16) * 2;

        ret = useAndroidNativeBuffer(pRockchipPort,
                                     pANBParams->bufferHeader,
                                     pANBParams->nPortIndex,
                                     pANBParams->pAppPrivate,
                                     nSizeBytes,
                                     (OMX_U8 *) pANB);
        if (ret != OMX_ErrorNone) {
            omx_err("%s: useAndroidNativeBuffer is failed: err=0x%x", __func__, ret);
            goto EXIT;
        }
    }
    break;

    case OMX_IndexParamStoreMetaDataBuffer: {
        StoreMetaDataInBuffersParams *pANBParams = (StoreMetaDataInBuffersParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        ROCKCHIP_OMX_BASEPORT *pRockchipPort = NULL;

        omx_trace("%s: OMX_IndexParamStoreMetaDataBuffer", __func__);

        ret = Rockchip_OMX_Check_SizeVersion(pANBParams, sizeof(StoreMetaDataInBuffersParams));
        if (ret != OMX_ErrorNone) {
            omx_err("%s: Rockchip_OMX_Check_SizeVersion(StoreMetaDataInBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pRockchipComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
        if (CHECK_PORT_TUNNELED(pRockchipPort) && CHECK_PORT_BUFFER_SUPPLIER(pRockchipPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pRockchipPort->bStoreMetaData = pANBParams->bStoreMetaData;
        if (pRockchipComponent->codecType == HW_VIDEO_ENC_CODEC) {
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;;
            pVideoEnc->bFirstInput = OMX_TRUE;
            if (portIndex == INPUT_PORT_INDEX)
                pVideoEnc->bStoreMetaData = pANBParams->bStoreMetaData;
        } else if (pRockchipComponent->codecType == HW_VIDEO_DEC_CODEC) {
            RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
            pVideoDec->bStoreMetaData = pANBParams->bStoreMetaData;
            pRockchipPort->bufferProcessType = BUFFER_SHARE;
            pRockchipPort->portDefinition.nBufferCountActual = 17;
            if (pRockchipPort->portDefinition.format.video.nFrameWidth 
			        * pRockchipPort->portDefinition.format.video.nFrameHeight > 1920 * 1088) {
                pRockchipPort->portDefinition.nBufferCountActual = 14;
            }
            if (pRockchipPort->portDefinition.format.video.nFrameWidth <= 1280) {
                pRockchipPort->portDefinition.nBufferCountActual = 25;
            }
            Rockchip_OSAL_Openvpumempool(pRockchipComponent, portIndex);
            pRockchipPort->portDefinition.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCrCb_NV12;
            omx_trace("OMX_IndexParamEnableAndroidBuffers & bufferProcessType change to BUFFER_SHARE");
        }
    }
    break;
    case OMX_IndexParamprepareForAdaptivePlayback: {
        omx_trace("%s: OMX_IndexParamprepareForAdaptivePlayback", __func__);
    }
    break;
    default: {
        omx_err("%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
    break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_GetInfoFromMetaData(OMX_IN OMX_BYTE pBuffer,
                                                OMX_OUT OMX_PTR *ppBuf)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    MetadataBufferType type;

    FunctionIn();

    /*
     * meta data contains the following data format.
     * payload depends on the MetadataBufferType
     * --------------------------------------------------------------
     * | MetadataBufferType                         |          payload                           |
     * --------------------------------------------------------------
     *
     * If MetadataBufferType is kMetadataBufferTypeCameraSource, then
     * --------------------------------------------------------------
     * | kMetadataBufferTypeCameraSource  | physical addr. of Y |physical addr. of CbCr |
     * --------------------------------------------------------------
     *
     * If MetadataBufferType is kMetadataBufferTypeGrallocSource, then
     * --------------------------------------------------------------
     * | kMetadataBufferTypeGrallocSource    | buffer_handle_t |
     * --------------------------------------------------------------
     */


    /* MetadataBufferType */
    Rockchip_OSAL_Memcpy(&type, pBuffer, sizeof(MetadataBufferType));
#ifdef USE_ANW
    if (type > kMetadataBufferTypeNativeHandleSource) {
        omx_err("Data passed in with metadata mode does not have type "
                          "kMetadataBufferTypeGrallocSource (%d), has type %ld instead",
                          kMetadataBufferTypeGrallocSource, type);
        return OMX_ErrorBadParameter;
    }
#else
    if ((type != kMetadataBufferTypeGrallocSource) && (type != kMetadataBufferTypeCameraSource)) {
        omx_err("Data passed in with metadata mode does not have type "
                          "kMetadataBufferTypeGrallocSource (%d), has type %ld instead",
                          kMetadataBufferTypeGrallocSource, type);
        return OMX_ErrorBadParameter;
    }
#endif
    if (type == kMetadataBufferTypeCameraSource) {

        void *pAddress = NULL;

        /* Address. of Y */
        Rockchip_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType), sizeof(void *));
        ppBuf[0] = (void *)pAddress;

        /* Address. of CbCr */
        Rockchip_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType) + sizeof(void *), sizeof(void *));
        ppBuf[1] = (void *)pAddress;

    } else if (type == kMetadataBufferTypeGrallocSource) {

        buffer_handle_t    pBufHandle;

        /* buffer_handle_t */
        Rockchip_OSAL_Memcpy(&pBufHandle, pBuffer + sizeof(MetadataBufferType), sizeof(buffer_handle_t));
        ppBuf[0] = (OMX_PTR)pBufHandle;
    }
#ifdef USE_ANW
    else if (type == kMetadataBufferTypeANWBuffer) {
        VideoNativeMetadata &nativeMeta = *(VideoNativeMetadata *)pBuffer;
        ANativeWindowBuffer *buffer = nativeMeta.pBuffer;
        ppBuf[0] = (OMX_PTR)buffer->handle;
        if (nativeMeta.nFenceFd >= 0) {
            sp<Fence> fence = new Fence(nativeMeta.nFenceFd);
            nativeMeta.nFenceFd = -1;
            status_t err = fence->wait(kFenceTimeoutMs);
            if (err != OK) {
                omx_err("Timed out waiting on input fence");
                return OMX_ErrorBadParameter;
            }
        }
    } else if (type == kMetadataBufferTypeNativeHandleSource) {
        omx_trace("kMetadataBufferTypeNativeHandleSource process in");
        VideoNativeHandleMetadata &nativehandleMeta = *(VideoNativeHandleMetadata*)pBuffer;
        ppBuf[0] = (OMX_PTR)nativehandleMeta.pHandle;
    }
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_GetInfoRkWfdMetaData(OMX_IN OMX_BYTE pBuffer,
                                                 OMX_OUT OMX_PTR *ppBuf)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    OMX_U32 type;
    buffer_handle_t    pBufHandle;
    FunctionIn();

    /*
     * meta data contains the following data format.
     * payload depends on the MetadataBufferType
     * --------------------------------------------------------------
     * | MetadataBufferType                         |          payload                           |
     * --------------------------------------------------------------
     *
     * If MetadataBufferType is kMetadataBufferTypeCameraSource, then
     * --------------------------------------------------------------
     * | kMetadataBufferTypeCameraSource  | WFD(0x1234)(4byte)|VPU_MEM(4byte)|rga_fd(4byte)|buffer_handle_t ]
     * | share_fd
     * --------------------------------------------------------------
     * else is kMetadataBufferANWSource, then
     * --------------------------------------------------------------
     * | kMetadataBufferTypeANW source | ANW | fence | WFD(0x1234)
     * --------------------------------------------------------------
     */

    /* MetadataBufferType */
    Rockchip_OSAL_Memcpy(&type, pBuffer + 4, 4);
#ifdef USE_ANW
    if (type != 0x1234) {
         Rockchip_OSAL_Memcpy(&type, pBuffer+sizeof(VideoNativeMetadata), 4);
         VideoGrallocMetadata *metadata = (VideoGrallocMetadata *)pBuffer;
         omx_trace("###type=0x%x, bufftype=%d",type, metadata->eType);
         if (type != 0x1234) {
            return OMX_ErrorBadParameter;
         }
         pBufHandle = metadata->pHandle;
         ppBuf[0] = (OMX_PTR)pBufHandle;
         return ret;
    }
#else
    if (type != 0x1234) {
        return OMX_ErrorBadParameter;
    }
#endif

    /* buffer_handle_t */
    Rockchip_OSAL_Memcpy(&pBufHandle, pBuffer + 16, sizeof(buffer_handle_t));
    ppBuf[0] = (OMX_PTR)pBufHandle;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_SetPrependSPSPPSToIDR(
    OMX_PTR pComponentParameterStructure,
    OMX_PTR pbPrependSpsPpsToIdr)
{
    OMX_ERRORTYPE                    ret        = OMX_ErrorNone;
    PrependSPSPPSToIDRFramesParams  *pANBParams = (PrependSPSPPSToIDRFramesParams *)pComponentParameterStructure;

    ret = Rockchip_OMX_Check_SizeVersion(pANBParams, sizeof(PrependSPSPPSToIDRFramesParams));
    if (ret != OMX_ErrorNone) {
        omx_err("%s: Rockchip_OMX_Check_SizeVersion(PrependSPSPPSToIDRFrames) is failed", __func__);
        goto EXIT;
    }

    (*((OMX_BOOL *)pbPrependSpsPpsToIdr)) = pANBParams->bEnable;

EXIT:
    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_CheckBuffType(OMX_U32 type)
{

    if ((type != kMetadataBufferTypeGrallocSource) && (type != kMetadataBufferTypeCameraSource)) {
        omx_err("Data passed in with metadata mode does not have type "
                          "kMetadataBufferTypeGrallocSource (%d), has type %ld instead",
                          kMetadataBufferTypeGrallocSource, type);
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}


OMX_COLOR_FORMATTYPE Rockchip_OSAL_Hal2OMXPixelFormat(
    unsigned int hal_format)
{
    OMX_COLOR_FORMATTYPE omx_format;
    switch (hal_format) {
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        omx_format = OMX_COLOR_FormatYCbYCr;
        break;
    case HAL_PIXEL_FORMAT_YV12:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    case HAL_PIXEL_FORMAT_YCrCb_NV12:
        omx_format = OMX_COLOR_FormatYUV420SemiPlanar;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
        omx_format = (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYUV420Flexible;
        break;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        omx_format = OMX_COLOR_Format32bitBGRA8888;
        break;
    case  HAL_PIXEL_FORMAT_RGBA_8888:
    case  HAL_PIXEL_FORMAT_RGBX_8888:
        omx_format = OMX_COLOR_Format32bitARGB8888;
        break;
    default:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    }
    return omx_format;
}

unsigned int Rockchip_OSAL_OMX2HalPixelFormat(
    OMX_COLOR_FORMATTYPE omx_format)
{
    unsigned int hal_format;
    switch ((OMX_U32)omx_format) {
    case OMX_COLOR_FormatYCbYCr:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_422_I;
        break;
    case OMX_COLOR_FormatYUV420Planar:
        hal_format = HAL_PIXEL_FORMAT_YV12;
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
        hal_format = HAL_PIXEL_FORMAT_YCrCb_NV12;
        break;
    case OMX_COLOR_FormatYUV420Flexible:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_888;
        break;
    case OMX_COLOR_Format32bitARGB8888:
        hal_format = HAL_PIXEL_FORMAT_RGBA_8888;
        break;
    case OMX_COLOR_Format32bitBGRA8888:
        hal_format = HAL_PIXEL_FORMAT_BGRA_8888;
        break;
    default:
        hal_format = HAL_PIXEL_FORMAT_YV12;
        break;
    }
    return hal_format;
}

OMX_ERRORTYPE Rockchip_OSAL_Fd2VpumemPool(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_BUFFERHEADERTYPE* bufferHeader)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT        *pRockchipPort        = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    struct vpu_display_mem_pool *pMem_pool = (struct vpu_display_mem_pool*)pVideoDec->vpumem_handle;
    OMX_U32 i = 0;
    OMX_U32 width = pRockchipPort->portDefinition.format.video.nStride;
    OMX_U32 height = pRockchipPort->portDefinition.format.video.nSliceHeight;
#if 1//LOW_VRESION
    OMX_U32 nBytesize = width * height * 2;
#else
    OMX_U32 nBytesize = width * height * 9 / 5;
#endif
    OMX_S32 dupshared_fd = -1;

    for (i = 0; i < pRockchipPort->portDefinition.nBufferCountActual; i++) {
        if (pRockchipPort->extendBufferHeader[i].OMXBufferHeader == bufferHeader) {
            omx_trace("commit bufferHeader 0x%x", bufferHeader);
            break;
        }
    }
    if (!pRockchipPort->extendBufferHeader[i].pRegisterFlag) {
        buffer_handle_t bufferHandle = NULL;
        if (pVideoDec->bStoreMetaData == OMX_TRUE) {
            OMX_PTR pBufferHandle;
            Rockchip_OSAL_GetInfoFromMetaData(bufferHeader->pBuffer, &pBufferHandle);
            bufferHandle =  (buffer_handle_t)pBufferHandle;
        } else {
            bufferHandle = (buffer_handle_t) bufferHeader->pBuffer;
        }
        gralloc_private_handle_t priv_hnd;
        Rockchip_OSAL_Memset(&priv_hnd, 0, sizeof(priv_hnd));
        Rockchip_get_gralloc_private((uint32_t*)bufferHandle, &priv_hnd);
        if (((!VPUMemJudgeIommu()) ? (priv_hnd.type != ANB_PRIVATE_BUF_VIRTUAL) : 1)) {
            pRockchipPort->extendBufferHeader[i].buf_fd[0] = priv_hnd.share_fd;
            pRockchipPort->extendBufferHeader[i].pRegisterFlag = 1;
            omx_trace("priv_hnd.share_fd = 0x%x", priv_hnd.share_fd);
            if (priv_hnd.share_fd > 0) {
                if (priv_hnd.size) {
                    nBytesize = priv_hnd.size;
                }
                if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
                    priv_hnd.share_fd |= 1 << 10;
                }
                dupshared_fd = pMem_pool->commit_hdl(pMem_pool, priv_hnd.share_fd , nBytesize);
                if (dupshared_fd > 0) {
                    pRockchipPort->extendBufferHeader[i].buf_fd[0] = dupshared_fd;
                }
                omx_trace("commit bufferHeader 0x%x share_fd = 0x%x nBytesize = %d", bufferHeader, pRockchipPort->extendBufferHeader[i].buf_fd[0], nBytesize);
            }
        } else {
            omx_trace("cma case gpu vmalloc can't used");
        }
    } else {
        omx_trace(" free bufferHeader 0x%x", pRockchipPort->extendBufferHeader[i].OMXBufferHeader);
        if (pRockchipPort->extendBufferHeader[i].pPrivate != NULL) {
            Rockchip_OSAL_FreeVpumem(pRockchipPort->extendBufferHeader[i].pPrivate);
            pRockchipPort->extendBufferHeader[i].pPrivate = NULL;
        };
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Rockchip_OSAL_resetVpumemPool(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT        *pRockchipPort        = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    struct vpu_display_mem_pool *pMem_pool = (struct vpu_display_mem_pool*)pVideoDec->vpumem_handle;
    pMem_pool->reset(pMem_pool);
    return OMX_ErrorNone;
}
OMX_ERRORTYPE  Rockchip_OSAL_FreeVpumem(OMX_IN OMX_PTR pVpuframe)
{

    omx_trace("Rockchip_OSAL_FreeVpumem");
    VPU_FRAME *pframe = (VPU_FRAME *)pVpuframe;
    VPUMemLink(&pframe->vpumem);
    VPUFreeLinear(&pframe->vpumem);
    Rockchip_OSAL_Free(pframe);
    return OMX_ErrorNone;
}

OMX_BUFFERHEADERTYPE *Rockchip_OSAL_Fd2OmxBufferHeader(ROCKCHIP_OMX_BASEPORT *pRockchipPort, OMX_IN OMX_S32 fd, OMX_IN OMX_PTR pVpuframe)
{
    OMX_U32 i = 0;
    for (i = 0; i < pRockchipPort->portDefinition.nBufferCountActual; i++) {
        if (fd == pRockchipPort->extendBufferHeader[i].buf_fd[0]) {
            omx_trace(" current fd = 0x%x send to render current header 0x%x", fd, pRockchipPort->extendBufferHeader[i].OMXBufferHeader);
            if ( pRockchipPort->extendBufferHeader[i].pPrivate != NULL) {
                omx_trace("This buff alreay send to display ");
                return NULL;
            }
            if (pVpuframe) {
                pRockchipPort->extendBufferHeader[i].pPrivate = pVpuframe;
            } else {
                omx_trace("vpu_mem point is NULL may error");
            }
            return pRockchipPort->extendBufferHeader[i].OMXBufferHeader;
        }
    }
    return NULL;
}

OMX_ERRORTYPE  Rockchip_OSAL_Openvpumempool(OMX_IN ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_U32 portIndex)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    VpuCodecContext_t           *p_vpu_ctx = pVideoDec->vpu_ctx;
    ROCKCHIP_OMX_BASEPORT *pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
    if (pRockchipPort->bufferProcessType == BUFFER_SHARE) {
        pVideoDec->vpumem_handle = (void*)open_vpu_memory_pool();
        if (pVideoDec->vpumem_handle != NULL) {
            omx_trace("open_vpu_memory_pool success handle 0x%x", pVideoDec->vpumem_handle);
        }
    } else {
        vpu_display_mem_pool   *pool = NULL;
        OMX_U32 hor_stride = Get_Video_HorAlign(pVideoDec->codecId, pRockchipPort->portDefinition.format.video.nStride,
                                                pRockchipPort->portDefinition.format.video.nSliceHeight);

        OMX_U32 ver_stride = Get_Video_VerAlign(pVideoDec->codecId, pRockchipPort->portDefinition.format.video.nSliceHeight);
        omx_err("hor_stride %d ver_stride %d", hor_stride, ver_stride);
        if (0 != create_vpu_memory_pool_allocator(&pool, 8, (hor_stride * ver_stride * 2))) {
            omx_err("create_vpu_memory_pool_allocator fail");
        }
        pVideoDec->vpumem_handle = (void*)(pool);
    }
    return OMX_ErrorNone;
}


OMX_ERRORTYPE  Rockchip_OSAL_Closevpumempool(OMX_IN ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{

    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT *pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    if (pRockchipPort->bufferProcessType == BUFFER_SHARE) {
        close_vpu_memory_pool((vpu_display_mem_pool *)pVideoDec->vpumem_handle);
        pVideoDec->vpumem_handle = NULL;
    } else if (pVideoDec->vpumem_handle != NULL) {
        release_vpu_memory_pool_allocator((vpu_display_mem_pool*)pVideoDec->vpumem_handle );
        pVideoDec->vpumem_handle  = NULL;
    }
    return OMX_ErrorNone;
}


//DDR Frequency conversion
 OMX_ERRORTYPE Rockchip_OSAL_PowerControl(
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
    int32_t width,
    int32_t height,
    int32_t mHevc,
    int32_t frameRate,
    OMX_BOOL mFlag,
 	int bitDepth)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    char prop_value[PROPERTY_VALUE_MAX];
    if (!property_get("sf.power.control", prop_value, NULL) || atoi(prop_value) <= 0) {
        return OMX_ErrorUndefined;
    }

    if (pVideoDec->power_fd == -1) {
        pVideoDec->power_fd = open("/dev/video_state", O_WRONLY);
        if (pVideoDec->power_fd == -1) {
            omx_err("power control open fd fail");
        }
    }

    if (bitDepth <= 0) bitDepth = 8;

    char para[200]= {0};
    int paraLen = 0;
    paraLen = sprintf(para, "%d,width=%d,height=%d,ishevc=%d,videoFramerate=%d,streamBitrate=%d", mFlag, width, height, mHevc, frameRate, bitDepth);
    omx_info(" write: %s", para);
    if (pVideoDec->power_fd != -1) {
        write(pVideoDec->power_fd, para, paraLen);
        if (!mFlag) {
            close(pVideoDec->power_fd);
            pVideoDec->power_fd = -1;
        }
    }

    return OMX_ErrorNone;
}



OMX_COLOR_FORMATTYPE Rockchip_OSAL_CheckFormat(
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
    OMX_IN OMX_PTR pVpuframe)
{
    RKVPU_OMX_VIDEODEC_COMPONENT    *pVideoDec          = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT           *pInputPort         = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    OMX_COLOR_FORMATTYPE             eColorFormat       = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCrCb_NV12;
    VPU_FRAME *pframe = (VPU_FRAME *)pVpuframe;
    VpuCodecContext_t *p_vpu_ctx = pVideoDec->vpu_ctx;
    if ((pVideoDec->codecId == OMX_VIDEO_CodingHEVC && (pframe->OutputWidth != 0x20))
            || (pframe->ColorType & VPU_OUTPUT_FORMAT_BIT_MASK) == VPU_OUTPUT_FORMAT_BIT_10) { // 10bit
        eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCrCb_NV12_10;
        omx_trace("set format to nv12 10bit");
        if ((pframe->ColorType & OMX_COLORSPACE_MASK) != 0) {
            OMX_RK_EXT_COLORSPACE colorSpace = (OMX_RK_EXT_COLORSPACE)((pframe->ColorType & OMX_COLORSPACE_MASK) >> 20);
            pVideoDec->extColorSpace = colorSpace;
            omx_trace("extension color space = %d", colorSpace);
        }
        if ((pframe->ColorType & OMX_DYNCRANGE_MASK) != 0) {
           OMX_RK_EXT_DYNCRANGE dyncRange = (OMX_RK_EXT_DYNCRANGE)((pframe->ColorType & OMX_DYNCRANGE_MASK) >> 24);
           pVideoDec->extDyncRange = dyncRange;
        }

        if (pVideoDec->bIsPowerControl == OMX_TRUE && pVideoDec->bIs10bit == OMX_FALSE) {
            Rockchip_OSAL_PowerControl(pRockchipComponent, 3840, 2160, pVideoDec->bIsHevc,
                                           pInputPort->portDefinition.format.video.xFramerate,
                                           OMX_FALSE,
                                           8);
            pVideoDec->bIsPowerControl = OMX_FALSE;
        }

        if (pframe->FrameWidth > 1920 && pframe->FrameHeight > 1088 
            && pVideoDec->bIsPowerControl == OMX_FALSE) {
            Rockchip_OSAL_PowerControl(pRockchipComponent, 3840, 2160, pVideoDec->bIsHevc,
                                           pInputPort->portDefinition.format.video.xFramerate,
                                           OMX_TRUE,
                                           10);
            pVideoDec->bIsPowerControl = OMX_TRUE;
        }
        pVideoDec->bIs10bit = OMX_TRUE;
    }

    return eColorFormat;
}

OMX_U32 Rockchip_OSAL_GetVideoNativeMetaSize() {
    return sizeof(VideoNativeMetadata);
}

#ifdef __cplusplus
}
#endif
