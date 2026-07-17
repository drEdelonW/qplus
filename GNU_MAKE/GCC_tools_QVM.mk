.DEFAULT_GOAL := all

QCC_BIN := OUTPUT/qcc/qcc.elf
QCC_ABS := $(abspath $(QCC_BIN))

$(QCC_BIN):
	@$(MAKE) TARGET=qcc build

QVM_OUT := $(OUT_DIR)/progs.dat

$(QVM_OUT): $(QCC_BIN) $(QC_FILES) | $(OUT_DIR)
	@{ echo $(abspath $@); echo; printf '%s\n' $(notdir $(QC_FILES)); } > $(QC_DIR)/progs.src
	@cd $(QC_DIR) && $(QCC_ABS)
	@mv -f $(QC_DIR)/progdefs.h $(OUT_DIR)/progdefs.h
	@$(info DONE. -> $@)

.PHONY: qcbuild qcclean
qcbuild: $(QVM_OUT)
	@touch $(OUT_DIR)/$(TARGET)

qcclean:
	@rm -f $(QVM_OUT) $(QC_DIR)/progs.src $(OUT_DIR)/progdefs.h $(OUT_DIR)/$(TARGET) $(OUT_DIR)/pak0.pak

all:      qcbuild
build:    qcbuild
targets:  qcbuild
clean:    qcclean

ENGINE     ?= quakeX11
ENGINE_ELF := OUTPUT/$(ENGINE)/$(ENGINE).elf
ENGINE_BIN := run_env/$(ENGINE)
PACK       ?= 0

$(ENGINE_ELF):
	@$(MAKE) TARGET=$(ENGINE) build

$(ENGINE_BIN): $(ENGINE_ELF)
	@cd run_env && ln -sf ../$(ENGINE_ELF) $(ENGINE)

RUN_PREFIX := DISPLAY=:1
.PHONY: qcrun
qcrun: qcbuild $(ENGINE_BIN)
ifeq ($(PACK),1)
	@cd $(QC_DIR) &&\
		$(QCC_ABS) -pak $(abspath run_env/id1)/ $(abspath $(OUT_DIR))/pak0.pak
endif
	@mkdir -p run_env/$(TARGET)
	@cd run_env/$(TARGET) &&\
		ln -sf ../../$(OUT_DIR)/progs.dat progs.dat
	@[ -f $(OUT_DIR)/pak0.pak ] &&\
		cd run_env/$(TARGET) &&\
		ln -sf ../../$(OUT_DIR)/pak0.pak pak0.pak || true
	@cd run_env &&\
		$(RUN_PREFIX) ./$(ENGINE) -game $(TARGET)
