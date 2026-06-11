DST_PLATFORM := STM32
include features/fh_HAL.mk

$(eval INCLUDES += ../src/engine/Shared/utils)
$(eval INCLUDES += ../src/platform/MCU/STM32F7)

$(eval FSRC_DIR = $(SRC_DIR)/STM32_LCD) $(eval INCLUDES += $(FSRC_DIR))
        SRC_LIST += $(FSRC_DIR)/main.c
        SRC_LIST += $(FSRC_DIR)/stm32f7xx_it.c
#         SRC_LIST += $(FSRC_DIR)/system_stm32f7xx.c
#         SRC_LIST += $(FSRC_DIR)/SW4STM32/syscalls.c
#         SRC_LIST += $(FSRC_DIR)/SW4STM32/startup_stm32f769xx.s


   $(eval CORE_DIR := $(STMSRC_DIR)/Core)
        $(eval INCLUDES += $(CORE_DIR)/Inc)
        SRC_LIST += $(CORE_DIR)/Src/syscalls.c
#         SRC_LIST += $(CORE_DIR)/Src/stm32f7xx_it.c
        SRC_LIST += $(CORE_DIR)/Src/system_stm32f7xx.c
        SRC_LIST += $(CORE_DIR)/Startup/startup_stm32f769nihx.s

# /Users/edelon/SelfLab/rnd/quake/MCU_src/BSP/STM32F769I-Discovery/stm32f769i_discovery.h
        $(eval INCLUDES += $(STMSRC_DIR)/Drivers/BSP/STM32F769I-Discovery)
        SRC_LIST += $(STMSRC_DIR)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery.c
        SRC_LIST += $(STMSRC_DIR)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_lcd.c
        SRC_LIST += $(STMSRC_DIR)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_sdram.c
#         SRC_LIST += $(STMSRC_DIR)/Drivers/BSP/Components/otm8009a/otm8009a.c
        SRC_LIST += $(STMSRC_DIR)/Drivers/BSP/Components/nt35510/nt35510.c

# IDSRC_DIR := /Users/edelon/SelfLab/STM32_src_docs/STM32CubeF7
#     $(eval INCLUDES += $(IDSRC_DIR)/Drivers/BSP/STM32F769I-Discovery)
#         SRC_LIST += $(IDSRC_DIR)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery.c
#         SRC_LIST += $(IDSRC_DIR)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_lcd.c
#         SRC_LIST += $(IDSRC_DIR)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_sdram.c
#         SRC_LIST += $(IDSRC_DIR)/Drivers/BSP/Components/nt35510/nt35510.c
#         SRC_LIST += $(IDSRC_DIR)/Drivers/BSP/Components/otm8009a/otm8009a.c
#         SRC_LIST += $(IDSRC_DIR)/Drivers/BSP/STM32F769I-Discovery/stm32f769i_discovery_sd.c



# Drivers/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.c
MCU_FLAGS = -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
CFLAGS += $(MCU_FLAGS)
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

CXXFLAGS += $(MCU_FLAGS)

LDFLAGS += $(MCU_FLAGS)
LDFLAGS += \
    -mcpu=cortex-m7 \
    -T"$(STMSRC_DIR)/STM32F769NIHX_FLASH.ld" \
    -Wl,-Map="$(OUT_DIR)/$(TARGET).map" \
    -Wl,--gc-sections -static \
    --specs=nano.specs \
    -mfpu=fpv5-d16 -mfloat-abi=hard \
    -mthumb -Wl,--start-group \
    -lc -lm -lstdc++ -lsupc++ \
    -Wl,--end-group
