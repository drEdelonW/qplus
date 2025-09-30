
include features/fh_qEngine.mk
include features/fh_MCU_platform.mk

CFLAGS += \
    -mcpu=cortex-m7 -std=gnu11 \
    -g3 -DDEBUG -DUSE_HAL_DRIVER \
    -DSTM32F769xx -c -O0 \
    -ffunction-sections \
    -fdata-sections -Wall \
    -fstack-usage \
    -fcyclomatic-complexity \
    -MMD -MP \
    --specs=nano.specs \
    -mfpu=fpv5-d16 \
    -mfloat-abi=hard \
    -mthumb

LDFLAGS += \
    -mcpu=cortex-m7 \
    -T"$(STMSRC_DIR)/STM32F769NIHX_FLASH.ld" \
    -Wl,-Map="stm32f7q1.map" \
    -Wl,--gc-sections -static \
    --specs=nano.specs \
    -mfpu=fpv5-d16 -mfloat-abi=hard \
    -mthumb -Wl,--start-group \
    -lc -lm -lstdc++ -lsupc++ \
    -Wl,--end-group
