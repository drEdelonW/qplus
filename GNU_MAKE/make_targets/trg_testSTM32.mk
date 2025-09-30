DST_PLATFORM := STM32

# INCLUDES += $(SRC_DIR)
# INCLUDES += /opt/homebrew/opt/libx11/include
# INCLUDES += /opt/homebrew/include

# include features/fh_qEngine.mk

$(eval ROOT_DIR := $(STMSRC_DIR)) $(eval INCLUDES += $(ROOT_DIR))
    $(eval CORE_DIR := $(ROOT_DIR)/Core) $(eval INCLUDES += $(CORE_DIR)/Inc)
    $(eval HAL_DIR := $(ROOT_DIR)/Drivers/STM32F7xx_HAL_Driver) $(eval INCLUDES += $(HAL_DIR)/Inc)
    $(eval RTOS_DIR := $(ROOT_DIR)/Middlewares/Third_Party/FreeRTOS/Source) $(eval INCLUDES += $(RTOS_DIR)/Inc)
        SRC_LIST += $(RTOS_DIR)/portable/MemMang/heap_4.c
        SRC_LIST += $(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1/port.c
#         SRC_LIST += $(VECMAT_DIR)/Vector3d.cpp


CFLAGS += \
    -mcpu=cortex-m7 \
    -std=gnu11 \
    -g3 \
    -DDEBUG \
    -DUSE_HAL_DRIVER \
    -DSTM32F769xx \
    -c \
    -O0 \
    -ffunction-sections \
    -fdata-sections \
    -Wall \
    -fstack-usage \
    -fcyclomatic-complexity \
    -MMD \
    -MP \
    --specs=nano.specs \
    -mfpu=fpv5-d16 \
    -mfloat-abi=hard \
    -mthumb