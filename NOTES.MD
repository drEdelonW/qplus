# Install
    sudo apt install xserver-xephyr

# run EXTERNAL WINDOW
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