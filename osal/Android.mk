LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(PLATFORM_VERSION),4.4.4) 
BOARD_VERSION_LOW := true
endif
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Rockchip_OSAL_Event.c \
	Rockchip_OSAL_Queue.c \
	Rockchip_OSAL_ETC.c \
	Rockchip_OSAL_Mutex.c \
	Rockchip_OSAL_Thread.c \
	Rockchip_OSAL_Memory.c \
	Rockchip_OSAL_Semaphore.c \
	Rockchip_OSAL_Library.c \
	Rockchip_OSAL_Log.c\
	Rockchip_OSAL_RGA_Process.c \
	Rockchip_OSAL_Android.cpp \
	Rockchip_OSAL_SharedMemory.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libRkOMX_OSAL

LOCAL_SHARED_LIBRARIES := libhardware
LOCAL_STATIC_LIBRARIES := liblog libcutils

ifeq ($(BOARD_VERSION_LOW),true)
LOCAL_CFLAGS += -DLOW_VRESION
endif

ifeq ($(BOARD_CONFIG_3GR),true)
      LOCAL_CFLAGS += -DROCKCHIP_GPU_LIB_ENABLE
endif

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 7.0)))
     LOCAL_CFLAGS += -DUSE_ANW
endif

LOCAL_C_INCLUDES := $(ROCKCHIP_OMX_INC)/khronos \
	$(ROCKCHIP_OMX_INC)/rockchip \
	$(ROCKCHIP_OMX_TOP)/osal \
	$(ROCKCHIP_OMX_COMPONENT)/common \
	$(ROCKCHIP_OMX_COMPONENT)/video/dec \
	$(ROCKCHIP_OMX_COMPONENT)/video/enc \
	$(TOP)/frameworks/native/include/media/hardware \
	$(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/system/core/libion/include \
        $(TOP)/system/core/libion/kernel-headers \
	$(TOP)/hardware/rockchip/libgralloc \
	$(TOP)/hardware/rockchip/librkvpu \
	$(TOP)/hardware/rockchip/librkvpu/omx_get_gralloc_private

ifeq ($(OMX_USE_DRM), true)
	LOCAL_CFLAGS += -DUSE_DRM
	LOCAL_C_INCLUDES +=  $(TOP)/hardware/rockchip/librga \
			     $(TOP)/external/libdrm/include/drm/ 
endif

include $(BUILD_STATIC_LIBRARY)
