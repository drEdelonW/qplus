/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/
// cmdlib.c

#include "cmdlib.h"
#include <sys/time.h>
#include <unistd.h>

#define PATHSEPERATOR   '/'

char com_token[1024];
int  com_eof;

/*
================
I_FloatTime
================
*/
double I_FloatTime() {
    struct timeval tp;
    struct timezone tzp;
    gettimeofday(&tp, &tzp);

    static int secbase = 0;
    if (!secbase) {
        secbase = tp.tv_sec;
        return tp.tv_usec / 1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
cStr_p COM_Parse(cStr_p data) {
    if (!data)
        return NULL;

    char ch;
skipwhite:
    while ((ch = *data) <= ' ') {    // skip whitespace
        if (ch == 0) {
            com_eof = true;
            return NULL;   // end of file;
        }
        data++;
    }

    // skip // comments
    if ((ch == '/') &&
        (data[1] == '/')
        ) {
        while ((*data) &&
            (*data != '\n') // until new line
            )   data++;

        goto skipwhite;
    }


    // handle quoted strings specially
    int len = 0;
    com_token[len] = 0;
    if (ch == '\"') {
        data++;
        do {
            ch = *data++;
            if (ch == '\"') {
                com_token[len] = 0;
                return data;
            }
            com_token[len] = ch;
            len++;
        } while (1);
    }

    // parse single characters
    if ((ch == '{') ||
        (ch == '}') ||
        (ch == ')') ||
        (ch == '(') ||
        (ch == '\'') ||
        (ch == ':')
        ) {
        com_token[len] = ch;
        len++;
        com_token[len] = 0x00;
        return data + 1;
    }

    // parse a regular word
    do {
        com_token[len] = ch;
        data++;
        len++;
        ch = *data;
        if ((ch == '{') ||
            (ch == '}') ||
            (ch == ')') ||
            (ch == '(') ||
            (ch == '\'') ||
            (ch == ':')
            )   break;
    } while (ch > ' ');

    com_token[len] = 0;
    return data;
}




/*
================
filelength
================
*/
int filelength(int handle) {
    struct stat fileinfo;
    if (fstat(handle, &fileinfo) == -1)
        Error("Error fstating");

    return fileinfo.st_size;
}

int tell(int handle) {
    return lseek(handle, 0, SEEK_CUR);
}

cStr_p strupr(cStr_p start) {
    cStr_p in;
    in = start;
    while (*in) {
        *in = toupper(*in);
        in++;
    }
    return start;
}

cStr_p strlower(cStr_p start) {
    cStr_p in = start;
    while (*in) {
        *in = tolower(*in);
        in++;
    }
    return start;
}


/*
=============================================================================

                        MISC FUNCTIONS

=============================================================================
*/

/*
=================
Error

For abnormal program terminations
=================
*/
void Error(cStr_p error, ...) { // TODO: rework it to "va.h"
    printf("\n************ ERROR ************\n");
    va_list argptr; va_start(argptr, error); {
        vprintf(error, argptr);
    } va_end(argptr);
    printf("\n");
    exit(1);
}


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
// set these before calling CheckParm
int myargc;
cStr_ar myargv;
int CheckParm(cStr_p check) {
    for (int i = 1; i < myargc; i++)
        if (!strcasecmp(check, myargv[i]))
            return i;

    return 0;
}


#ifndef O_BINARY
#define O_BINARY 0
#endif

int SafeOpenWrite(cStr_p filename) {
    umask(0);
    int handle = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
    if (handle == -1)
        Error("Error opening %s: %s", filename, strerror(errno));

    return handle;
}

int SafeOpenRead(cStr_p filename) {
    int handle = open(filename, O_RDONLY | O_BINARY);
    if (handle == -1)
        Error("Error opening %s: %s", filename, strerror(errno));

    return handle;
}


void SafeRead(int handle, Any_p buffer, long count) {
    if (read(handle, buffer, count) != count)
        Error("File read failure");
}


void SafeWrite(int handle, Any_p buffer, long count) {
    if (write(handle, buffer, count) != count)
        Error("File write failure");
}


Any_p SafeMalloc(long size) {
    Any_p ptr = malloc(size);
    if (!ptr)
        Error("Malloc failure for %lu bytes", size);
    return ptr;
}


/*
==============
LoadFile
==============
*/
long LoadFile(cStr_p filename, void** bufferptr) {
    int handle = SafeOpenRead(filename);
    long length = filelength(handle);
    Any_p buffer = SafeMalloc(length + 1);
    ((uint8_p)buffer)[length] = 0;
    SafeRead(handle, buffer, length);
    close(handle);

    *bufferptr = buffer;
    return length;
}


/*
==============
SaveFile
==============
*/
void SaveFile(cStr_p filename, Any_p buffer, long count) {
    int handle = SafeOpenWrite(filename);
    SafeWrite(handle, buffer, count);
    close(handle);
}



void DefaultExtension(cStr_p path, cStr_p extension) {
    // if path doesn't have a .EXT, append extension
    // (extension should include the .)
    cStr_p src = path + strlen(path) - 1;

    while ((*src != PATHSEPERATOR) &&
        (src != path)
        ) {
        if (*src == '.')
            return;                 // it has an extension
        src--;
    }

    strcat(path, extension);
}


void DefaultPath(cStr_p path, cStr_p basepath) {
    if (path[0] == PATHSEPERATOR)
        return;                   // absolute path location
    char temp[128];
    strcpy(temp, path);
    strcpy(path, basepath);
    strcat(path, temp);
}


void StripFilename(cStr_p path) {
    int length = strlen(path) - 1;
    while ((length > 0) &&
        (path[length] != PATHSEPERATOR)
        )   length--;
    path[length] = 0;
}

void StripExtension(cStr_p path) {
    int  length = strlen(path) - 1;
    while ((length > 0) &&
        (path[length] != '.')
        ) {
        length--;
        if (path[length] == '/')
            return;  // no extension
    }
    if (length)
        path[length] = 0;
}


/*
====================
Extract file parts
====================
*/
void ExtractFilePath(cStr_p path, cStr_p dest) {
    cStr_p src = path + strlen(path) - 1;

    // back up until a \ or the start
    while ((src != path) &&
        (*(src - 1) != PATHSEPERATOR)
        )   src--;

    memcpy(dest, path, src - path);
    dest[src - path] = 0;
}

void ExtractFileBase(cStr_p path, cStr_p dest) {
    cStr_p src = path + strlen(path) - 1;
    // back up until a \ or the start
    while ((src != path) &&
        (*(src - 1) != PATHSEPERATOR)
        )   src--;

    while ((*src) &&
        (*src != '.')
        )   *dest++ = *src++;

    *dest = 0;
}

void ExtractFileExtension(cStr_p path, cStr_p dest) {
    cStr_p src = path + strlen(path) - 1;
    // back up until a . or the start
    while ((src != path) &&
        (*(src - 1) != '.')
        )   src--;
    if (src == path) {
        *dest = 0; // no extension
        return;
    }

    strcpy(dest, src);
}


/*
==============
ParseNum / ParseHex
==============
*/
long ParseHex(cStr_p hex) {
    long num = 0;
    cStr_p str = hex;
    while (*str) {
        num <<= 4;
        /**/ if ((*str >= '0') && (*str <= '9'))    num += 0 + *str - '0';
        else if ((*str >= 'a') && (*str <= 'f'))    num += 10 + *str - 'a';
        else if ((*str >= 'A') && (*str <= 'F'))    num += 10 + *str - 'A';
        else    Error("Bad hex number: %s", hex);
        str++;
    }

    return num;
}


long ParseNum(cStr_p str) {
    if (str[0] == '$')                      return ParseHex(str + 1);
    if ((str[0] == '0') && (str[1] == 'x')) return ParseHex(str + 2);
    return atol(str);
}



/*
============================================================================

                    BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef __BIG_ENDIAN__

short   LittleShort(short l) {
    uint8_t    b1, b2;

    b1 = l & 0xFF;
    b2 = (l >> 8) & 0xFF;

    return (b1 << 8) + b2;
}

short   BigShort(short l) {
    return l;
}


long    LittleLong(long l) {
    uint8_t    b1, b2, b3, b4;

    b1 = l & 0xFF;
    b2 = (l >> 8) & 0xFF;
    b3 = (l >> 16) & 0xFF;
    b4 = (l >> 24) & 0xFF;

    return ((long)b1 << 24) + ((long)b2 << 16) + ((long)b3 << 8) + b4;
}

long    BigLong(long l) {
    return l;
}


float LittleFloat(float l) {
    union { uint8_t b[4]; float f; } in, out;

    in.f = l;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];

    return out.f;
}

float BigFloat(float l) {
    return l;
}


#else


short   BigShort(short l) {
    uint8_t b1 = (l >> 0) & 0xFF;
    uint8_t b2 = (l >> 8) & 0xFF;

    return (b1 << 8) + b2;
}

short   LittleShort(short l) {
    return l;
}


long BigLong(long l) {
    uint8_t    b1, b2, b3, b4;

    b1 = (l >> 0) & 0xFF;
    b2 = (l >> 8) & 0xFF;
    b3 = (l >> 16) & 0xFF;
    b4 = (l >> 24) & 0xFF;

    return ((long)b1 << 24) + ((long)b2 << 16) + ((long)b3 << 8) + b4;
}

long    LittleLong(long l) {
    return l;
}

float BigFloat(float l) {
    union {
        uint8_t b[4];
        float f;
    } in, out;

    in.f = l;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];

    return out.f;
}

float LittleFloat(float l) {
    return l;
}



#endif

