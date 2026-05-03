# Install
    sudo apt install xserver-xephyr

    sudo apt install \
        make \
        gcc-aarch64-linux-gnu \
        gcc-arm-linux-gnueabihf \
        gcc build-essential \
        libx11-dev libxext-dev


# ARM32 Install
    sudo apt install gcc-arm-linux-gnueabihf
    sudo dpkg --add-architecture armhf
    sudo apt update
    sudo apt install libx11-dev:armhf libxext-dev:armhf

# MacOS specific
    clang -dM -E - < /dev/null | grep __APPLE__

# run EXTERNAL WINDOW
    open -a XQuartz  # run X11 server on MacOS

    Xephyr :1 -screen 320x200x16 -ac -retro

# run in extern window
    DISPLAY=:1 ./quake.x11 -nosound -noshm -nodga -verbose
    DISPLAY=:1 ./quake.x11 -nosound
    DISPLAY=:1 ./quake.x11


code --install-extension ms-vscode.cpptools


sudo dpkg --add-architecture i386
sudo apt update
sudo apt install \
    gcc-multilib g++-multilib libc6-dev-i386 \
    libx11-dev:i386 libxext-dev:i386 libxxf86dga-dev:i386


    ln -s ../OUTPUT/quake.x11 quake


    Нужно заменять на intptr_t, uintptr_t, size_t.



sudo apt update
sudo apt install -y libsdl2-2.0-0 libsdl2-dev \
                    mesa-utils libgl1-mesa-dri libegl1-mesa libgles2-mesa


export SDL_VIDEO_X11_FORCE_EGL=1
export SDL_RENDER_DRIVER=opengles2     # рендерер SDL через GLES2
OUTPUT/testSDL2/testSDL2.elf

JackHummer - map editor in Steam

# OpenGL files
    vid_svgalib.c / vid_x.c -> gl_vidlinux.c / gl_vidlinuxglx.c
    screen.c        -> gl_screen.c
    [V] draw.c      -> gl_draw.c

    model.c         -> gl_model.c / gl_mesh.c

    r_main.c        -> gl_rmain.c
    r_misc.c        -> gl_rmisc.c
    r_surf.c        -> gl_rsurf.c
    r_light.c       -> gl_rlight.c
    r_efrag.c       -> gl_refrag.c
    r_sky.c + d_sky.c       -> gl_warp.c частично
