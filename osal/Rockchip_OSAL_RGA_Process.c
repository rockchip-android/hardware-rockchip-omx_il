#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include "vpu_type.h"
#include "Rockchip_OMX_Def.h"
#include "vpu_global.h"
#include "Rockchip_OSAL_Log.h"
#include "Rockchip_OSAL_Memory.h"
#include "Rockchip_OSAL_RGA_Process.h"
#include "hardware/rga.h"
#ifdef USE_DRM
#include "drmrga.h"
#include "RgaApi.h"
#include <drm_fourcc.h>
#else
typedef struct _rga_ctx {
    int32_t rga_fd;
} rga_ctx_t;
#endif

OMX_S32 rga_dev_open(void **rga_ctx)
{
#ifndef USE_DRM
    rga_ctx_t *ctx = NULL;
    ctx = Rockchip_OSAL_Malloc(sizeof(rga_ctx_t));
    Rockchip_OSAL_Memset(ctx, 0, sizeof(rga_ctx_t));
    ctx->rga_fd = -1;
    ctx->rga_fd = open("/dev/rga", O_RDWR, 0);
    if (ctx->rga_fd < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga open fail");
        return -1;
    }
    *rga_ctx = ctx;
    return 0;
#else
    RgaInit(rga_ctx);
    if (*rga_ctx == NULL) {
        return -1;
    }
    return 0;
#endif
}

OMX_S32 rga_dev_close(void *rga_ctx)
{
#ifndef USE_DRM
    rga_ctx_t *ctx = (rga_ctx_t *)rga_ctx;
    if (ctx == NULL) {
        return 0;
    }
    if ( ctx->rga_fd >= 0) {
        close(ctx->rga_fd);
        ctx->rga_fd = -1;
    }
    Rockchip_OSAL_Free(rga_ctx);
    rga_ctx = NULL;
    return 0;
#else
    RgaDeInit(rga_ctx);
    return 0;
#endif
}

OMX_S32 rga_copy(RockchipVideoPlane *plane, VPUMemLinear_t *vpumem, uint32_t Width, uint32_t Height, int format, int rga_fd)
{
    struct rga_req  Rga_Request;
    Rockchip_OSAL_Memset(&Rga_Request, 0x0, sizeof(Rga_Request));
#ifdef SOFIA_3GR
    Rga_Request.line_draw_info.color = plane->fd & 0xffff;
#else
    Rga_Request.src.yrgb_addr = plane->fd;
#endif
    Rga_Request.src.uv_addr  = 0;
    Rga_Request.src.v_addr   =  0;
    Rga_Request.src.vir_w = plane->stride;
    Rga_Request.src.vir_h = Height;

    Rga_Request.src.format = format;

    Rga_Request.src.act_w = Width;
    Rga_Request.src.act_h = Height;
    Rga_Request.src.x_offset = 0;
    Rga_Request.src.y_offset = 0;
    Rga_Request.dst.yrgb_addr = 0;
#ifdef SOFIA_3GR
    if (!VPUMemJudgeIommu()) {
        Rga_Request.dst.yrgb_addr  = vpumem->phy_addr;
    } else {
        Rga_Request.line_draw_info.color |= (vpumem->phy_addr & 0xffff) << 16;
        Rga_Request.dst.uv_addr  = vpumem->vir_addr;
    }

#else
    if (!VPUMemJudgeIommu()) {
        Rga_Request.dst.uv_addr  = vpumem->phy_addr;
    } else {
        Rga_Request.dst.yrgb_addr = vpumem->phy_addr;
        Rga_Request.dst.uv_addr  = (OMX_U32)vpumem->vir_addr;
    }
#endif
    Rga_Request.dst.v_addr   = 0;
    Rga_Request.dst.vir_w = Width;
    Rga_Request.dst.vir_h = Height;
    Rga_Request.dst.format = Rga_Request.src.format;

    Rga_Request.clip.xmin = 0;
    Rga_Request.clip.xmax = Width - 1;
    Rga_Request.clip.ymin = 0;
    Rga_Request.clip.ymax = Height - 1;

    Rga_Request.dst.act_w = Width;
    Rga_Request.dst.act_h = Height;
    Rga_Request.dst.x_offset = 0;
    Rga_Request.dst.y_offset = 0;
    Rga_Request.rotate_mode = 0;
    Rga_Request.render_mode = 5;
    if (plane->type == ANB_PRIVATE_BUF_VIRTUAL) {
        Rga_Request.src.uv_addr = (OMX_U32)plane->addr;
        Rga_Request.mmu_info.mmu_en = 1;
        Rga_Request.mmu_info.mmu_flag = ((2 & 0x3) << 4) | 1;
        if (VPUMemJudgeIommu()) {
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 10) | (1 << 8));
        } else {
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 8));
        }
    } else {
        if (VPUMemJudgeIommu()) {
            Rga_Request.mmu_info.mmu_en = 1;
            Rga_Request.mmu_info.mmu_flag = ((2 & 0x3) << 4) | 1;
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 10) | (1 << 8));
        }
    }
    Rockchip_OSAL_Log(ROCKCHIP_LOG_INFO, "rga start in");
    if (ioctl(rga_fd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga rga_copy fail");
        return -1;
    }
    Rockchip_OSAL_Log(ROCKCHIP_LOG_INFO, "rga start out");
    return 0;
}

OMX_S32 rga_crop_scale(RockchipVideoPlane *plane,
                       VPUMemLinear_t *vpumem, OMX_VIDEO_PARAMS_EXTENDED *param_video,
                       RK_U32 orgin_w, RK_U32 orgin_h, int rga_fd)
{
    struct rga_req  Rga_Request;
    RK_U32 new_width = 0, new_height = 0;
    Rockchip_OSAL_Memset(&Rga_Request, 0x0, sizeof(Rga_Request));
    if (param_video->bEnableScaling || param_video->bEnableCropping) {
        if (param_video->bEnableScaling) {
            new_width = param_video->ui16ScaledWidth;
            new_height = param_video->ui16ScaledHeight;
        } else if (param_video->bEnableCropping) {
            new_width = orgin_w - param_video->ui16CropLeft - param_video->ui16CropRight;
            new_height = orgin_h - param_video->ui16CropTop - param_video->ui16CropBottom;
        }
    }
#ifdef SOFIA_3GR
    Rga_Request.line_draw_info.color = plane->fd & 0xffff;
#else
    Rga_Request.src.yrgb_addr = plane->fd;
#endif

    Rga_Request.src.uv_addr  = 0;
    Rga_Request.src.v_addr   =  0;
    Rga_Request.src.vir_w = plane->stride;
    Rga_Request.src.vir_h = orgin_h;

    Rga_Request.src.format = RK_FORMAT_YCbCr_420_SP;
    if (param_video->bEnableCropping) {
        Rga_Request.src.act_w = orgin_w - param_video->ui16CropLeft - param_video->ui16CropRight;
        Rga_Request.src.act_h = orgin_h - param_video->ui16CropTop - param_video->ui16CropBottom;
        Rga_Request.src.x_offset = param_video->ui16CropLeft;
        Rga_Request.src.y_offset = param_video->ui16CropTop;
    } else {
        Rga_Request.src.act_w = orgin_w;
        Rga_Request.src.act_h = orgin_h;
        Rga_Request.src.x_offset = 0;
        Rga_Request.src.y_offset = 0;
    }

    Rga_Request.dst.yrgb_addr = 0;

#ifdef SOFIA_3GR
    if (!VPUMemJudgeIommu()) {
        Rga_Request.dst.yrgb_addr = vpumem->phy_addr;
        Rga_Request.dst.uv_addr  = vpumem->phy_addr + plane->stride * orgin_h;
    } else {
        Rga_Request.line_draw_info.color |= (vpumem->phy_addr & 0xffff) << 16;
        Rga_Request.dst.uv_addr  = (OMX_U32)vpumem->vir_addr;
    }
#else
    if (!VPUMemJudgeIommu()) {
        Rga_Request.dst.uv_addr  = vpumem->phy_addr;
    } else {
        Rga_Request.dst.yrgb_addr = vpumem->phy_addr;
        Rga_Request.dst.uv_addr  = (OMX_U32)vpumem->vir_addr;
    }
#endif
    Rga_Request.dst.v_addr   = 0;
    Rga_Request.dst.vir_w = new_width;
    Rga_Request.dst.vir_h = new_height;
    Rga_Request.dst.format = RK_FORMAT_YCbCr_420_SP;

    Rga_Request.clip.xmin = 0;
    Rga_Request.clip.xmax = new_width - 1;
    Rga_Request.clip.ymin = 0;
    Rga_Request.clip.ymax = new_height - 1;

    Rga_Request.dst.act_w = new_width;
    Rga_Request.dst.act_h = new_height;
    Rga_Request.dst.x_offset = 0;
    Rga_Request.dst.y_offset = 0;
    Rga_Request.rotate_mode = 1;
    Rga_Request.sina = 0;
    Rga_Request.cosa = 65536;

    if (plane->type == ANB_PRIVATE_BUF_VIRTUAL) {
        Rga_Request.src.uv_addr = (OMX_U32)plane->addr;
        Rga_Request.mmu_info.mmu_en = 1;
        Rga_Request.mmu_info.mmu_flag = ((2 & 0x3) << 4) | 1;
        if (VPUMemJudgeIommu()) {
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 10) | (1 << 8));
        } else {
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 8));
        }
    } else {
        if (VPUMemJudgeIommu()) {
            Rga_Request.mmu_info.mmu_en = 1;
            Rga_Request.mmu_info.mmu_flag = ((2 & 0x3) << 4) | 1;
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 10) | (1 << 8));
        }
    }

    Rockchip_OSAL_Log(ROCKCHIP_LOG_INFO, "rga start in");

    if (ioctl(rga_fd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga_rgb2nv12 rga RGA_BLIT_SYNC fail");
        return -1;
    }
    Rockchip_OSAL_Log(ROCKCHIP_LOG_INFO, "rga start out");
    return 0;
}

OMX_S32 rga_convert(
    RockchipVideoPlane *vplanes,
    VPUMemLinear_t *vpumem,
    uint32_t mWidth,
    uint32_t mHeight,
    int src_format,
    int dst_format,
    int rga_fd)
{

    if (rga_fd < 0) {
        return -1;
    }
    int32_t Width  = (mWidth + 15) & (~15);
    int32_t Height = (mHeight + 15) & (~15);
    struct rga_req  Rga_Request;
    Rockchip_OSAL_Memset(&Rga_Request, 0x0, sizeof(Rga_Request));
    Rga_Request.src.yrgb_addr =  0;
#ifdef SOFIA_3GR
    if (!VPUMemJudgeIommu()) {
        Rga_Request.src.yrgb_addr  = vpumem->phy_addr;
        Rga_Request.src.uv_addr  = vpumem->phy_addr + Width * Height;
    } else {
        Rga_Request.line_draw_info.color = vpumem->phy_addr & 0xffff;
        Rga_Request.src.yrgb_addr = vpumem->phy_addr;
        Rga_Request.src.uv_addr  = vpumem->vir_addr;
    }
#else
    if (!VPUMemJudgeIommu()) {
        Rga_Request.src.uv_addr  = vpumem->phy_addr;
    } else {
        Rga_Request.src.yrgb_addr = vpumem->phy_addr;
        Rga_Request.src.uv_addr  = (OMX_U32)vpumem->vir_addr;
    }
#endif
    Rga_Request.src.v_addr   = 0;
    Rga_Request.src.vir_w = Width;
    Rga_Request.src.vir_h = Height;
    Rga_Request.src.format = src_format;

    Rga_Request.src.act_w = Width;
    Rga_Request.src.act_h = Height;
    Rga_Request.src.x_offset = 0;
    Rga_Request.src.y_offset = 0;
#ifdef SOFIA_3GR
    Rga_Request.line_draw_info.color |= (vplanes->fd & 0xffff) << 16;
#else
    Rga_Request.dst.yrgb_addr = vplanes->fd;
#endif
    Rga_Request.dst.uv_addr  = 0;
    Rga_Request.dst.v_addr   = 0;
    Rga_Request.dst.vir_w = Width;
    Rga_Request.dst.vir_h = Height;
    Rga_Request.dst.format = dst_format;
    Rga_Request.clip.xmin = 0;
    Rga_Request.clip.xmax = Width - 1;
    Rga_Request.clip.ymin = 0;
    Rga_Request.clip.ymax = Height - 1;
    Rga_Request.dst.act_w = Width;
    Rga_Request.dst.act_h = Height;
    Rga_Request.dst.x_offset = 0;
    Rga_Request.dst.y_offset = 0;
    Rga_Request.rotate_mode = 0;
    Rga_Request.yuv2rgb_mode = 0;
    if (vplanes->type == ANB_PRIVATE_BUF_VIRTUAL) {
        Rga_Request.dst.uv_addr  =  (OMX_U32)(vplanes->addr);
        Rga_Request.mmu_info.mmu_en = 1;
        Rga_Request.mmu_info.mmu_flag  = ((2 & 0x3) << 4) | 1;
        if (VPUMemJudgeIommu()) {
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 10) | (1 << 8));
        } else {
            Rga_Request.mmu_info.mmu_flag |= ((1 << 31) | (1 << 10));
        }
    } else {
        if (VPUMemJudgeIommu()) {
            Rga_Request.mmu_info.mmu_en    = 1;
            Rga_Request.mmu_info.mmu_flag  = ((2 & 0x3) << 4) | 1;
            Rga_Request.mmu_info.mmu_flag |= (1 << 31) | (1 << 10) | (1 << 8);
        }
    }
    if (ioctl(rga_fd, RGA_BLIT_SYNC, &Rga_Request) != 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga_convert fail");
        return -1;
    }
    return 0;
}

void rga_nv12_crop_scale(RockchipVideoPlane *plane,
                         VPUMemLinear_t *vpumem, OMX_VIDEO_PARAMS_EXTENDED *param_video,
                         RK_U32 orgin_w, RK_U32 orgin_h, void* rga_ctx)
{
#ifndef USE_DRM
    rga_ctx_t *ctx = (rga_ctx_t *)rga_ctx;
    if (ctx == NULL) {
        return;
    }
    if (rga_crop_scale(plane, vpumem, param_video, orgin_w, orgin_h, ctx->rga_fd) < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga_crop_scale fail");
    }
#else
    RK_U32 new_width = 0, new_height = 0;
    rga_info_t src;
    rga_info_t dst;
    (void) rga_ctx;
    memset((void*)&src.rect, 0, sizeof(rga_info_t));
    memset((void*)&dst.rect, 0, sizeof(rga_info_t));
    if (param_video->bEnableScaling || param_video->bEnableCropping) {
        if (param_video->bEnableScaling) {
            new_width = param_video->ui16ScaledWidth;
            new_height = param_video->ui16ScaledHeight;
        } else if (param_video->bEnableCropping) {
            new_width = orgin_w - param_video->ui16CropLeft - param_video->ui16CropRight;
            new_height = orgin_h - param_video->ui16CropTop - param_video->ui16CropBottom;
        }
    }

    if (param_video->bEnableCropping) {
        RK_U32 x, y, w, h;
        w = orgin_w - param_video->ui16CropLeft - param_video->ui16CropRight;
        h = orgin_h - param_video->ui16CropTop - param_video->ui16CropBottom;
        x = param_video->ui16CropLeft;
        y = param_video->ui16CropTop;
        rga_set_rect(&src.rect, x, y, w, h, plane->stride, DRM_FORMAT_NV12);
    } else {
        rga_set_rect(&src.rect, 0, 0, orgin_w, orgin_h, plane->stride, DRM_FORMAT_NV12);

    }
    rga_set_rect(&dst.rect, 0, 0, orgin_w, orgin_h, orgin_w, DRM_FORMAT_NV12);
    src.fd = plane->fd;
    dst.fd = vpumem->phy_addr;
    RgaBlit(&src, &dst, NULL);
#endif
}

void rga_rgb2nv12(RockchipVideoPlane *plane, VPUMemLinear_t *vpumem,
                  uint32_t Width, uint32_t Height, void* rga_ctx)
{

#ifndef USE_DRM
    int src_format =  RK_FORMAT_RGBA_8888;
    int dst_format =  RK_FORMAT_YCbCr_420_SP;

    rga_ctx_t *ctx = (rga_ctx_t *)rga_ctx;
    if (ctx == NULL) {
        return;
    }
    if (rga_convert(plane, vpumem, Width, Height, src_format, dst_format, ctx->rga_fd) < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga_rgb2nv12 fail");
    }
#else
    rga_info_t src;
    rga_info_t dst;
    (void) rga_ctx;
    memset((void*)&src.rect, 0, sizeof(rga_info_t));
    memset((void*)&dst.rect, 0, sizeof(rga_info_t));
    rga_set_rect(&src.rect, 0, 0, Width, Height, (Width + 15) & (~15), DRM_FORMAT_RGBA8888);
    rga_set_rect(&dst.rect, 0, 0, Width, Height, Width, DRM_FORMAT_NV12);
    src.fd = plane->fd;
    dst.fd = vpumem->phy_addr;
    RgaBlit(&src, &dst, NULL);
#endif
}

void rga_nv122rgb( RockchipVideoPlane *planes, VPUMemLinear_t *vpumem, uint32_t Width, uint32_t Height, int dst_format, void* rga_ctx)
{
#ifndef USE_DRM
    int src_format =  RK_FORMAT_YCbCr_420_SP;
    rga_ctx_t *ctx = (rga_ctx_t *)rga_ctx;
    if (ctx == NULL) {
        return;
    }
    if (rga_convert(planes, vpumem, Width, Height, src_format, dst_format, ctx->rga_fd) < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga_nv122rgb fail");
    }
#else
    rga_info_t src;
    rga_info_t dst;
    int32_t format;
    (void) rga_ctx;
    memset((void*)&src.rect, 0, sizeof(rga_info_t));
    memset((void*)&dst.rect, 0, sizeof(rga_info_t));
    rga_set_rect(&src.rect, 0, 0, Width, Height, (Width + 15) & (~15), DRM_FORMAT_NV12);
    if (dst_format == RK_FORMAT_BGRA_8888) {
        dst_format = DRM_FORMAT_BGRA8888;
    } else if (dst_format == RK_FORMAT_RGBA_8888) {
        dst_format = DRM_FORMAT_RGBA8888;
    }
    rga_set_rect(&dst.rect, 0, 0, Width, Height, planes->stride, dst_format);
    src.fd = planes->fd;
    dst.fd = vpumem->phy_addr;
    RgaBlit(&src, &dst, NULL);
#endif
}

void rga_nv12_copy(RockchipVideoPlane *plane, VPUMemLinear_t *vpumem, uint32_t Width, uint32_t Height, void* rga_ctx)
{
#ifndef USE_DRM
    int format =  RK_FORMAT_YCbCr_420_SP;
    rga_ctx_t *ctx = (rga_ctx_t *)rga_ctx;
    if (ctx == NULL) {
        return;
    }

    if (rga_copy(plane, vpumem, Width, Height, format, ctx->rga_fd) < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga_nv12_copy fail");
    }
#else
    rga_info_t src;
    rga_info_t dst;
    (void) rga_ctx;
    memset((void*)&src.rect, 0, sizeof(rga_info_t));
    memset((void*)&dst.rect, 0, sizeof(rga_info_t));
    rga_set_rect(&src.rect, 0, 0, Width, Height, plane->stride, DRM_FORMAT_NV12);
    rga_set_rect(&dst.rect, 0, 0, Width, Height, Width, DRM_FORMAT_NV12);
    src.fd = plane->fd;
    dst.fd = vpumem->phy_addr;
    RgaBlit(&src, &dst, NULL);

#endif
}
void rga_rgb_copy(RockchipVideoPlane *plane, VPUMemLinear_t *vpumem, uint32_t Width, uint32_t Height, void* rga_ctx)
{
#ifndef USE_DRM
    int format =  RK_FORMAT_RGBA_8888;
    rga_ctx_t *ctx = (rga_ctx_t *)rga_ctx;
    if (ctx == NULL) {
        return;
    }
    if (rga_copy(plane, vpumem, Width, Height, format, ctx->rga_fd) < 0) {
        Rockchip_OSAL_Log(ROCKCHIP_LOG_ERROR, "rga_nv12_copy fail");
    }
#else
    rga_info_t src;
    rga_info_t dst;
    (void) rga_ctx;
    memset((void*)&src.rect, 0, sizeof(rga_info_t));
    memset((void*)&dst.rect, 0, sizeof(rga_info_t));
    rga_set_rect(&src.rect, 0, 0, Width, Height, plane->stride, DRM_FORMAT_RGBA8888);
    rga_set_rect(&dst.rect, 0, 0, Width, Height, Width, DRM_FORMAT_RGBA8888);
    src.fd = plane->fd;
    dst.fd = vpumem->phy_addr;
    RgaBlit(&src, &dst, NULL);
#endif
}


