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
    clear ; \
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

# BUILD STM GCC om ARM host
```
    mkdir -p ~/stm32_toolchain && cd ~/stm32_toolchain

    # Скачиваем официальный нативный бинарник от ARM (версия 13.3.Rel1 под Linux ARM64)
    curl -Lo arm-gnu-toolchain.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-aarch64-arm-none-eabi.tar.xz"

    # Распаковываем архив
    tar -xvf arm-gnu-toolchain.tar.xz --strip-components=1

    # Проверяем работу компилятора
    ./bin/arm-none-eabi-gcc --version
```



# make SD image
    fdisk -l /dev/sda
```
Disk /dev/sda: 29.13 GiB, 31281119232 bytes, 61095936 sectors
Disk model: MassStorageClass
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x54531ad4

Device     Boot Start     End Sectors Size Id Type
/dev/sda1        2048 2099199 2097152   1G  c W95 FAT32 (LBA)
```

# make Image
    # dd if=/dev/sda of=/root/sda_head_and_sda1.img bs=512 count=$(Sectors)+1 status=progress && sync
    # dd if=/dev/sda of=/root/sda_head_and_sda1.img bs=512 count=2099200 status=progress && sync
    sudo dd if=/dev/sda of=./QUAKE_SD.img bs=512 count=131073 status=progress && sync

# write image to SD card
    # sudo dd if=/root/sda_head_and_sda1.img of=/dev/sda bs=4M status=progress && sync
    sudo dd if=./QUAKE_SD.img of=/dev/sda bs=4M status=progress && sync

# make partition table
    parted -s /dev/sda mklabel msdos mkpart primary fat32 2048s 133119s
    dd if=/dev/zero of=/dev/sda1 bs=4M status=progress && sync
    mkfs.vfat -F 32 -s 1 -S 512 -R 4110 -n "QUAKE_SD" /dev/sda1 && sync


# fsck.vfat -nvv /dev/sda1
fsck.fat 4.2 (2021-01-31)
Checking we can access the last sector of the filesystem
Boot sector contents:
System ID "MSDOS5.0"
Media byte 0xf8 (hard disk)
       512 bytes per logical sector
      4096 bytes per cluster
      4110 reserved sectors
First FAT starts at byte 2104320 (sector 4110)
         2 FATs, 32 bit entries
   1044992 bytes per FAT (= 2041 sectors)
Root directory start at cluster 2 (arbitrary size)
Data area starts at byte 4194304 (sector 8192)
    261120 data clusters (1069547520 bytes)
63 sectors/track, 255 heads
      2048 hidden sectors
   2097152 sectors total
Checking for unused clusters.
Checking free cluster summary.
/dev/sda1: 5 files, 4567/261120 clusters
