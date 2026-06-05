/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// common.c -- misc functions used in client and server

#include "common.h"
#include "host.h"
#include <string.h>
#include <stdarg.h>
#include "sys.h"
#include "Pak.h"
#include "console.h"
#include "cmd.h"
#include "draw.h"
#include "endian_tools.h"
#include "gamedefs.h"
#include "q_tools.h"


#define NUM_SAFE_ARGVS  7

static cString   _lArgV[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static cStringRO _argVDummy = " ";
static cStringRO _safeArgvs[NUM_SAFE_ARGVS] = {
    "-stdvid",
    "-nolan",
    "-nosound",
    "-nocdaudio",
    "-nojoy",
    "-nomouse",
    "-dibonly"
};



bool  msg_suppress_1 = false; // supress System messages

// if a packfile directory differs from this, it is assumed to be hacked


common_t com;

//============================================================================


/*
============
COM_SkipPath
============
*/
cString COM_SkipPath(cStringRO pathname) {
    cString last = (cString)pathname;
    while (*pathname) {
        if (*pathname == '/')
            last = (cString)pathname + 1;
        pathname++;
    }
    return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(cStringRO in, cString out) {
    while ((*in) && (*in != '.'))
        *out++ = *in++;
    *out = 0;
}

/*
============
COM_FileExtension
============
*/
cString COM_FileExtension(cStringRO in) {
    static char exten[8];
    while ((*in) && (*in != '.'))
        in++;

    if (!*in)
        return "";

    in++;

    int i = 0;
    for (; (i < 7) && (*in); i++, in++)
        exten[i] = *in;
    exten[i] = 0;
    return exten;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase(cStringRO in, cString out) {
    cString s = (cString)in + strlen(in) - 1;

    cString s2;
    while ((s != in) && (*s != '.'))
        s--;

    for (s2 = s; (*s2) && (*s2 != '/'); s2--)
        ;

    if ((s - s2) < 2)   strcpy(out, "?model?");
    else {
        s--;
        strncpy(out, s2 + 1u, (size_t)(s - s2));
        out[s - s2] = 0;
    }
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(cStringRO path, cString extension) {
    //
    // if path doesn't have a .EXT, append extension
    // (extension should include the .)
    //
    cString src = (cString)path + strlen(path) - 1;

    while ((*src != '/') && (src != path)) {
        if (*src == '.')
            return;                 // it has an extension
        src--;
    }

    strcat((cString)path, extension);
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
cString COM_Parse(cString data) {
    char c;

    int len = 0;
    com.token[0] = 0;

    if (!data)
        return NULL;

    // skip whitespace
skipwhite:
    while ((c = *data) <= ' ') {
        if (c == 0)
            return NULL;                    // end of file;
        data++;
    }

    // skip // comments
    if ((c == '/') && (data[1] == '/')) {
        while ((*data) && (*data != '\n'))
            data++;
        goto skipwhite;
    }


    // handle quoted strings specially
    if (c == '\"') {
        data++;
        while (1) {
            c = *data++;
            if ((c == '\"') || (!c)) {
                com.token[len] = 0;
                return data;
            }
            com.token[len] = c;
            len++;
        }
    }

    // parse single characters
    if ((c == '{') ||
        (c == '}') ||
        (c == ')') ||
        (c == '(') ||
        (c == '\'') ||
        (c == ':')
        ) {
        com.token[len] = c;
        len++;
        com.token[len] = 0;
        return data + 1;
    }

    // parse a regular word
    do {
        com.token[len] = c;
        data++;
        len++;
        c = *data;
        if ((c == '{') ||
            (c == '}') ||
            (c == ')') ||
            (c == '(') ||
            (c == '\'') ||
            (c == ':'))
            break;
    } while (c > 32);

    com.token[len] = 0;
    return data;
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm(cStringRO parm) {
    for (int i = 1; i < com.argc; i++) {
        if (!com.argv[i])                   continue;   // NEXTSTEP sometimes clears appkit vars.
        if (!Q_strcmp(parm, com.argv[i]))   return i;
    }

    return 0;
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv(int argc, cStringArray argv) {
    // reconstitute the command line for the cmdline externally visible cvar
    int n = 0;
    for (int j = 0; (j < MAX_NUM_ARGVS) && (j < argc); j++) {
        int i = 0;

        while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i]) {
            com.cmdline[n++] = argv[j][i++];
        }

        if (n < (CMDLINE_LENGTH - 1))
            com.cmdline[n++] = ' ';
        else
            break;
    }

    com.cmdline[n] = 0;

    bool safe = false;

    for (com.argc = 0; (com.argc < MAX_NUM_ARGVS) && (com.argc < argc);
        com.argc++) {
        _lArgV[com.argc] = argv[com.argc];
        if (!Q_strcmp("-safe", argv[com.argc]))
            safe = true;
    }

    if (safe) {
        // force all the safe-mode switches. Note that we reserved extra space in
        // case we need to add these, so we don't need an overflow check
        for (int i = 0; i < NUM_SAFE_ARGVS; i++) {
            _lArgV[com.argc] = (cString)_safeArgvs[i];
            com.argc++;
        }
    }

    _lArgV[com.argc] = (cString)_argVDummy;
    com.argv = _lArgV;

    if (COM_CheckParm("-rogue")) {
        rogue = true;
        standard_quake = false;
    }

    if (COM_CheckParm("-hipnotic")) {
        hipnotic = true;
        standard_quake = false;
    }
}




/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
cString va(cStringRO format, ...) {
    va_list argptr;             va_start(argptr, format);
    static char string[1024];   vsnprintf(string, sizeof(string), format, argptr);
    va_end(argptr);

    return string;
}


/// just for debugging
int32_t memsearch(uint8_p start, int32_t count, int32_t search) {
    for (int32_t i = 0; i < count; i++)
        if (start[i] == search)
            return i;
    return -1;
}


char com_cachedir[MAX_OSPATH];

typedef struct searchpath_s searchpath_t;
typedef searchpath_t* searchpath_p;
struct searchpath_s {
    char         filename[MAX_OSPATH];
    pack_p       pack;   // only one of filename / pack will be used
    searchpath_p next;
};

searchpath_p com_searchpaths;

/*
============
COM_Path_f

============
*/
void COM_Path_f() {
    Con_Printf("Current search path:\n");
    for (searchpath_p s = com_searchpaths; s; s = s->next) {
        if (s->pack)    Con_Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
        else            Con_Printf("%s\n", s->filename);
    }
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile(cStringRO filename, TypeLess_ptr data, size_t len) {
    char name[MAX_OSPATH];
    snprintf(name, sizeof(name), "%s/%s", com.gamedir, filename);

    int handle = Sys_FileOpenWrite(name);
    if (handle == -1) {
        Host_Printf("COM_WriteFile: failed on %s\n", name);
        return;
    }

    Host_Printf("COM_WriteFile: %s\n", name);
    Sys_FileWrite(handle, data, len);
    Sys_FileClose(handle);
}


/*
============
COM_CreatePath

Only used for CopyFile
============
*/
void COM_CreatePath(cStringRO path) {
    for (cString ofs = (cString)path + 1; *ofs; ofs++) {
        if (*ofs == '/') {       // create the directory
            *ofs = 0x00;
            Sys_mkdir(path);
            *ofs = '/';
        }
    }
}


/*
===========
COM_CopyFile

Copies a file over from the net to the local cache, creating any directories
needed.  This is for the convenience of developers using ISDN from home.
===========
*/
void COM_CopyFile(cString netpath, cString cachepath) {
    int in;
    int ret = Sys_FileOpenRead(netpath, &in);
    if (ret == -1) return; // ERROR

    size_t remaining = (size_t)ret;
    COM_CreatePath(cachepath);     // create directories up to the cache file
    int out = Sys_FileOpenWrite(cachepath);

    while (remaining) {
        char buf[4096];
        size_t count = (remaining < sizeof(buf)) ?
            remaining : (int)sizeof(buf);
        Sys_FileRead(in, buf, count);
        Sys_FileWrite(out, buf, count);
        remaining -= count;
    }

    Sys_FileClose(in);
    Sys_FileClose(out);
}

/*
===========
COM_FindFile

Finds the file in the search path.
Sets com.filesize and one of handle or file
===========
*/
static bool     _progHack;
#include "terminal_tools.h"

int COM_FindFile(cStringRO filename, int* handle, FILE** file) {
    if (file && handle)         Host_SysError("COM_FindFile: both handle and file set");
    if (!file && !handle)       Host_SysError("COM_FindFile: neither handle or file set");

    //
    // search through the path, one element at a time
    //
    searchpath_p search = com_searchpaths;
    if ((_progHack) && // gross hack to use quake 1 progs with quake 2 maps
        (!strcmp(filename, "progs.dat"))
        ) {
        search = search->next;
    }

    for (; search; search = search->next) {
        // is the element a pak file?
        if (search->pack) {
            // look through all the pak file elements
            pack_p pak = search->pack;
            for (int idxPak = 0; idxPak < pak->numfiles; idxPak++)
                if (!strcmp(pak->files[idxPak].name, filename)) {       // found it!
                    Host_Printf("PackFile: %s : %s\n", pak->filename, filename);
                    if (handle) {
                        *handle = pak->handle;
                        Sys_FileSeek(pak->handle, pak->files[idxPak].filepos);
                    }
                    else {       // open a new file on the pakfile
                        *file = fopen(pak->filename, "rb");
                        if (*file)
                            fseek(*file, pak->files[idxPak].filepos, SEEK_SET);
                    }
                    com.filesize = pak->files[idxPak].filelen;
                    return com.filesize;
                }
        }
        else {
            // check a file in the directory tree
            if (!Registered) {       // if not a registered version, don't ever go beyond base
                if (strchr(filename, '/') ||
                    strchr(filename, '\\'))
                    continue;
            }

            char netpath[MAX_OSPATH];
            snprintf(netpath, sizeof(netpath), "%s/%s", search->filename, filename);

            int findtime = Sys_FileTime(netpath);
            if (findtime == -1)
                continue;

            // see if the file needs to be updated in the cache
            char cachepath[MAX_OSPATH];
            if (!com_cachedir[0])
                strcpy(cachepath, netpath);
            else {
#if defined(_WIN32)
                if ((strlen(netpath) < 2) || (netpath[1] != ':'))
                    snprintf(cachepath, sizeof(cachepath), "%s%s", com_cachedir, netpath);
                else
                    snprintf(cachepath, sizeof(cachepath), "%s%s", com_cachedir, netpath + 2);
#else
                snprintf(cachepath, sizeof(cachepath), "%s%s", com_cachedir, netpath);
#endif

                int cachetime = Sys_FileTime(cachepath);

                if (cachetime < findtime)
                    COM_CopyFile(netpath, cachepath);
                strcpy(netpath, cachepath);
            }

            Host_Printf("FindFile: %s\n", netpath);
            int fHandle;

            com.filesize = Sys_FileOpenRead(netpath, &fHandle);
            if (handle)
                *handle = fHandle;
            else {
                Sys_FileClose(fHandle);
                *file = fopen(netpath, "rb");
            }
            return com.filesize;
        }

    }

    Host_Printf("FindFile: can't find %s\n", filename);

    if (handle) *handle = -1;
    else        *file = NULL;

    com.filesize = -1;
    return -1;
}


/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
int COM_OpenFile(cStringRO filename, int* handle) {
    return COM_FindFile(filename, handle, NULL);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int COM_FOpenFile(cStringRO filename, FILE** file) {
    return COM_FindFile(filename, NULL, file);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile(int h) {
    for (searchpath_p s = com_searchpaths; s; s = s->next)
        if (s->pack && (s->pack->handle == h))
            return;

    Sys_FileClose(h);
}

typedef enum ComLoadHunk_e {
    HUNK_ZMALLOC = 0u,   // Z_Malloc
    HUNK_HUNK = 1u,   // Hunk_AllocName
    HUNK_TEMP = 2u,   // Hunk_TempAlloc
    HUNK_CACHE = 3u,   // Cache_Alloc
    HUNK_STACK = 4u    // stack buffer, fallback to temp hunk
} ComLoadHunk_t;

/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 uint8_t.
============
*/
CacheUser_p loadcache;
uint8_p     loadbuf;
size_t      loadsize;
uint8_p COM_LoadFile(cStringRO path, ComLoadHunk_t usehunk) {
    uint8_p buf = NULL;     // quiet compiler warning

    // look for it in the filesystem or pack files
    int h;
    size_t len = (size_t)COM_OpenFile(path, &h);
    if (h == -1)    return NULL;

    // extract the filename base name for hunk tag
    char    base[32];
    COM_FileBase(path, base);

#if 0
    if (usehunk == 0)       buf = Z_Malloc(len + 1);
    else if (usehunk == 1)  buf = Hunk_AllocName(len + 1, base);
    else if (usehunk == 2)  buf = Hunk_TempAlloc(len + 1);
    else if (usehunk == 3)  buf = Cache_Alloc(loadcache, len + 1, base);
    else if (usehunk == 4) {
        if (len + 1 > loadsize) buf = Hunk_TempAlloc(len + 1);
        else                    buf = loadbuf;
    }
    else        Host_SysError("COM_LoadFile: bad usehunk");
#else
    switch (usehunk) {
    case HUNK_ZMALLOC:  buf = Z_Malloc(len + 1);                        break;
    case HUNK_HUNK:     buf = Hunk_AllocName(len + 1, base);            break;
    case HUNK_TEMP:     buf = Hunk_TempAlloc(len + 1);                  break;
    case HUNK_CACHE:    buf = Cache_Alloc(loadcache, len + 1, base);    break;
    case HUNK_STACK:    buf = ((len + 1) > loadsize) ? Hunk_TempAlloc(len + 1) : loadbuf; break;
    default:            Host_SysError("COM_LoadFile: bad usehunk");     break;
    }
#endif
    if (!buf)           Host_SysError("COM_LoadFile: not enough space for %s", path);

    ((uint8_p)buf)[len] = 0x00;

    Draw_BeginDisc();
    Sys_FileRead(h, buf, len);
    COM_CloseFile(h);
    Draw_EndDisc();

    return buf;
}

uint8_p COM_LoadHunkFile(cStringRO path) { return COM_LoadFile(path, HUNK_HUNK); }
uint8_p COM_LoadTempFile(cStringRO path) { return COM_LoadFile(path, HUNK_TEMP); }

void COM_LoadCacheFile(cStringRO path, CacheUser_p cu) {
    loadcache = cu;
    COM_LoadFile(path, HUNK_CACHE);
}

// uses temp hunk if larger than bufsize
uint8_p COM_LoadStackFile(cStringRO path, TypeLess_ptr buffer, size_t bufsize) {
    loadbuf = (uint8_p)buffer;
    loadsize = bufsize;
    return COM_LoadFile(path, HUNK_STACK);

}


/*
================
COM_AddGameDirectory

Sets com.gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
void COM_AddGameDirectory(cStringRO dir) {
    strcpy(com.gamedir, dir);

    //
    // add the directory to the search path
    //
    searchpath_p search = Hunk_Alloc(sizeof(searchpath_t));
    strcpy(search->filename, dir);
    search->next = com_searchpaths;
    com_searchpaths = search;

    //
    // add any pak files in the format pak0.pak pak1.pak, ...
    //
    for (int i = 0; ; i++) {
        char pakfile[MAX_OSPATH];
        snprintf(pakfile, sizeof(pakfile), "%s/pak%i.pak", dir, i);
        pack_p pak = COM_LoadPackFile(pakfile);
        if (!pak)
            break;
        search = Hunk_Alloc(sizeof(searchpath_t));
        search->pack = pak;
        search->next = com_searchpaths;
        com_searchpaths = search;
    }

    //
    // add the contents of the parms.txt file to the end of the command line
    //

}

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem() {
    static char _baseDir[MAX_OSPATH];

    //
    // -basedir <path>
    // Overrides the system supplied base directory (under GAMENAME)
    //
    int param = COM_CheckParm("-basedir");
    if (param && (param < com.argc - 1))    strcpy(_baseDir, com.argv[param + 1]);
    else                                    strcpy(_baseDir, host_parms.baseDir);

    size_t st_len = strlen(_baseDir);

    if (st_len > 0) {
        if ((_baseDir[st_len - 1] == '\\') ||
            (_baseDir[st_len - 1] == '/'))
            _baseDir[st_len - 1] = 0;
    }

    //
    // -cachedir <path>
    // Overrides the system supplied cache directory (NULL or /qcache)
    // -cachedir - will disable caching.
    //
    param = COM_CheckParm("-cachedir");
    if (param && (param < com.argc - 1)) {
        if (com.argv[param + 1][0] == '-')  com_cachedir[0] = 0x00;
        else                         strcpy(com_cachedir, com.argv[param + 1]);
    }
    else if (host_parms.cacheDir)    strcpy(com_cachedir, host_parms.cacheDir);
    else                                    com_cachedir[0] = 0x00;

    //
    // start up with GAMENAME by default (id1)
    //
    COM_AddGameDirectory(va("%s/"GAMENAME, _baseDir));

    if (COM_CheckParm("-rogue"))    COM_AddGameDirectory(va("%s/rogue", _baseDir));
    if (COM_CheckParm("-hipnotic")) COM_AddGameDirectory(va("%s/hipnotic", _baseDir));

    //
    // -game <gamedir>
    // Adds basedir/gamedir as an override game
    //
    param = COM_CheckParm("-game");
    if (param && (param < com.argc - 1)) {
        contModified = true;
        COM_AddGameDirectory(va("%s/%s", _baseDir, com.argv[param + 1]));
    }

    //
    // -path <dir or packfile> [<dir or packfile>] ...
    // Fully specifies the exact serach path, overriding the generated one
    //
    param = COM_CheckParm("-path");
    if (param) {
        contModified = true;
        com_searchpaths = NULL;
        while (++param < com.argc) {
            if (!com.argv[param] ||
                (com.argv[param][0] == '+') ||
                (com.argv[param][0] == '-'))
                break;

            searchpath_p search = Hunk_Alloc(sizeof(searchpath_t));
            if (!strcmp(COM_FileExtension(com.argv[param]), "pak")) {
                search->pack = COM_LoadPackFile(com.argv[param]);
                if (!search->pack)      Host_SysError("Couldn't load packfile: %s", com.argv[param]);
            }
            else    strcpy(search->filename, com.argv[param]);
            search->next = com_searchpaths;
            com_searchpaths = search;
        }
    }

    if (COM_CheckParm("-proghack")) _progHack = true;
}


/*
================
COM_Init
================
*/

void COM_Init(cStringRO basedir) {
    Endian_Init();

    GM_GameInit();
    Cmd_AddCommand("path", COM_Path_f);

    COM_InitFilesystem();
    GM_CheckRegistered();
}
