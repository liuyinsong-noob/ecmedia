LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= srtp
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES :=                     \
        $(LOCAL_PATH)                           \
        $(LOCAL_PATH)/.. \
        $(LOCAL_PATH)/include           \
        $(LOCAL_PATH)/libfaad           \
        $(LOCAL_PATH)/common/mp4ff      \
        $(LOCAL_PATH)/libfaad/codebook

LOCAL_SRC_FILES :=                              \
	./srtp/ekt.c \
        ./srtp/srtp.c \
        ./crypto/cipher/aes.c \
        ./crypto/cipher/aes_cbc.c \
        ./crypto/cipher/aes_icm.c \
        ./crypto/cipher/cipher.c \
        ./crypto/cipher/null_cipher.c \
        ./crypto/hash/auth.c \
        ./crypto/hash/hmac.c \
        ./crypto/hash/null_auth.c \
        ./crypto/hash/sha1.c \
        ./crypto/kernel/alloc.c \
        ./crypto/kernel/crypto_kernel.c \
        ./crypto/kernel/err.c \
        ./crypto/kernel/key.c \
        ./crypto/math/datatypes.c \
        ./crypto/math/gf2_8.c \
        ./crypto/math/stat.c \
        ./crypto/replay/rdb.c \
        ./crypto/replay/rdbx.c \
        ./crypto/replay/ut_sim.c \
        ./crypto/rng/ctr_prng.c \
        ./crypto/rng/prng.c \
        ./crypto/rng/rand_source.c

LOCAL_CFLAGS := \
        -DHAVE_STDLIB_H \
        -DHAVE_STRING_H \
        -DSIZEOF_UNSIGNED_LONG=4 \
        -DSIZEOF_UNSIGNED_LONG_LONG=8 \
        -DHAVE_STDINT_H \
        -DHAVE_INTTYPES_H \
        -DHAVE_NETINET_IN_H \
        -DHAVE_UINT64_T \
        -DHAVE_UINT32_T \
        -DHAVE_UINT16_T \
        -DHAVE_UINT8_T \
        -DHAVE_UINT_T \
	-DHAVE_CONFIG_H

LOCAL_CFLAGS += -DCPU_CISC

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/crypto/include

LOCAL_LDLIBS := -llog

include $(BUILD_STATIC_LIBRARY)
