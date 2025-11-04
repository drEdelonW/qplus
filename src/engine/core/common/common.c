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
#include "cvar_q1.h"
#include <string.h>
#include <stdarg.h>
#include "sys.h"
#include "Pak.h"
#include "console.h"
#include "cmd.h"
#include "draw.h"
#include "endian_tools.h"
#include "host.h"
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


static bool     _progHack;
static int32_t  _Registered = 1;  // only for startup check, then set

bool  msg_suppress_1 = false; // supress System messages

// if a packfile directory differs from this, it is assumed to be hacked



common_t com;

bool standard_quake = true;
bool rogue;
bool hipnotic;

// this graphic needs to be in the pak file to use registered features
uint16_t pop[] = {
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000,
    0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000,
    0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600,
    0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563,
    0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564,
    0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564,
    0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563,
    0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500,
    0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200,
    0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000,
    0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in QuakeParms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently.  This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because they are added at the end, they will not override an explicit setting on the original command line.

*/



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
        strncpy(out, s2 + 1, s - s2);
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
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void COM_CheckRegistered() {
    int h;
    COM_OpenFile("gfx/pop.lmp", &h);
    _Registered = 0;

    if (h == -1) {
#if WINDED
        Sys_Error("This dedicated server requires a full registered copy of Quake");
#endif
        Con_Printf("Playing shareware version.\n");
        if (contModified)
            Sys_Error("You must have the registered version to use modified games");
        return;
    }

    uint16_t  check[128];
    Sys_FileRead(h, check, sizeof(check));
    COM_CloseFile(h);

    for (int i = 0; i < 128; i++)
        if (pop[i] != (uint16_t)BigShort(check[i]))
            Sys_Error("Corrupted data file.");

    Cvar_Set("cmdline", com.cmdline);
    Cvar_Set("registered", "1");
    _Registered = 1;
    Con_Printf("Playing registered version.\n");
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
    static char string[1024];   vsprintf(string, format, argptr);
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
void COM_WriteFile(cStringRO filename, TypeLess_ptr data, int32_t len) {
    char    name[MAX_OSPATH];
    snprintf(name, sizeof(name), "%s/%s", com.gamedir, filename);

    int handle = Sys_FileOpenWrite(name);
    if (handle == -1) {
        Sys_Printf("COM_WriteFile: failed on %s\n", name);
        return;
    }

    Sys_Printf("COM_WriteFile: %s\n", name);
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
            *ofs = 0;
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
    int remaining = Sys_FileOpenRead(netpath, &in);
    COM_CreatePath(cachepath);     // create directories up to the cache file
    int out = Sys_FileOpenWrite(cachepath);

    while (remaining) {
        char buf[4096];
        int count = (remaining < sizeof(buf)) ?
            remaining : sizeof(buf);
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
int COM_FindFile(cStringRO filename, int* handle, FILE** file) {
    if (file && handle)     Sys_Error("COM_FindFile: both handle and file set");
    if (!file && !handle)   Sys_Error("COM_FindFile: neither handle or file set");

    //
    // search through the path, one element at a time
    //
    searchpath_p search = com_searchpaths;
    if (_progHack) { // gross hack to use quake 1 progs with quake 2 maps
        if (!strcmp(filename, "progs.dat"))
            search = search->next;
    }

    for (; search; search = search->next) {
        // is the element a pak file?
        if (search->pack) {
            // look through all the pak file elements
            pack_p pak = search->pack;
            for (int idxPak = 0; idxPak < pak->numfiles; idxPak++)
                if (!strcmp(pak->files[idxPak].name, filename)) {       // found it!
                    Sys_Printf("PackFile: %s : %s\n", pak->filename, filename);
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
            if (!_Registered) {       // if not a registered version, don't ever go beyond base
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

            Sys_Printf("FindFile: %s\n", netpath);
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

    Sys_Printf("FindFile: can't find %s\n", filename);

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
        if (s->pack && s->pack->handle == h)
            return;

    Sys_FileClose(h);
}


/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 uint8_t.
============
*/
CacheUser_p loadcache;
uint8_p     loadbuf;
int         loadsize;
uint8_p COM_LoadFile(cStringRO path, int usehunk) {
    uint8_p buf = NULL;     // quiet compiler warning

    // look for it in the filesystem or pack files
    int h;
    int len = COM_OpenFile(path, &h);
    if (h == -1)    return NULL;

    // extract the filename base name for hunk tag
    char    base[32];
    COM_FileBase(path, base);

    if (usehunk == 1)       buf = Hunk_AllocName(len + 1, base);
    else if (usehunk == 2)  buf = Hunk_TempAlloc(len + 1);
    else if (usehunk == 0)  buf = Z_Malloc(len + 1);
    else if (usehunk == 3)  buf = Cache_Alloc(loadcache, len + 1, base);
    else if (usehunk == 4) {
        if (len + 1 > loadsize) buf = Hunk_TempAlloc(len + 1);
        else                    buf = loadbuf;
    }
    else        Sys_Error("COM_LoadFile: bad usehunk");

    if (!buf)   Sys_Error("COM_LoadFile: not enough space for %s", path);

    ((uint8_p)buf)[len] = 0;

    Draw_BeginDisc();
    Sys_FileRead(h, buf, len);
    COM_CloseFile(h);
    Draw_EndDisc();

    return buf;
}

uint8_p COM_LoadHunkFile(cStringRO path) { return COM_LoadFile(path, 1); }
uint8_p COM_LoadTempFile(cStringRO path) { return COM_LoadFile(path, 2); }

void COM_LoadCacheFile(cStringRO path, CacheUser_p cu) {
    loadcache = cu;
    COM_LoadFile(path, 3);
}

// uses temp hunk if larger than bufsize
uint8_p COM_LoadStackFile(cStringRO path, TypeLess_ptr buffer, int32_t bufsize) {
    loadbuf = (uint8_p)buffer;
    loadsize = bufsize;
    uint8_p buf = COM_LoadFile(path, 4);

    return buf;
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
    char basedir[MAX_OSPATH];

    //
    // -basedir <path>
    // Overrides the system supplied base directory (under GAMENAME)
    //
    int param = COM_CheckParm("-basedir");
    if (param && (param < com.argc - 1))    strcpy(basedir, com.argv[param + 1]);
    else                                    strcpy(basedir, host_parms.basedir);

    int st_len = strlen(basedir);

    if (st_len > 0) {
        if ((basedir[st_len - 1] == '\\') ||
            (basedir[st_len - 1] == '/'))
            basedir[st_len - 1] = 0;
    }

    //
    // -cachedir <path>
    // Overrides the system supplied cache directory (NULL or /qcache)
    // -cachedir - will disable caching.
    //
    param = COM_CheckParm("-cachedir");
    if (param && (param < com.argc - 1)) {
        if (com.argv[param + 1][0] == '-')  com_cachedir[0] = 0;
        else                         strcpy(com_cachedir, com.argv[param + 1]);
    }
    else if (host_parms.cachedir)    strcpy(com_cachedir, host_parms.cachedir);
    else                                    com_cachedir[0] = 0;

    //
    // start up with GAMENAME by default (id1)
    //
    COM_AddGameDirectory(va("%s/"GAMENAME, basedir));

    if (COM_CheckParm("-rogue"))    COM_AddGameDirectory(va("%s/rogue", basedir));
    if (COM_CheckParm("-hipnotic")) COM_AddGameDirectory(va("%s/hipnotic", basedir));

    //
    // -game <gamedir>
    // Adds basedir/gamedir as an override game
    //
    param = COM_CheckParm("-game");
    if (param && (param < com.argc - 1)) {
        contModified = true;
        COM_AddGameDirectory(va("%s/%s", basedir, com.argv[param + 1]));
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
                if (!search->pack)      Sys_Error("Couldn't load packfile: %s", com.argv[param]);
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
    COM_Endian_Init();

    Cvar_RegisterVariable(&registered);
    Cvar_RegisterVariable(&cmdline);
    Cmd_AddCommand("path", COM_Path_f);

    COM_InitFilesystem();
    COM_CheckRegistered();
}
