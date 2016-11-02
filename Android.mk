LOCAL_PATH := $(call my-dir)
BOARD_USE_ANB := true
BOARD_USE_STOREMETADATA := true

ifeq ($(filter sofia%, $(TARGET_BOARD_PLATFORM)), )
	BOARD_CONFIG_3GR := false
else
	BOARD_CONFIG_3GR := true
endif

ifeq ($(strip $(BOARD_USE_DRM)), true)
	OMX_USE_DRM := false
else
      	OMX_USE_DRM := false
endif

include $(CLEAR_VARS)

ROCKCHIP_OMX_TOP := $(LOCAL_PATH)

ROCKCHIP_OMX_INC := $(ROCKCHIP_OMX_TOP)/include/
ROCKCHIP_OMX_COMPONENT := $(ROCKCHIP_OMX_TOP)/component

include $(ROCKCHIP_OMX_TOP)/osal/Android.mk
include $(ROCKCHIP_OMX_TOP)/core/Android.mk

include $(ROCKCHIP_OMX_COMPONENT)/common/Android.mk
include $(ROCKCHIP_OMX_COMPONENT)/video/dec/Android.mk
include $(ROCKCHIP_OMX_COMPONENT)/video/enc/Android.mk


