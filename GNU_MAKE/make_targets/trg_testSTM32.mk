DST_PLATFORM := STM32

# INCLUDES += $(SRC_DIR)
# INCLUDES += /opt/homebrew/opt/libx11/include
# INCLUDES += /opt/homebrew/include

# include features/fh_qEngine.mk

$(eval PLATFORM_DIR = $(SRC_DIR)/platform) $(eval INCLUDES += $(PLATFORM_DIR)/API)
        $(eval MCU_DIR = $(PLATFORM_DIR)/MCU) $(eval INCLUDES += $(MCU_DIR))
                SRC_LIST += $(MCU_DIR)/main.c
                $(eval INCLUDES += $(MCU_DIR)/STM32F7)
                SRC_LIST += $(MCU_DIR)/STM32F7/perepherial.c
# $(eval ROOT_DIR := $(STMSRC_DIR)) $(eval INCLUDES += $(ROOT_DIR))
$(eval INCLUDES += $(STMSRC_DIR)/Drivers/CMSIS/Device/ST/STM32F7xx/Include)
$(eval INCLUDES += $(STMSRC_DIR)/Drivers/CMSIS/Include)

    $(eval CORE_DIR := $(STMSRC_DIR)/Core)
        $(eval INCLUDES += $(CORE_DIR)/Inc)
        SRC_LIST += $(CORE_DIR)/Src/freertos.c
#         SRC_LIST += $(CORE_DIR)/Src/main.c
        SRC_LIST += $(CORE_DIR)/Src/stm32f7xx_hal_msp.c
        SRC_LIST += $(CORE_DIR)/Src/stm32f7xx_hal_timebase_tim.c
        SRC_LIST += $(CORE_DIR)/Src/stm32f7xx_it.c
        SRC_LIST += $(CORE_DIR)/Src/syscalls.c
        SRC_LIST += $(CORE_DIR)/Src/sysmem.c
        SRC_LIST += $(CORE_DIR)/Src/system_stm32f7xx.c
        SRC_LIST += $(CORE_DIR)/Startup/startup_stm32f769nihx.s

    $(eval HAL_DIR := $(STMSRC_DIR)/Drivers/STM32F7xx_HAL_Driver)
        $(eval INCLUDES += $(HAL_DIR)/Inc)
        $(eval INCLUDES += $(HAL_DIR)/Inc/Legacy)
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_adc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_adc_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_cec.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_cortex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_crc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_crc_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_dma.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_dma2d.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_dma_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_dsi.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_eth.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_exti.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_flash.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_flash_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_gpio.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_i2c.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_i2c_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_iwdg.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_ltdc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_ltdc_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_mmc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_nand.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_nor.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_pcd.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_pcd_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_pwr.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_pwr_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_qspi.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_rcc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_rcc_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_rtc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_rtc_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_sai.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_sai_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_sd.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_sdram.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_spdifrx.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_spi.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_spi_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_sram.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_tim.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_tim_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_uart.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_uart_ex.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_hal_wwdg.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_ll_fmc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_ll_sdmmc.c
        SRC_LIST += $(HAL_DIR)/Src/stm32f7xx_ll_usb.c

    $(eval RTOS_DIR := $(STMSRC_DIR)/Middlewares/Third_Party/FreeRTOS/Source)
        $(eval INCLUDES += $(RTOS_DIR)/include)
        $(eval INCLUDES += $(RTOS_DIR)/CMSIS_RTOS)
        $(eval INCLUDES += $(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1)
        SRC_LIST += $(RTOS_DIR)/portable/MemMang/heap_4.c
        SRC_LIST += $(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1/port.c
        SRC_LIST += $(RTOS_DIR)/CMSIS_RTOS/cmsis_os.c
        SRC_LIST += $(RTOS_DIR)/croutine.c
        SRC_LIST += $(RTOS_DIR)/event_groups.c
        SRC_LIST += $(RTOS_DIR)/list.c
        SRC_LIST += $(RTOS_DIR)/queue.c
        SRC_LIST += $(RTOS_DIR)/stream_buffer.c
        SRC_LIST += $(RTOS_DIR)/tasks.c
        SRC_LIST += $(RTOS_DIR)/timers.c

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

    LDFLAGS += \
        -mcpu=cortex-m7 \
        -T"$(STMSRC_DIR)/STM32F769NIHX_FLASH.ld" \
        -Wl,-Map="stm32f7q1.map" \
        -Wl,--gc-sections \
        -static \
        --specs=nano.specs \
        -mfpu=fpv5-d16 \
        -mfloat-abi=hard \
        -mthumb \
        -Wl,--start-group \
        -lc -lm -lstdc++ -lsupc++ \
        -Wl,--end-group