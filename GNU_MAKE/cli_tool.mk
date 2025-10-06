# ---------- Colors ----------
ifeq ($(NO_COLOR),1)
  $(info COLORS DISABLED)
  COLOR := 0
else ifneq (,$(findstring MINGW,$(UNAME_S)))
  COLOR := 0
else ifeq ($(shell test -t 1 && echo 1),1)
  COLOR := 1
else
  $(info COLORS NOT SUPPORTED)
  COLOR := 0
endif
# ifeq ($(OS),Windows_NT)
#   COLOR := 0
# endif

ifeq ($(COLOR),1)
  RED := \033[31m
  GRN := \033[32m
  CYN := \033[36m
  BLD := \033[1m
  RST := \033[0m
else
  RED :=
  GRN :=
  CYN :=
  BLD :=
  RST :=
endif