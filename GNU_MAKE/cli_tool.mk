# ---------- Colors ----------
ifeq ($(NO_COLOR),1)
  COLOR := 0
else ifeq ($(shell test -t 1 && echo 1),1)
  COLOR := 1
else
  COLOR := 0
endif

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