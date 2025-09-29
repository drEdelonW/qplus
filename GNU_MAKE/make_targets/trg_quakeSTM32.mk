DST_PLATFORM := STM32

INCLUDES += $(SRC_DIR)
# INCLUDES += /opt/homebrew/opt/libx11/include
# INCLUDES += /opt/homebrew/include

include features/fh_qEngine.mk

# $(eval VECMAT_DIR := $(SRC_DIR)/vectorMath) $(eval INCLUDES += $(VECMAT_DIR))
#         SRC_LIST += $(VECMAT_DIR)/Vector3d.cpp

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR)/API)
        $(eval MCU_DIR = $(PLATFORM_DIR)/MCU) $(eval INCLUDES += $(MCU_DIR))
                SRC_LIST += $(MCU_DIR)/cd_null.c
                SRC_LIST += $(MCU_DIR)/snd_null.c
                SRC_LIST += $(MCU_DIR)/sys_null.c
                SRC_LIST += $(MCU_DIR)/vid_null.c
                SRC_LIST += $(MCU_DIR)/net_mcu.c
                SRC_LIST += $(MCU_DIR)/in_mcu.c

# $(eval OTHER_DIR = $(SRC_DIR)/etc) $(eval INCLUDES += $(OTHER_DIR))

# DEFINES += __STM32__