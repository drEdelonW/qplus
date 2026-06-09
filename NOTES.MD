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

```
    make TARGET=quakeX11 fresh && \
    make TARGET=quakeGLX11 fresh && \
    make TARGET=quakeSTM32 fresh

    make TARGET=quakeX11 && \
    make TARGET=quakeGLX11

    make && make stupload && make stdebug
```

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


    #apt install texinfo bison flex libgmp-dev libmpfr-dev libmpc-dev texlive
    apt-get update && apt-get install -y libisl-dev
    apt-get install -y libgmp-dev libmpfr-dev libmpc-dev
```

    mkdir -p ~/stm32_toolchain && cd ~/stm32_toolchain
    git clone --depth 1 --branch 13.3.rel1 https://github.com/STMicroelectronics/gnu-tools-for-stm32.git
    # cd gnu-tools-for-stm32

    # cd src/gcc && \
    # ./contrib/download_prerequisites && \
    # cd -

    rm -rf build-native
    rm -rf install-native
    ./build-toolchain.sh
    ./build-toolchain.sh | tee buil.log
```

```
    mkdir -p ~/stm32_toolchain && cd ~/stm32_toolchain

    # Скачиваем официальный нативный бинарник от ARM (версия 13.3.Rel1 под Linux ARM64)
    curl -Lo arm-gnu-toolchain.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-aarch64-arm-none-eabi.tar.xz"

    # Распаковываем архив
    tar -xvf arm-gnu-toolchain.tar.xz --strip-components=1

    # Проверяем работу компилятора
    ./bin/arm-none-eabi-gcc --version
```

/root/stm32_toolchain/gnu-tools-for-stm32/src/binutils/missing: 81: makeinfo: not found
WARNING: 'makeinfo' is missing on your system.
         You should only need it if you modified a '.texi' file, or
         any other file indirectly affecting the aspect of the manual.
         You might want to install the Texinfo package:
         <http://www.gnu.org/software/texinfo/>
         The spurious makeinfo call might also be the consequence of
         using a buggy 'make' (AIX, DU, IRIX), in which case you might
         want to install GNU make:
         <http://www.gnu.org/software/make/>
make[3]: *** [Makefile:1792: doc/bfd.info] Error 127
make[3]: Leaving directory '/root/stm32_toolchain/gnu-tools-for-stm32/build-native/binutils/bfd'
make[2]: *** [Makefile:1952: info-recursive] Error 1
make[2]: Leaving directory '/root/stm32_toolchain/gnu-tools-for-stm32/build-native/binutils/bfd'
make[1]: *** [Makefile:3119: all-bfd] Error 2
make[1]: Leaving directory '/root/stm32_toolchain/gnu-tools-for-stm32/build-native/binutils'
make: *** [Makefile:1031: all] Error 2
root@GitLab-Pi5:~/stm32_toolchain/gnu-tools-for-stm32#

