LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Rockchip_OMX_Basecomponent.c \
	Rockchip_OMX_Baseport.c

LOCAL_MODULE := libRkOMX_Basecomponent

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES := libRkOMX_OSAL
LOCAL_SHARED_LIBRARIES := libcutils libutils liblog

LOCAL_C_INCLUDES := $(ROCKCHIP_OMX_INC)/khronos \
	$(ROCKCHIP_OMX_INC)/rockchip \
	$(ROCKCHIP_OMX_TOP)/osal

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Rockchip_OMX_Resourcemanager.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libRkOMX_Resourcemanager

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES := libRkOMX_OSAL
LOCAL_SHARED_LIBRARIES := libcutils libutils liblog

LOCAL_C_INCLUDES := $(ROCKCHIP_OMX_INC)/khronos \
	$(ROCKCHIP_OMX_INC)/rockchip \
	$(ROCKCHIP_OMX_TOP)/osal

include $(BUILD_SHARED_LIBRARY)
