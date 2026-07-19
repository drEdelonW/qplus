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
// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdarg.h>

#ifdef NeXT
#include <libc.h>
#endif

#include "types.h"

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)


// set these before calling CheckParm
extern int myargc;
extern cStr_ar myargv;

cStr_p strupr(cStr_p in);
cStr_p strlower(cStr_p in);
int filelength(int handle);
int tell(int handle);

double I_FloatTime();

void Error(cStr_p error, ...);
int  CheckParm(cStr_p check);

int  SafeOpenWrite(cStr_p filename);
int  SafeOpenRead(cStr_p filename);
void  SafeRead(int handle, Any_p buffer, long count);
void  SafeWrite(int handle, Any_p buffer, long count);
Any_p SafeMalloc(long size);

long LoadFile(cStr_p filename, cStr_ar bufferptr);
void SaveFile(cStr_p filename, Any_p buffer, long count);

void  DefaultExtension(cStr_p path, cStr_p extension);
void  DefaultPath(cStr_p path, cStr_p basepath);
void  StripFilename(cStr_p path);
void  StripExtension(cStr_p path);

void  ExtractFilePath(cStr_p path, cStr_p dest);
void  ExtractFileBase(cStr_p path, cStr_p dest);
void ExtractFileExtension(cStr_p path, cStr_p dest);

long  ParseNum(cStr_p str);

short BigShort(short l);
short LittleShort(short l);
long BigLong(long l);
long LittleLong(long l);
float BigFloat(float l);
float LittleFloat(float l);


cStr_p COM_Parse(cStr_p data);

extern char com_token[1024];
extern int  com_eof;



#endif
