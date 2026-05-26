$(eval RTOS_DIR := $(STMSRC_DIR)/Middlewares/Third_Party/FreeRTOS/Source)
    $(eval INCLUDES += $(RTOS_DIR)/include)
        SRC_LIST += $(RTOS_DIR)/croutine.c
        SRC_LIST += $(RTOS_DIR)/event_groups.c
        SRC_LIST += $(RTOS_DIR)/list.c
        SRC_LIST += $(RTOS_DIR)/queue.c
        SRC_LIST += $(RTOS_DIR)/stream_buffer.c
        SRC_LIST += $(RTOS_DIR)/tasks.c
        SRC_LIST += $(RTOS_DIR)/timers.c
    $(eval INCLUDES += $(RTOS_DIR)/CMSIS_RTOS)
        SRC_LIST += $(RTOS_DIR)/CMSIS_RTOS/cmsis_os.c
    $(eval INCLUDES += $(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1)
        SRC_LIST += $(RTOS_DIR)/portable/MemMang/heap_4.c
        SRC_LIST += $(RTOS_DIR)/portable/GCC/ARM_CM7/r0p1/port.c

    SRC_LIST += $(CORE_DIR)/Src/freertos.c
