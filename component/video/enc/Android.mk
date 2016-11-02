LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	Rkvpu_OMX_VencControl.c \
	Rkvpu_OMX_Venc.c \
	library_register.c

LOCAL_MODULE := libomxvpu_enc
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(ROCKCHIP_OMX_INC)/khronos \
	$(ROCKCHIP_OMX_INC)/rockchip \
	$(ROCKCHIP_OMX_TOP)/osal \
	$(ROCKCHIP_OMX_TOP)/core \
	$(ROCKCHIP_OMX_COMPONENT)/common \
	$(ROCKCHIP_OMX_COMPONENT)/video/enc \
	$(TOP)/hardware/rockchip/librkvpu \
	$(TOP)/hardware/rockchip/librkvpu/omx_get_gralloc_private	

LOCAL_STATIC_LIBRARIES := libRkOMX_OSAL \
	  	libRkOMX_Basecomponent 
	
LOCAL_SHARED_LIBRARIES := libc \
	 libdl \
	 libcutils \
	 libutils \
	 liblog \
	 libui \
	 libRkOMX_Resourcemanager \
	 libhardware \
	 libvpu \
         libgralloc_priv_omx 
	
ifeq ($(BOARD_USE_ANB), true)
LOCAL_CFLAGS += -DUSE_ANB
endif

ifeq ($(BOARD_USE_STOREMETADATA), true)
LOCAL_CFLAGS += -DUSE_STOREMETADATA
endif

ifeq ($(BOARD_CONFIG_3GR),true)
	LOCAL_CFLAGS += -DSOFIA_3GR \
			-DROCKCHIP_GPU_LIB_ENABLE              
	
endif


ifeq ($(OMX_USE_DRM), true)
LOCAL_SHARED_LIBRARIES += librga
endif

include $(BUILD_SHARED_LIBRARY)
