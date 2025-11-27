DST_PLATFORM := STM32

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR)/API)
    $(eval MCU_DIR = $(PLATFORM_DIR)/MCU) $(eval INCLUDES += $(MCU_DIR))
        $(eval STM32_DIR = $(MCU_DIR)/STM32F7) $(eval INCLUDES += $(STM32_DIR))
            $(eval FS_DIR = $(STM32_DIR)/FileSystem) $(eval INCLUDES += $(FS_DIR))
                SRC_LIST += $(FS_DIR)/per_SD_tools.c
                SRC_LIST += $(FS_DIR)/SD_TF_tools.c
                SRC_LIST += $(FS_DIR)/MBR_tool.c
                SRC_LIST += $(FS_DIR)/fs_FAT32.c

#             SRC_LIST += $(STM32_DIR)/main.c
            SRC_LIST += $(STM32_DIR)/perepherial.c
            SRC_LIST += $(STM32_DIR)/retarget_io.c
            SRC_LIST += $(STM32_DIR)/per_FMC_SDRAM.c

        SRC_LIST += $(MCU_DIR)/sys_STM32F7.c

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR)) $(eval INCLUDES += $(PLATFORM_DIR)/API)
    $(eval MCU_DIR = $(PLATFORM_DIR)/MCU) $(eval INCLUDES += $(MCU_DIR))
#         SRC_LIST += $(MCU_DIR)/cd_null.c
#         SRC_LIST += $(MCU_DIR)/snd_null.c
#         SRC_LIST += $(MCU_DIR)/sys_null.c
#         SRC_LIST += $(MCU_DIR)/vid_null.c
        SRC_LIST += $(MCU_DIR)/net_mcu.c
#         SRC_LIST += $(MCU_DIR)/in_mcu.c

# $(eval ROOT_DIR := $(STMSRC_DIR)) $(eval INCLUDES += $(ROOT_DIR))
$(eval INCLUDES += $(STMSRC_DIR)/Drivers/CMSIS/Device/ST/STM32F7xx/Include)
$(eval INCLUDES += $(STMSRC_DIR)/Drivers/CMSIS/Include)

    $(eval CORE_DIR := $(STMSRC_DIR)/Core)
        $(eval INCLUDES += $(CORE_DIR)/Inc)
        # SRC_LIST += $(CORE_DIR)/Src/freertos.c
        # SRC_LIST += $(CORE_DIR)/Src/main.c
        SRC_LIST += $(CORE_DIR)/Src/stm32f7xx_hal_msp.c
        SRC_LIST += $(CORE_DIR)/Src/stm32f7xx_hal_timebase_tim.c
        SRC_LIST += $(CORE_DIR)/Src/stm32f7xx_it.c
        SRC_LIST += $(CORE_DIR)/Src/syscalls.c
        # SRC_LIST += $(CORE_DIR)/Src/sysmem.c
        SRC_LIST += $(CORE_DIR)/Src/system_stm32f7xx.c
        SRC_LIST += $(CORE_DIR)/Startup/startup_stm32f769nihx.s

include features/fh_HAL.mk
# include features/fh_RTOS.mk

    DEFINES += STM32
