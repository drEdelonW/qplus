#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "sys.h"
#include "client.h"


cString Sys_ConsoleInput() {
    static char text[256];

    if (cls.state == ca_dedicated) {
        fd_set	fdset;
        FD_ZERO(&fdset);
        FD_SET(0, &fdset); // stdin

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        if ((select(1, &fdset, NULL, NULL, &timeout) == -1) ||
            (!FD_ISSET(0, &fdset))
            ) {
            return NULL;
        }

        int len = read(0, text, sizeof(text));
        if (len < 1)
            return NULL;
        text[len - 1] = 0;    // rip off the /n and terminate

        return text;
    }
    return NULL;
}

/*
============
    Sys_FileTime

    returns -1 if not present
============
*/
int	Sys_FileTime(cStringRO path) {
    struct stat buf;

    if (stat(path, &buf) == -1)
        return -1;

    return buf.st_mtime;
}


void Sys_mkdir(cStringRO path) {
    mkdir(path, 0777);
}

int Sys_FileOpenRead(cStringRO path, int* handle) {
    struct stat	fileinfo;

    int h = open(path, O_RDONLY, 0666);
    *handle = h;
    if (h == -1)
        return -1;

    if (fstat(h, &fileinfo) == -1)
        Sys_Error("Error fstating %s", path);

    return fileinfo.st_size;
}

int Sys_FileOpenWrite(cStringRO path) {
    umask(0);

    int handle = open(path, (O_RDWR | O_CREAT | O_TRUNC), 0666);

    if (handle == -1)
        Sys_Error("Error opening %s: %s", path, strerror(errno));

    return handle;
}

int Sys_FileWrite(int handle, TypeLess_ptr src, int count) {
    return write(handle, src, count);
}

void Sys_FileClose(int handle) {
    close(handle);
}

void Sys_FileSeek(int handle, int position) {
    lseek(handle, position, SEEK_SET);
}

int Sys_FileRead(int handle, TypeLess_ptr dest, int count) {
    return read(handle, dest, count);
}

void Sys_DebugLog(cStringRO file, cStringRO fmt, ...) {
    va_list argptr;         va_start(argptr, fmt);
    static char data[1024]; vsnprintf(data, sizeof(data), fmt, argptr);
    va_end(argptr);
    //    fd = open(file, O_WRONLY | O_BINARY | O_CREAT | O_APPEND, 0666);
    int fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    write(fd, data, strlen(data));
    close(fd);
}

void Sys_EditFile(cString filename) {
    char cmd[256];

    cString term = getenv("TERM");
    if ((term) && (!strcmp(term, "xterm"))) {
        cString editor = getenv("VISUAL");
        if (!editor)    editor = getenv("EDITOR");
        if (!editor)    editor = getenv("EDIT");
        if (!editor)    editor = "vi";
        sprintf(cmd, "xterm -e %s %s", editor, filename);
        system(cmd);
    }

}
