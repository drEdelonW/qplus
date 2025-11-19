#include "gamedefs.h"
#include "common.h"
#include "console.h"
#include "sys.h"
#include "Pak.h"
#include "endian_tools.h"
#include "cvar_q1.h"



bool standard_quake = true;
bool rogue;
bool hipnotic;

int32_t  Registered = 1;  // only for startup check, then set

// this graphic needs to be in the pak file to use registered features
static uint16_t _pop[128] = {
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

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in QuakeParms_t->baseDir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently.  This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because they are added at the end, they will not override an explicit setting on the original command line.

*/



/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void GM_CheckRegistered() {
    int h;
    COM_OpenFile("gfx/pop.lmp", &h);
    Registered = 0;

    if (h == -1) {
#if WINDED
        Sys_Error("This dedicated server requires a full registered copy of Quake");
#endif
        Con_Printf("Playing shareware version.\n");
        if (contModified)
            Sys_Error("You must have the registered version to use modified games");
        return;
    }

    int16_t  check[128];
    Sys_FileRead(h, check, sizeof(check));
    COM_CloseFile(h);

    for (int i = 0; i < 128; i++)
        if (_pop[i] != (uint16_t)BigShort(check[i]))
            Sys_Error("Corrupted data file.");

    Cvar_Set("cmdline", com.cmdline);
    Cvar_Set("registered", "1");
    Registered = 1;
    Con_Printf("Playing registered version.\n");
}

void GM_GameInit() {
    Cvar_RegisterVariable(&registered);
    Cvar_RegisterVariable(&cmdline);
}

void GM_Quit() {
#if 0
static char end1[] =
    "\x1b[?7h\x1b[40m\x1b[2J\x1b[0;1;41m"
    "\x1b[1;1H                QUAKE: The Doomed Dimension \x1b[33mby \x1b[44mid\x1b[41m Software                      "
    "\x1b[2;1H  ----------------------------------------------------------------------------  "
    "\x1b[3;1H           CALL 1-800-IDGAMES TO ORDER OR FOR TECHNICAL SUPPORT                 "
    "\x1b[4;1H             PRICE: $45.00 (PRICES MAY VARY OUTSIDE THE US.)                    "
    "\x1b[5;1H                                                                                "
    "\x1b[6;1H  \x1b[37mYes! You only have one fourth of this incredible epic. That is because most   "
    "\x1b[7;1H   of you have paid us nothing or at most, very little. You could steal the     "
    "\x1b[8;1H   game from a friend. But we both know you'll be punished by God if you do.    "
    "\x1b[9;1H        \x1b[33mWHY RISK ETERNAL DAMNATION? CALL 1-800-IDGAMES AND BUY NOW!             "
    "\x1b[10;1H             \x1b[37mRemember, we love you almost as much as He does.                   "
    "\x1b[11;1H                                                                                "
    "\x1b[12;1H            \x1b[33mProgramming: \x1b[37mJohn Carmack, Michael Abrash, John Cash                "
    "\x1b[13;1H       \x1b[33mDesign: \x1b[37mJohn Romero, Sandy Petersen, American McGee, Tim Willits         "
    "\x1b[14;1H                     \x1b[33mArt: \x1b[37mAdrian Carmack, Kevin Cloud                           "
    "\x1b[15;1H               \x1b[33mBiz: \x1b[37mJay Wilbur, Mike Wilson, Donna Jackson                      "
    "\x1b[16;1H            \x1b[33mProjects: \x1b[37mShawn Green   \x1b[33mSupport: \x1b[37mBarrett Alexander                  "
    "\x1b[17;1H              \x1b[33mSound Effects: \x1b[37mTrent Reznor and Nine Inch Nails                   "
    "\x1b[18;1H  For other information or details on ordering outside the US, check out the    "
    "\x1b[19;1H     files accompanying QUAKE or our website at http://www.idsoftware.com.      "
    "\x1b[20;1H    \x1b[0;41mQuake is a trademark of Id Software, inc., (c)1996 Id Software, inc.        "
    "\x1b[21;1H     All rights reserved. NIN logo is a registered trademark licensed to        "
    "\x1b[22;1H                 Nothing Interactive, Inc. All rights reserved.                 "
    "\x1b[40m\x1b[23;1H\x1b[0m";
static char end2[] =
    "\x1b[?7h\x1b[40m\x1b[2J\x1b[0;1;41m"
    "\x1b[1;1H        QUAKE \x1b[33mby \x1b[44mid\x1b[41m Software                                                    "
    "\x1b[2;1H -----------------------------------------------------------------------------  "
    "\x1b[3;1H        \x1b[37mWhy did you quit from the registered version of QUAKE? Did the          "
    "\x1b[4;1H        scary monsters frighten you? Or did Mr. Sandman tug at your             "
    "\x1b[5;1H        little lids? No matter! What is important is you love our               "
    "\x1b[6;1H        game, and gave us your money. Congratulations, you are probably         "
    "\x1b[7;1H        not a thief.                                                            "
    "\x1b[8;1H                                                           Thank You.           "
    "\x1b[9;1H        \x1b[33;44mid\x1b[41m Software is:                                                         "
    "\x1b[10;1H        PROGRAMMING: \x1b[37mJohn Carmack, Michael Abrash, John Cash                    "
    "\x1b[11;1H        \x1b[33mDESIGN: \x1b[37mJohn Romero, Sandy Petersen, American McGee, Tim Willits        "
    "\x1b[12;1H        \x1b[33mART: \x1b[37mAdrian Carmack, Kevin Cloud                                        "
    "\x1b[13;1H        \x1b[33mBIZ: \x1b[37mJay Wilbur, Mike Wilson     \x1b[33mPROJECTS MAN: \x1b[37mShawn Green              "
    "\x1b[14;1H        \x1b[33mBIZ ASSIST: \x1b[37mDonna Jackson        \x1b[33mSUPPORT: \x1b[37mBarrett Alexander             "
    "\x1b[15;1H        \x1b[33mSOUND EFFECTS AND MUSIC: \x1b[37mTrent Reznor and Nine Inch Nails               "
    "\x1b[16;1H                                                                                "
    "\x1b[17;1H        If you need help running QUAKE refer to the text files in the           "
    "\x1b[18;1H        QUAKE directory, or our website at http://www.idsoftware.com.           "
    "\x1b[19;1H        If all else fails, call our technical support at 1-800-IDGAMES.         "
    "\x1b[20;1H      \x1b[0;41mQuake is a trademark of Id Software, inc., (c)1996 Id Software, inc.      "
    "\x1b[21;1H        All rights reserved. NIN logo is a registered trademark licensed        "
    "\x1b[22;1H             to Nothing Interactive, Inc. All rights reserved.                  "
    "\x1b[23;1H\x1b[40m\x1b[0m";

    if (registered.value)   printf("%s", end2);
    else                    printf("%s", end1);
#endif
}