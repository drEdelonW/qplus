ifeq ($(UNAME_S),Linux)
    ST_RESET_CMD ?= st-flash reset

    ST_FLASH_CMD ?= st-flash --format ihex write $(TARGET_FILE).hex
    ST_PROGRAM   ?= $(ST_FLASH_CMD) && $(ST_RESET_CMD)

    ST_FLASH_BL  ?= st-flash write $(TARGET_FILE).bin 0x08004000
    ST_PROGRAMBL ?= $(ST_FLASH_BL) && $(ST_RESET_CMD)

else ifeq ($(UNAME_S),MINGW64_NT-10.0-19045)
    ST_LINK_PATH    ?= "/c/Program Files (x86)/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/"
    ST_LINK_EXE     ?= ST-LINK_CLI.exe
    ST_PROGRAM      ?= $(ST_LINK_PATH)$(ST_LINK_EXE) -c SWD  -P $(TARGET_FILE).hex -Rst

    ST_FLASH_BL     ?= $(ST_LINK_PATH)$(ST_LINK_EXE) -c SWD UR -P $(TARGET_FILE).bin 0x08004000 -Rst
    ST_PROGRAMBL    ?= $(ST_FLASH_BL)
else ifeq ($(UNAME_S),Darwin)
    ST_RESET_CMD ?= st-flash reset

    ST_FLASH_CMD ?= st-flash --format ihex write $(TARGET_FILE).hex
    ST_PROGRAM   ?= $(ST_FLASH_CMD) && $(ST_RESET_CMD)

    ST_FLASH_BL  ?= st-flash write $(TARGET_FILE).bin 0x08004000
    ST_PROGRAMBL ?= $(ST_FLASH_BL) && $(ST_RESET_CMD)
else
endif

streset:
	@echo "Reset by ST-Link..."
	@$(ST_RESET_CMD)

stupload: $(TARGET_FILE).hex
	@echo "Uploading by ST-Link..."
	$(ST_PROGRAM)

stuploadbl: $(TARGET_FILE).bin
	@echo "Uploading by ST-Link..."
	@$(ST_PROGRAMBL)


