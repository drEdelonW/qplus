CC  := arm-none-eabi-gcc
ASM := arm-none-eabi-gcc
CXX := arm-none-eabi-g++
LD := $(CXX)
CFLAGS := \
	-mcpu=cortex-m7 \
	-std=gnu11 \
	-g3 \
	-DDEBUG \
	-DUSE_HAL_DRIVER \
	-DSTM32F769xx \
	-c
CFLAGS1 := \
	-O0 \
	-ffunction-sections \
	-fdata-sections \
	-Wall \
	-fstack-usage \
	-fcyclomatic-complexity \
	-MMD \
	-MP
CFLAGS2 := \
	--specs=nano.specs \
	-mfpu=fpv5-d16 \
	-mfloat-abi=hard \
	-mthumb
ROOT_DIR := ../
HAL_DIR  = Drivers/STM32F7xx_HAL_Driver
RTOS_DIR = Middlewares/Third_Party/FreeRTOS/Source
CINCLUDES := \
	-I$(ROOT_DIR)Core/Inc \
	-I$(ROOT_DIR)$(HAL_DIR)/Inc \
	-I$(ROOT_DIR)$(HAL_DIR)/Inc/Legacy \
	-I$(ROOT_DIR)$(RTOS_DIR)/include \
	-I$(ROOT_DIR)$(RTOS_DIR)/CMSIS_RTOS \
	-I$(ROOT_DIR)$(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1 \
	-I$(ROOT_DIR)Drivers/CMSIS/Device/ST/STM32F7xx/Include \
	-I$(ROOT_DIR)Drivers/CMSIS/Include

# make -j12 all

$(CC) "$(ROOT_DIR)$(RTOS_DIR)/portable/MemMang/heap_4.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/portable/MemMang/heap_4.d" -MT"$(RTOS_DIR)/portable/MemMang/heap_4.o" $(CFLAGS2) -o "$(RTOS_DIR)/portable/MemMang/heap_4.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1/port.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1/port.d" -MT"$(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1/port.o" $(CFLAGS2) -o "$(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1/port.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/CMSIS_RTOS/cmsis_os.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/CMSIS_RTOS/cmsis_os.d" -MT"$(RTOS_DIR)/CMSIS_RTOS/cmsis_os.o" $(CFLAGS2) -o "$(RTOS_DIR)/CMSIS_RTOS/cmsis_os.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/croutine.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/croutine.d" -MT"$(RTOS_DIR)/croutine.o" $(CFLAGS2) -o "$(RTOS_DIR)/croutine.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/event_groups.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/event_groups.d" -MT"$(RTOS_DIR)/event_groups.o" $(CFLAGS2) -o "$(RTOS_DIR)/event_groups.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/list.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/list.d" -MT"$(RTOS_DIR)/list.o" $(CFLAGS2) -o "$(RTOS_DIR)/list.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/queue.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/queue.d" -MT"$(RTOS_DIR)/queue.o" $(CFLAGS2) -o "$(RTOS_DIR)/queue.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/stream_buffer.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/stream_buffer.d" -MT"$(RTOS_DIR)/stream_buffer.o" $(CFLAGS2) -o "$(RTOS_DIR)/stream_buffer.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/tasks.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/tasks.d" -MT"$(RTOS_DIR)/tasks.o" $(CFLAGS2) -o "$(RTOS_DIR)/tasks.o"
$(CC) "$(ROOT_DIR)$(RTOS_DIR)/timers.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(RTOS_DIR)/timers.d" -MT"$(RTOS_DIR)/timers.o" $(CFLAGS2) -o "$(RTOS_DIR)/timers.o"

$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_adc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_adc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_adc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_adc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_adc_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_adc_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_adc_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_adc_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_cec.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_cec.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_cec.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_cec.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_cortex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_cortex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_cortex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_cortex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_crc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_crc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_crc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_crc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_crc_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_crc_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_crc_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_crc_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_dma.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_dma.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_dma.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_dma.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_dma2d.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_dma2d.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_dma2d.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_dma2d.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_dma_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_dma_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_dma_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_dma_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_dsi.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_dsi.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_dsi.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_dsi.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_eth.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_eth.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_eth.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_eth.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_exti.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_exti.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_exti.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_exti.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_flash.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_flash.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_flash.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_flash.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_flash_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_flash_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_flash_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_flash_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_gpio.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_gpio.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_gpio.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_gpio.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_i2c.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_i2c.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_i2c.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_i2c.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_i2c_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_i2c_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_i2c_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_i2c_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_iwdg.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_iwdg.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_iwdg.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_iwdg.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_ltdc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_ltdc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_ltdc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_ltdc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_ltdc_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_ltdc_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_ltdc_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_ltdc_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_mmc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_mmc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_mmc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_mmc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_nand.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_nand.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_nand.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_nand.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_nor.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_nor.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_nor.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_nor.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_pcd.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_pcd.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_pcd.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_pcd.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_pcd_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_pcd_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_pcd_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_pcd_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_pwr.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_pwr.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_pwr.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_pwr.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_pwr_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_pwr_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_pwr_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_pwr_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_qspi.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_qspi.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_qspi.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_qspi.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_rcc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_rcc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_rcc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_rcc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_rcc_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_rcc_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_rcc_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_rcc_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_rtc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_rtc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_rtc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_rtc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_rtc_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_rtc_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_rtc_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_rtc_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_sai.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_sai.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_sai.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_sai.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_sai_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_sai_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_sai_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_sai_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_sd.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_sd.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_sd.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_sd.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_sdram.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_sdram.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_sdram.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_sdram.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_spdifrx.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_spdifrx.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_spdifrx.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_spdifrx.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_spi.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_spi.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_spi.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_spi.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_spi_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_spi_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_spi_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_spi_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_sram.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_sram.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_sram.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_sram.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_tim.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_tim.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_tim.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_tim.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_tim_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_tim_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_tim_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_tim_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_uart.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_uart.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_uart.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_uart.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_uart_ex.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_uart_ex.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_uart_ex.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_uart_ex.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_hal_wwdg.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_hal_wwdg.d" -MT"$(HAL_DIR)/Src/stm32f7xx_hal_wwdg.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_hal_wwdg.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_ll_fmc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_ll_fmc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_ll_fmc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_ll_fmc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_ll_sdmmc.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_ll_sdmmc.d" -MT"$(HAL_DIR)/Src/stm32f7xx_ll_sdmmc.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_ll_sdmmc.o"
$(CC) "$(ROOT_DIR)$(HAL_DIR)/Src/stm32f7xx_ll_usb.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"$(HAL_DIR)/Src/stm32f7xx_ll_usb.d" -MT"$(HAL_DIR)/Src/stm32f7xx_ll_usb.o" $(CFLAGS2) -o "$(HAL_DIR)/Src/stm32f7xx_ll_usb.o"

$(ASM) -mcpu=cortex-m7 -g3 -DDEBUG -c $(CINCLUDES) -x assembler-with-cpp -MMD -MP -MF"Core/Startup/startup_stm32f769nihx.d" -MT"Core/Startup/startup_stm32f769nihx.o" $(CFLAGS2) -o "Core/Startup/startup_stm32f769nihx.o" "$(ROOT_DIR)Core/Startup/startup_stm32f769nihx.s"

$(CC) "$(ROOT_DIR)Core/Src/freertos.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/freertos.d" -MT"Core/Src/freertos.o" $(CFLAGS2) -o "Core/Src/freertos.o"
$(CC) "$(ROOT_DIR)Core/Src/main.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/main.d" -MT"Core/Src/main.o" $(CFLAGS2) -o "Core/Src/main.o"
$(CC) "$(ROOT_DIR)Core/Src/stm32f7xx_hal_msp.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/stm32f7xx_hal_msp.d" -MT"Core/Src/stm32f7xx_hal_msp.o" $(CFLAGS2) -o "Core/Src/stm32f7xx_hal_msp.o"
$(CC) "$(ROOT_DIR)Core/Src/stm32f7xx_hal_timebase_tim.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/stm32f7xx_hal_timebase_tim.d" -MT"Core/Src/stm32f7xx_hal_timebase_tim.o" $(CFLAGS2) -o "Core/Src/stm32f7xx_hal_timebase_tim.o"
$(CC) "$(ROOT_DIR)Core/Src/stm32f7xx_it.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/stm32f7xx_it.d" -MT"Core/Src/stm32f7xx_it.o" $(CFLAGS2) -o "Core/Src/stm32f7xx_it.o"
$(CC) "$(ROOT_DIR)Core/Src/syscalls.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/syscalls.d" -MT"Core/Src/syscalls.o" $(CFLAGS2) -o "Core/Src/syscalls.o"
$(CC) "$(ROOT_DIR)Core/Src/sysmem.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/sysmem.d" -MT"Core/Src/sysmem.o" $(CFLAGS2) -o "Core/Src/sysmem.o"
$(CC) "$(ROOT_DIR)Core/Src/system_stm32f7xx.c" $(CFLAGS) $(CINCLUDES) $(CFLAGS1) -MF"Core/Src/system_stm32f7xx.d" -MT"Core/Src/system_stm32f7xx.o" $(CFLAGS2) -o "Core/Src/system_stm32f7xx.o"

$(LD)  -o "stm32f7q1.elf" @"objects.list"   -mcpu=cortex-m7 -T"D:\SelfLab\RnD\quake\MCU_src\STM32F769NIHX_FLASH.ld" --specs=nosys.specs -Wl,-Map="stm32f7q1.map" -Wl,--gc-sections -static $(CFLAGS2) -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
# Finished building target: stm32f7q1.elf

arm-none-eabi-size  stm32f7q1.elf
arm-none-eabi-objdump -h -S stm32f7q1.elf  > "stm32f7q1.list"
#    text	   data	    bss	    dec	    hex	filename
#   60392	 192336	  39728	 292456	  47668	stm32f7q1.elf
# Finished building: default.size.stdout

# Finished building: stm32f7q1.list
