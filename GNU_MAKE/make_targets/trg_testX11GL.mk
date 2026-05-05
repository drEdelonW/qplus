DST_PLATFORM := PC

INCLUDES += $(SRC_DIR)
$(eval X11GL_DIR = $(SRC_DIR)/testX11GL) $(eval INCLUDES += $(X11GL_DIR))
        SRC_LIST += $(X11GL_DIR)/main.c

#----------LINUX------------
ifeq ($(UNAME_S),Linux)

    RUN_PREFIX := DISPLAY=:1

    LDLIBS += -lGL -lX11 -lm

#----------MacOS------------
else ifeq ($(UNAME_S),Darwin)

#     HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null)
    CFLAGS  += -arch arm64
    CFLAGS  += -I/opt/X11/include
#     CXXFLAGS += -I$(HOMEBREW_PREFIX)/include
#     CXXFLAGS +=  -I$(HOMEBREW_PREFIX)/include/SDL2
#     LDFLAGS += -L$(HOMEBREW_PREFIX)/lib -Wl,-rpath,$(HOMEBREW_PREFIX)/lib
    LDFLAGS += -L/opt/X11/lib -Wl,-rpath,/opt/X11/lib
    LDLIBS  += -lX11 -lGL

endif
#     CFLAGS  += $(SDL2_CFLAGS)
#     LDLIBS  += $(SDL2_LIBS)
#     LDLIBS  += -lm
