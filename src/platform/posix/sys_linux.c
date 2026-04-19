#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/mman.h>


#include "sys.h"
#include "host.h"
#include "common.h"
#include "versions.h"
#include "client.h"
#include "cvar_q1.h"
#include "q_tools.h"
#include "net_vcr.h"
#include "gamedefs.h"



int nostdout = 0;

static cString _baseDir = ".";
// cString cachedir = "/tmp";

CVAR(sys_linerefresh, "0");// set for entity display

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val) {}

#if 0
void Sys_Printf(cStringRO fmt, ...) {
    va_list argptr;     va_start(argptr, fmt);
    char text[1024];    vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "%s", text);

    Con_Print(text);
}
#elif 0
void Sys_Printf(cStringRO fmt, ...) {
    cString t_p;
    int l, r;

    if (nostdout)
        return;

    va_list argptr; va_start(argptr, fmt);
    char text[1024];    vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    l = strlen(text);
    t_p = text;

    // make sure everything goes through, even though we are non-blocking
    while (l) {
        r = write(1, text, l);
        if (r != l)
            sleep(0);
        if (r > 0) {
            t_p += r;
            l -= r;
        }
    }

}
#else

void Sys_Printf(cStringRO fmt, ...) {
    va_list argptr;     va_start(argptr, fmt);
    char text[1024];    vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    if (strlen(text) > sizeof(text))
        Sys_Error("memory overwrite in Sys_Printf");

    if (nostdout)
        return;

    for (uint8_p p = (uint8_p)text; *p; p++) {
        *p &= 0x7f;
        if (((*p > 128) ||  // ASCII [DEL] last printable
            (*p < 32)) &&   // ASCII [SP] first printable
            (*p != 10) &&   // ASCII [LF]
            (*p != 13) &&   // ASCII [CR]
            (*p != 9)       // ASCII [HT]
            ) {
            printf("[%02x]", *p);
        }
        else putc(*p, stdout);

    }
}
#endif


void Sys_Quit() {
    Host_Shutdown();
    fcntl(0, F_SETFL, (fcntl(0, F_GETFL, 0) & ~FNDELAY));

    GM_Quit();

    fflush(stdout);
    exit(0);
}

void Sys_Init() {
#if id386
    Sys_SetFPCW();
#endif
}

void Sys_Error(cStringRO error, ...) {
    // change stdin to non blocking
    fcntl(0, F_SETFL, (fcntl(0, F_GETFL, 0) & ~FNDELAY));

    va_list argptr;     va_start(argptr, error);
    char string[1024];  vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);
    fprintf(stderr, "Error: %s\n", string);

    Host_Shutdown();
    exit(1);
}

void Sys_Warn(cStringRO warning, ...) {
    va_list argptr;     va_start(argptr, warning);
    char string[1024];  vsnprintf(string, sizeof(string), warning, argptr);
    va_end(argptr);
    fprintf(stderr, "Warning: %s", string);
}

double Sys_FloatTime() {
    struct timeval tp;
    struct timezone tzp;
    static int _secBase;

    gettimeofday(&tp, &tzp);

    if (!_secBase) {
        _secBase = tp.tv_sec;
        return tp.tv_usec / 1000000.0;
    }

    return (tp.tv_sec - _secBase) + tp.tv_usec / 1000000.0;
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int _oktogo;

void alarm_handler(int x) {
    _oktogo = 1;
}

void Sys_LineRefresh() {}

// void floating_point_exception_handler(int whatever) {
//     //	Sys_Warn("floating point exception\n");
//     signal(SIGFPE, floating_point_exception_handler);
// }



#if !id386
void Sys_HighFPPrecision() {}

void Sys_LowFPPrecision() {}
#endif

int main(int c, cStringArray v) {
    QuakeParms_t parms;

    //	static char cwd[1024];

    //	signal(SIGFPE, floating_point_exception_handler);
    signal(SIGFPE, SIG_IGN);

    memset(&parms, 0, sizeof(parms));

    COM_InitArgv(c, v);
    parms.argc = com.argc;
    parms.argv = com.argv;

    parms.memsize =
#ifdef GLQUAKE
        16 *
#else
        8 *
#endif
        1024 * 1024;

    int memParam = COM_CheckParm("-mem");
    if (memParam)
        parms.memsize = (int)(Q_atof(com.argv[memParam + 1]) * 1024 * 1024);
    parms.membase = malloc(parms.memsize);

    parms.baseDir = _baseDir;
    // caching is disabled by default, use -cachedir to enable
    //	parms.cacheDir = cachedir;

    fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

    Host_Init(&parms);
    Sys_Init();

    if (COM_CheckParm("-nostdout")) nostdout = 1;
    else {
        fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);
        printf("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
    }

    double oldtime = Sys_FloatTime() - 0.1;
    while (1) {
        double newtime = Sys_FloatTime();
        double time = newtime - oldtime;    // find time spent rendering last frame

        if (Host_IsDedicated()) {     // play vcrfiles at max speed
            if ((time < sys_ticrate.value) &&
                ((vcrFile == -1) ||
                    (recording))
                ) {
                usleep(1);
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > (sys_ticrate.value * 2))     oldtime = newtime;
        else                                    oldtime += time;


        Host_Frame(time);   // all compute are here

        // graphic debugging aids
        if (sys_linerefresh.value) {
            Sys_LineRefresh();
        }
    }
}


/*
    ================
    Sys_MakeCodeWriteable
    ================
*/
#if 0
void Sys_MakeCodeWriteable(uint32_t startaddr, uint32_t length) {
    int psize = getpagesize();

    uint32_t addr = (startaddr & (~(psize - 1))) - psize;

    //	fprintf(stderr, "writable code %lx(%lx)-%lx, length=%lx\n", startaddr,
    //			addr, startaddr+length, length);

    int r = mprotect((cString)addr, (length + startaddr - addr + psize), 7);

    if (r < 0) {
        Sys_Error("Protection change failed\n");
    }

}
#else
void Sys_MakeCodeWriteable(uintptr_t startaddr, size_t length) {
    long psize = sysconf(_SC_PAGESIZE); // portable page size

    // align start address down to page boundary
    uintptr_t addr = (startaddr & ~(uintptr_t)(psize - 1));

    // round length up to cover the full range
    size_t len = (startaddr + length - addr + psize - 1) & ~(psize - 1);

    if (mprotect((TypeLess_ptr)addr, len, PROT_READ | PROT_WRITE | PROT_EXEC) < 0) {
        Sys_Error("Protection change failed\n");
    }
}
#endif

