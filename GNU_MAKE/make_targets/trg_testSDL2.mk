DST_PLATFORM := PC

INCLUDES += $(SRC_DIR)
$(eval SDL_DIR = $(SRC_DIR)/srcSDL2) $(eval INCLUDES += $(SDL_DIR))
        SRC_LIST += $(SDL_DIR)/main.c

#     CFLAGS  += -DNO_X11_SHM
#     # X11 target on *nix
#     CPPFLAGS   += -DX11 -DVID_X11
#     CPPFLAGS += -I/opt/X11/include

#     LDFLAGS  += -L/opt/X11/lib
#     LDLIBS     += -lX11 -lXext


    SDL2_CFLAGS := $(shell pkg-config --cflags sdl2)
    SDL2_LIBS   := $(shell pkg-config --libs sdl2)
    CFLAGS  += $(SDL2_CFLAGS)
    LDLIBS  += $(SDL2_LIBS)
#----------LINUX------------

# RUN_PREFIX := DISPLAY=:1

#----------MacOS------------
    HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null)
    CFLAGS  += -arch arm64
    CPPFLAGS += -I$(HOMEBREW_PREFIX)/include -I$(HOMEBREW_PREFIX)/include/SDL2
    LDFLAGS += -L$(HOMEBREW_PREFIX)/lib -Wl,-rpath,$(HOMEBREW_PREFIX)/lib
    LDLIBS  += -lm

