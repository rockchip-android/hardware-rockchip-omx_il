LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Rockchip_OMX_Component_Register.c \
	Rockchip_OMX_Core.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX_Core
LOCAL_PROPRIETARY_MODULE := true

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libRkOMX_OSAL libRkOMX_Basecomponent
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils liblog \
	libRkOMX_Resourcemanager

LOCAL_C_INCLUDES := $(ROCKCHIP_OMX_INC)/khronos \
	$(ROCKCHIP_OMX_INC)/rockchip \
	$(ROCKCHIP_OMX_TOP)/osal \
	$(ROCKCHIP_OMX_TOP)/component/common \
	$(TOP)/hardware/rk29/librkvpu \

include $(BUILD_SHARED_LIBRARY)
