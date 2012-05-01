LOCAL_PATH := $(call my-dir)

A3D_CLIENT_VERSION := A3D_GLESv1_CM

# include libraries in correct order
include $(LOCAL_PATH)/texgz/Android.mk
include $(LOCAL_PATH)/a3d/Android.mk

include $(CLEAR_VARS)
LOCAL_MODULE    := LaserShark
LOCAL_CFLAGS    := -Wall -D$(A3D_CLIENT_VERSION)
LOCAL_SRC_FILES := android_jni.c
LOCAL_LDLIBS    := -Llibs/armeabi \
                   -llog -la3d -ltexgz

include $(BUILD_SHARED_LIBRARY)
