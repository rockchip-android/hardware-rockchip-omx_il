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
 * @file        Rkvpu_OMX_Vdec.h
 * @brief
 * @author      Csy (csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2013.11.28 : Create
 */

#ifndef Rkvpu_OMX_VIDEO_DECODE
#define Rkvpu_OMX_VIDEO_DECODE

#include "OMX_Component.h"
#include "Rockchip_OMX_Def.h"
#include "Rockchip_OSAL_Queue.h"
#include "Rockchip_OMX_Baseport.h"
#include "Rockchip_OMX_Basecomponent.h"
#include "OMX_Video.h"
#include "vpu_api.h"

#define MAX_VIDEO_INPUTBUFFER_NUM           4
#define MAX_VIDEO_OUTPUTBUFFER_NUM          2

#define DEFAULT_FRAME_WIDTH                 1920
#define DEFAULT_FRAME_HEIGHT                1080

#define DEFAULT_VIDEO_INPUT_BUFFER_SIZE    (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT)/2
#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE   (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT * 3) / 2

#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX    1

//#define WRITR_FILE

typedef struct {
    void *pAddrY;
    void *pAddrC;
} CODEC_DEC_ADDR_INFO;

typedef struct _CODEC_DEC_BUFFER {
    void *pVirAddr[MAX_BUFFER_PLANE];   /* virtual address   */
    int   bufferSize[MAX_BUFFER_PLANE]; /* buffer alloc size */
    int   fd[MAX_BUFFER_PLANE];         /* buffer FD */
    int   dataSize;                     /* total data length */
} CODEC_DEC_BUFFER;

typedef struct _DECODE_CODEC_EXTRA_BUFFERINFO {
    /* For Decode Output */
    OMX_U32 imageWidth;
    OMX_U32 imageHeight;
    OMX_COLOR_FORMATTYPE ColorFormat;
    //PrivateDataShareBuffer PDSB;
} DECODE_CODEC_EXTRA_BUFFERINFO;


typedef enum _RKVPU_OMX_VDEC_FLAG_MAP {
    RKVPU_OMX_VDEC_NONE         = 0,
    RKVPU_OMX_VDEC_IS_DIV3      = 0x01,
    RKVPU_OMX_VDEC_USE_DTS      = 0x02,
    RKVPU_OMX_VDEC_THUMBNAIL    = 0x04,
    RKVPU_OMX_VDEC_BUTT,
} RKVPU_OMX_VDEC_FLAG_MAP;

typedef struct _RKVPU_OMX_VIDEODEC_COMPONENT {
    OMX_HANDLETYPE hCodecHandle;
    OMX_BOOL bThumbnailMode;
    OMX_BOOL bFirstFrame;
    OMX_VIDEO_PARAM_AVCTYPE AVCComponent[ALL_PORT_NUM];

    /* Buffer Process */
    OMX_BOOL       bExitBufferProcessThread;
    OMX_HANDLETYPE hInputThread;
    OMX_HANDLETYPE hOutputThread;

    OMX_VIDEO_CODINGTYPE codecId;

    /* Shared Memory Handle */
    OMX_HANDLETYPE hSharedMemory;

    /* For DRM Play */
    OMX_BOOL bDRMPlayerMode;

    /* For Reconfiguration DPB */
    OMX_BOOL bReconfigDPB;

    /* CSC handle */
    OMX_PTR csc_handle;
    OMX_U32 csc_set_format;

    OMX_HANDLETYPE hRefHandle;

    VpuCodecContext_t *vpu_ctx;
    OMX_U32 flags;

    OMX_BOOL bIsANBEnabled;

    void *rga_ctx;

    OMX_BOOL bDecSendEOS;

    OMX_BOOL bStoreMetaData;
    OMX_BOOL bPvr_Flag;
    OMX_PTR  vpumem_handle;
    OMX_U32 maxCount; // when buffer in AL big than 8,if max timeout no consume we continue send one buffer to AL
    OMX_BOOL bOld_api;
    /* For vpu control */
    OMX_BOOL bFastMode;
    OMX_RK_EXT_COLORSPACE extColorSpace;
    OMX_RK_EXT_DYNCRANGE extDyncRange;

#ifdef WRITR_FILE
    FILE *fp_out;
#endif
    /* For debug */
    FILE *fp_in;
    OMX_BOOL bPrintFps;
    void *rkapi_hdl;
    OMX_S32 (*rkvpu_open_cxt)(VpuCodecContext_t **ctx);
    OMX_S32 (*rkvpu_close_cxt)(VpuCodecContext_t **ctx);

    OMX_ERRORTYPE (*Rkvpu_codec_srcInputProcess) (OMX_COMPONENTTYPE *pOMXComponent, ROCKCHIP_OMX_DATA *pInputData);
    OMX_ERRORTYPE (*Rkvpu_codec_srcOutputProcess) (OMX_COMPONENTTYPE *pOMXComponent, ROCKCHIP_OMX_DATA *pInputData);
    OMX_ERRORTYPE (*Rkvpu_codec_dstInputProcess) (OMX_COMPONENTTYPE *pOMXComponent, ROCKCHIP_OMX_DATA *pOutputData);
    OMX_ERRORTYPE (*Rkvpu_codec_dstOutputProcess) (OMX_COMPONENTTYPE *pOMXComponent, ROCKCHIP_OMX_DATA *pOutputData);

} RKVPU_OMX_VIDEODEC_COMPONENT;

#ifdef __cplusplus
extern "C" {
#endif

int calc_plane(int width, int height);
void UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent);
OMX_BOOL Rkvpu_Check_BufferProcess_State(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_U32 nPortIndex);

OMX_ERRORTYPE Rkvpu_OMX_InputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Rkvpu_OMX_OutputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Rkvpu_Dec_ComponentInit(OMX_COMPONENTTYPE *pOMXComponent);
OMX_ERRORTYPE Rkvpu_Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent);

OMX_ERRORTYPE Rockchip_OMX_ComponentConstructor(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
OMX_ERRORTYPE Rockchip_OMX_ComponentDeInit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
}
#endif

#endif
