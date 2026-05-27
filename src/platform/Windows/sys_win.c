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
// sys_win.c -- Win32 system interface code

#include "sys.h"
#include "host.h"
#include "console.h"
#include "common.h"
#include "client.h"
#include "screen.h"
#include "q_tools.h"
#include "winquake.h"
#include "errno.h"
#include "resource.h"
#include "conproc.h"

#define MINIMUM_WIN_MEMORY  0x0880000
#define MAXIMUM_WIN_MEMORY  0x1000000

#define CONSOLE_ERROR_TIMEOUT 60.0 // # of seconds to wait on Sys_Error running
                                        //  dedicated before exiting
#define PAUSE_SLEEP  50    // sleep time on pause or minimization
#define NOT_FOCUS_SLEEP 20    // sleep time when not focus

int         starttime;
bool    ActiveApp, Minimized;
bool    WinNT;

static double   _pfreq;
static LegacyTimeStamp_t   _curtime = 0.0;
static LegacyTimeStamp_t   _lastcurtime = 0.0;
static int      _lowshift;
static bool _ScReturnOnEnter = false;
HANDLE          hinput, houtput;

// static cString _trackingTag = "Clams & Mooses";

static HANDLE _tEvent;
static HANDLE _hFile;
static HANDLE _hEventParent;
static HANDLE _hEventChild;

void MaskExceptions();
void Sys_InitFloatTime();
void Sys_PushFPCW_SetHigh();
void Sys_PopFPCW();

volatile int     sys_checksum;


/*
================
Sys_PageIn
================
*/
void Sys_PageIn(TypeLess_ptr ptr, int size) {
    // touch all the memory to make sure it's there. The 16-page skip is to
    // keep Win 95 from thinking we're trying to page ourselves in (we are
    // doing that, of course, but there's no reason we shouldn't)
    byte* x = (byte*)ptr;

    for (int n = 0; n < 4; n++) {
        for (int m = 0; m < (size - 16 * 0x1000); m += 4) {
            sys_checksum += *(int*)&x[m];
            sys_checksum += *(int*)&x[m + 16 * 0x1000];
        }
    }
}


/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES  10
FILE* sys_handles[MAX_HANDLES];

int  findhandle() {
    for (int i = 1; i < MAX_HANDLES; i++)
        if (!sys_handles[i])
            return i;
    Sys_Error("out of handles");
    return -1;
}

/*
================
filelength
================
*/
int filelength(FILE* f) {
    int t = VID_ForceUnlockedAndReturnState();
    int pos = ftell(f);
    fseek(f, 0, SEEK_END);
    int end = ftell(f);
    fseek(f, pos, SEEK_SET);

    VID_ForceLockState(t);

    return end;
}

int Sys_FileOpenRead(cStringRO path, int* hndl) {
    int t = VID_ForceUnlockedAndReturnState();
    int i = findhandle();
    FILE* f = fopen(path, "rb");
    int retval;

    if (!f) {
        *hndl = -1;
        retval = -1;
    }
    else {
        sys_handles[i] = f;
        *hndl = i;
        retval = filelength(f);
    }
    VID_ForceLockState(t);

    return retval;
}

int Sys_FileOpenWrite(cStringRO path) {
    int t = VID_ForceUnlockedAndReturnState();
    int i = findhandle();
    FILE* f = fopen(path, "wb");
    if (!f)
        Sys_Error("Error opening %s: %s", path, strerror(errno));
    sys_handles[i] = f;

    VID_ForceLockState(t);
    return i;
}

void Sys_FileClose(int handle) {
    int t = VID_ForceUnlockedAndReturnState();
    fclose(sys_handles[handle]);
    sys_handles[handle] = NULL;
    VID_ForceLockState(t);
}

void Sys_FileSeek(int handle, int position) {
    int t = VID_ForceUnlockedAndReturnState();
    fseek(sys_handles[handle], position, SEEK_SET);
    VID_ForceLockState(t);
}

int Sys_FileRead(int handle, TypeLess_ptr dest, size_t count) {
    int t = VID_ForceUnlockedAndReturnState();
    int x = fread(dest, 1, count, sys_handles[handle]);
    VID_ForceLockState(t);
    return x;
}

int Sys_FileWrite(int handle, TypeLess_ptr data, size_t count) {
    int t = VID_ForceUnlockedAndReturnState();
    int x = fwrite(data, 1, count, sys_handles[handle]);
    VID_ForceLockState(t);
    return x;
}

int Sys_FileTime(cStringRO path) {
    int t = VID_ForceUnlockedAndReturnState();

    FILE* f = fopen(path, "rb");
    int retval;
    if (f) {
        fclose(f);
        retval = 1;
    }
    else {
        retval = -1;
    }

    VID_ForceLockState(t);
    return retval;
}

#if 0
void Sys_mkdir(cStringRO path) {
    _mkdir(path);
}
#else
void Sys_mkdir(cStringRO path) {
    /* WinAPI создаёт папку, если её нет; если есть — вернёт FALSE и
       GetLastError()==ERROR_ALREADY_EXISTS, что нам ок. */
    CreateDirectoryA(path, NULL);
}
#endif


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable(uintptr_t startaddr, size_t length) {
    DWORD  flOldProtect;
    if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
        Sys_Error("Protection change failed\n");
}


#ifndef _M_IX86
void Sys_SetFPCW() {}
void Sys_PushFPCW_SetHigh() {}
void Sys_PopFPCW() {}
void MaskExceptions() {}
#endif

/*
================
Sys_Init
================
*/
void Sys_Init() {
    LARGE_INTEGER PerformanceFreq;
    OSVERSIONINFO vinfo;

    MaskExceptions();
    Sys_SetFPCW();

    if (!QueryPerformanceFrequency(&PerformanceFreq))
        Sys_Error("No hardware timer available");

    // get 32 out of the 64 time bits such that we have around
    // 1 microsecond resolution
    uint32_t lowpart = (uint32_t)PerformanceFreq.LowPart;
    uint32_t highpart = (uint32_t)PerformanceFreq.HighPart;
    _lowshift = 0;

    while (highpart || (lowpart > 2000000.0)) {
        _lowshift++;
        lowpart >>= 1;
        lowpart |= (highpart & 1) << 31;
        highpart >>= 1;
    }

    _pfreq = 1.0 / (double)lowpart;

    Sys_InitFloatTime();

    vinfo.dwOSVersionInfoSize = sizeof(vinfo);

    if (!GetVersionEx(&vinfo))
        Sys_Error("Couldn't get OS info");

    if ((vinfo.dwMajorVersion < 4) ||
        (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)) {
        Sys_Error("WinQuake requires at least Win95 or NT 4.0");
    }

    if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        WinNT = true;
    else
        WinNT = false;
}


void Sys_Error(cStringRO error, ...) {
    char text2[1024];
    cStringRO text3 = "Press Enter to exit\n";
    cStringRO text4 = "***********************************\n";
    cStringRO text5 = "\n";
    DWORD  dummy;
    LegacyTimeStamp_t  starttime;
    static int in_sys_error0 = 0;
    static int in_sys_error1 = 0;
    static int in_sys_error2 = 0;
    static int in_sys_error3 = 0;

    if (!in_sys_error3) {
        in_sys_error3 = 1;
        VID_ForceUnlockedAndReturnState();
    }

    va_list argptr;     va_start(argptr, error);
    char text[1024];    vsnprintf(text, sizeof(text), error, argptr);
    va_end(argptr);

    if (isDedicated) {
        va_start(argptr, error);
        vsnprintf(text, sizeof(text), error, argptr);
        va_end(argptr);

        snprintf(text2, sizeof(text2), "ERROR: %s\n", text);
        WriteFile(houtput, text5, strlen(text5), &dummy, NULL);
        WriteFile(houtput, text4, strlen(text4), &dummy, NULL);
        WriteFile(houtput, text2, strlen(text2), &dummy, NULL);
        WriteFile(houtput, text3, strlen(text3), &dummy, NULL);
        WriteFile(houtput, text4, strlen(text4), &dummy, NULL);


        starttime = Sys_FloatTime();
        _ScReturnOnEnter = true; // so Enter will get us out of here

        while (!Sys_ConsoleInput() &&
            ((Sys_FloatTime() - starttime) < CONSOLE_ERROR_TIMEOUT)) {
        }
    }
    else {
        // switch to windowed so the message box is visible, unless we already
        // tried that and failed
        if (!in_sys_error0) {
            in_sys_error0 = 1;
            VID_SetDefaultMode();
            MessageBox(NULL, text, "Quake Error",
                MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
        }
        else {
            MessageBox(NULL, text, "Double Quake Error",
                MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
        }
    }

    if (!in_sys_error1) {
        in_sys_error1 = 1;
        Host_Shutdown();
    }

    // shut down QHOST hooks if necessary
    if (!in_sys_error2) {
        in_sys_error2 = 1;
        DeinitConProc();
    }

    exit(1);
}

void Sys_Printf(cStringRO fmt, ...) {
    DWORD   dummy;

    if (isDedicated) {
        va_list argptr;     va_start(argptr, fmt);
        char text[1024];    vsnprintf(text, sizeof(text), fmt, argptr);
        va_end(argptr);

        WriteFile(houtput, text, strlen(text), &dummy, NULL);
    }
}

void Sys_Quit() {
    VID_ForceUnlockedAndReturnState();

    Host_Shutdown();

    if (_tEvent)
        CloseHandle(_tEvent);

    if (isDedicated)
        FreeConsole();

    // shut down QHOST hooks if necessary
    DeinitConProc();

    exit(0);
}


/*
================
Sys_FloatTime
================
*/
LegacyTimeStamp_t Sys_FloatTime() {
    Sys_PushFPCW_SetHigh();

    LARGE_INTEGER   PerformanceCount;
    QueryPerformanceCounter(&PerformanceCount);

    uint32_t temp =
        ((uint32_t)PerformanceCount.LowPart >> _lowshift) |
        ((uint32_t)PerformanceCount.HighPart << (32 - _lowshift));

    static bool     first = TRUE;
    static uint32_t oldtime;
    if (first) {
        oldtime = temp;
        first = 0;
    }
    else {
        // check for turnover or backward time
        if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000)) {
            oldtime = temp; // so we can't get stuck
        }
        else {
            uint32_t t2 = temp - oldtime;
            LegacyTimeStamp_t time = (LegacyTimeStamp_t)t2 * _pfreq;
            oldtime = temp;

            _curtime += time;

            static int  sametimecount;
            if (_curtime == _lastcurtime) {
                sametimecount++;

                if (sametimecount > 100000) {
                    _curtime += 1.0;
                    sametimecount = 0;
                }
            }
            else {
                sametimecount = 0;
            }

            _lastcurtime = _curtime;
        }
    }

    Sys_PopFPCW();

    return _curtime;
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime() {
    Sys_FloatTime();

    int j = COM_CheckParm("-starttime");

    if (j)  _curtime = (LegacyTimeStamp_t)(Q_atof(com.argv[j + 1]));
    else    _curtime = 0.0;


    _lastcurtime = _curtime;
}


cString Sys_ConsoleInput() {
    static char text[256];
    static int  len;
    INPUT_RECORD recs[1024];
    DWORD   numevents, numread, dummy;

    if (!isDedicated)
        return NULL;


    for (;; ) {
        if (!GetNumberOfConsoleInputEvents(hinput, &numevents))
            Sys_Error("Error getting # of console events");

        if (numevents <= 0)
            break;

        if (!ReadConsoleInput(hinput, recs, 1, &numread))
            Sys_Error("Error reading console input");

        if (numread != 1)
            Sys_Error("Couldn't read console input");

        if (recs[0].EventType == KEY_EVENT) {
            if (!recs[0].Event.KeyEvent.bKeyDown) {
                int ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

                switch (ch) {
                case '\r':
                    WriteFile(houtput, "\r\n", 2, &dummy, NULL);

                    if (len) {
                        text[len] = 0;
                        len = 0;
                        return text;
                    }
                    else if (_ScReturnOnEnter) {
                        // special case to allow exiting from the error handler on Enter
                        text[0] = '\r';
                        len = 0;
                        return text;
                    }

                    break;

                case '\b':
                    WriteFile(houtput, "\b \b", 3, &dummy, NULL);
                    if (len) {
                        len--;
                    }
                    break;

                default:
                    if (ch >= ' ') {
                        WriteFile(houtput, &ch, 1, &dummy, NULL);
                        text[len] = ch;
                        len = (len + 1) & 0xff;
                    }

                    break;

                }
            }
        }
    }

    return NULL;
}

void Sys_Sleep() {
    Sleep(1);
}


void Sys_SendKeyEvents() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        // we always update if there are any event, even if we're paused
        scr.skipupdate = FALSE;

        if (!GetMessage(&msg, NULL, 0, 0))
            Sys_Quit();

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Sys_HighFPPrecision() {}
void Sys_LowFPPrecision() {}

/*
==============================================================================

WINDOWS CRAP

==============================================================================
*/


/*
==================
WinMain
==================
*/
void SleepUntilInput(int time) {
    MsgWaitForMultipleObjects(1, &_tEvent, FALSE, time, QS_ALLINPUT);
}


/*
==================
WinMain
==================
*/
HINSTANCE global_hInstance;
int   global_nCmdShow;
cString argv[MAX_NUM_ARGVS];
static cString _empty_string = "";
HWND  hwnd_dialog;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    QuakeParms_t parms;
    MEMORYSTATUS lpBuffer;
    static char cwd[1024];

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

    global_hInstance = hInstance;
    global_nCmdShow = nCmdShow;

    lpBuffer.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&lpBuffer);

    if (!GetCurrentDirectory(sizeof(cwd), cwd))
        Sys_Error("Couldn't determine current directory");

    if (cwd[Q_strlen(cwd) - 1] == '/')
        cwd[Q_strlen(cwd) - 1] = 0;

    parms.baseDir = cwd;
    parms.cacheDir = NULL;

    parms.argc = 1;
    argv[0] = _empty_string;

    while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS)) {
        while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
            lpCmdLine++;

        if (*lpCmdLine) {
            argv[parms.argc] = lpCmdLine;
            parms.argc++;

            while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
                lpCmdLine++;

            if (*lpCmdLine) {
                *lpCmdLine = 0;
                lpCmdLine++;
            }

        }
    }

    parms.argv = argv;

    COM_InitArgv(parms.argc, parms.argv);

    parms.argc = com.argc;
    parms.argv = com.argv;

    isDedicated = (COM_CheckParm("-dedicated") != 0);

    if (!isDedicated) {
        hwnd_dialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

        if (hwnd_dialog) {
            RECT    rect;
            if ((GetWindowRect(hwnd_dialog, &rect)) &&
                (rect.left > (rect.top * 2))
                ) {
                SetWindowPos(
                    hwnd_dialog, 0,
                    (rect.left / 2) - ((rect.right - rect.left) / 2),
                    rect.top, 0, 0,
                    SWP_NOZORDER | SWP_NOSIZE);
            }

            ShowWindow(hwnd_dialog, SW_SHOWDEFAULT);
            UpdateWindow(hwnd_dialog);
            SetForegroundWindow(hwnd_dialog);
        }
    }

    // take the greater of all the available memory or half the total memory,
    // but at least 8 Mb and no more than 16 Mb, unless they explicitly
    // request otherwise
    parms.memsize = lpBuffer.dwAvailPhys;

    if (parms.memsize < MINIMUM_WIN_MEMORY)
        parms.memsize = MINIMUM_WIN_MEMORY;

    if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
        parms.memsize = lpBuffer.dwTotalPhys >> 1;

    if (parms.memsize > MAXIMUM_WIN_MEMORY)
        parms.memsize = MAXIMUM_WIN_MEMORY;

    if (COM_CheckParm("-heapsize")) {
        int param;
        param = COM_CheckParm("-heapsize") + 1;

        if (param < com.argc)
            parms.memsize = Q_atoi(com.argv[param]) * 1024;
    }

    parms.membase = malloc(parms.memsize);

    if (!parms.membase)
        Sys_Error("Not enough memory free; check disk space\n");

    Sys_PageIn(parms.membase, parms.memsize);

    _tEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!_tEvent)
        Sys_Error("Couldn't create event");

    if (isDedicated) {
        if (!AllocConsole()) {
            Sys_Error("Couldn't create dedicated server console");
        }

        hinput = GetStdHandle(STD_INPUT_HANDLE);
        houtput = GetStdHandle(STD_OUTPUT_HANDLE);

        // give QHOST a chance to hook into the console
        int param;
        if ((param = COM_CheckParm("-HFILE")) > 0) {
            if (param < com.argc)
                _hFile = (HANDLE)Q_atoi(com.argv[param + 1]);
        }

        if ((param = COM_CheckParm("-HPARENT")) > 0) {
            if (param < com.argc)
                _hEventParent = (HANDLE)Q_atoi(com.argv[param + 1]);
        }

        if ((param = COM_CheckParm("-HCHILD")) > 0) {
            if (param < com.argc)
                _hEventChild = (HANDLE)Q_atoi(com.argv[param + 1]);
        }

        InitConProc(_hFile, _hEventParent, _hEventChild);
    }

    Sys_Init();

    // because sound is off until we become active
    S_BlockSound();

    Sys_Printf("Host_Init\n");
    Host_Init(&parms);

    LegacyTimeStamp_t oldtime = Sys_FloatTime();
    /* main window message loop */
    while (1) {
        LegacyTimeStamp_t  time, newtime;
        if (isDedicated) {
            newtime = Sys_FloatTime();
            time = newtime - oldtime;

            while (time < sys_ticrate.value) {
                Sys_Sleep();
                newtime = Sys_FloatTime();
                time = newtime - oldtime;
            }
        }
        else {
            // yield the CPU for a little while when paused, minimized, or not the focus
            if ((cl.paused && (!ActiveApp && !DDActive)) || Minimized || block_drawing) {
                SleepUntilInput(PAUSE_SLEEP);
                scr.skipupdate = TRUE;  // no point in bothering to draw
            }
            else if (!ActiveApp && !DDActive) {
                SleepUntilInput(NOT_FOCUS_SLEEP);
            }

            newtime = Sys_FloatTime();
            time = newtime - oldtime;
        }

        Host_Frame(time);
        oldtime = newtime;
    }

    /* return success of application */
    return TRUE;
}

