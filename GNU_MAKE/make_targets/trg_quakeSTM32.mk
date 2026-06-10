FORCE_32     := 1

OPT_LVL ?= 3
OPT_LVL ?= 0

include features/fh_qEngine.mk
include features/fh_MCU_platform.mk

MCU_FLAGS = -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
CFLAGS += $(MCU_FLAGS)
CFLAGS += \
    -std=gnu11 \
    -g3 -DDEBUG -DUSE_HAL_DRIVER \
    -DSTM32F769xx -c -O$(OPT_LVL) \
    -ffunction-sections \
    -fdata-sections -Wall \
    -fstack-usage \
    -MMD -MP \

CXXFLAGS += $(MCU_FLAGS)
CXXFLAGS += \
    -g3 -DDEBUG -DUSE_HAL_DRIVER \
    -DSTM32F769xx -c -O$(OPT_LVL) \
    -ffunction-sections \
    -fdata-sections -Wall \
    -fstack-usage \
    -MMD -MP \

LDFLAGS += $(MCU_FLAGS)
LDFLAGS += \
    -T"$(STMSRC_DIR)/STM32F769NIHX_FLASH.ld" \
    -Wl,-Map="stm32f7q1.map" \
    -Wl,--gc-sections -static \
    -Wl,--start-group \
    -lc -lm -lstdc++ -lsupc++ \
    -Wl,--end-group

#     -fcyclomatic-complexity \
