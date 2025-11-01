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
// cmd.c -- Quake script command processing module

#include "cmd.h"
#include "cbuf.h"

#include <string.h>
#include "q_tools.h"
#include "console.h"
#include "zone.h"
#include "common.h"
#include "host.h"
#include "client.h"
#include "sys.h"
#include "protocol.h"
#include "cvar.h"
#include "msg.h"


/*
    ==============================================================================

                            SCRIPT COMMANDS

    ==============================================================================
*/

/*
    ===============
    Cmd_StuffCmds_f

    Adds command line parameters as script statements
    Commands lead with a +, and continue until a - or another +
    quake +prog jctest.qp +cmd amlev1
    quake -nosound +cmd amlev1
    ===============
*/
void Cmd_StuffCmds_f() {
    if (Cmd_Argc() != 1) {
        Con_Printf("stuffcmds : execute command line parameters\n");
        return;
    }

    // build the combined string to parse from
    int s = 0;
    for (int i = 1; i < com_argc; i++) {
        if (!com_argv[i])   continue;  // NEXTSTEP nulls out -NXHost

        s += Q_strlen(com_argv[i]) + 1;
    }
    if (!s)     return;

    cString text = Z_Malloc(s + 1);
    text[0] = 0;
    for (int i = 1; i < com_argc; i++) {
        if (!com_argv[i])   continue;   // NEXTSTEP nulls out -NXHost

        Q_strcat(text, com_argv[i]);
        if (i != (com_argc - 1)) {
            Q_strcat(text, " ");
        }
    }

    // pull out the commands
    cString build = Z_Malloc(s + 1);
    build[0] = 0;

    for (int i = 0; i < (s - 1); i++) {
        if (text[i] == '+') {
            i++;
            int j = i;
            for (;
                (
                    (text[j] != '+') &&
                    (text[j] != '-') &&
                    (text[j] != 0));
                j++);

            char c = text[j];
            text[j] = 0;

            Q_strcat(build, text + i);
            Q_strcat(build, "\n");
            text[j] = c;
            i = j - 1;
        }
    }

    if (build[0]) Cbuf_InsertText(build);

    Z_Free(text);
    Z_Free(build);
}


/*
    ===============
    Cmd_Exec_f
    ===============
*/
void Cmd_Exec_f() {
    if (Cmd_Argc() != 2) { Con_Printf("exec <filename> : execute a script file\n"); return; }

    size_t mark = Hunk_LowMark();
    cString f = (cString)COM_LoadHunkFile(Cmd_Argv(1));
    if (!f) { Con_Printf("couldn't exec %s\n", Cmd_Argv(1)); return; }

    Con_Printf("execing %s\n", Cmd_Argv(1));

    Cbuf_InsertText(f);
    Hunk_FreeToLowMark(mark);
}


/*
    ===============
    Cmd_Echo_f

    Just prints the rest of the line to the console
    ===============
*/
void Cmd_Echo_f() {
    for (int i = 1; i < Cmd_Argc(); i++) {
        Con_Printf("%s ", Cmd_Argv(i));
    }
    Con_Printf("\n");
}

/*
    ===============
    Cmd_Alias_f

    Creates a new command that executes a command string (possibly ; seperated)
    ===============
*/
cString CopyString(cString in) {
    cString out = Z_Malloc(strlen(in) + 1);
    strcpy(out, in);
    return out;
}

/*
    =============================================================================

                        COMMAND EXECUTION

    =============================================================================
*/


typedef struct CmdFunction_s CmdFunction_t;
typedef CmdFunction_t* CmdFunction_p;
struct CmdFunction_s {
    CmdFunction_p  next;
    cString         name;
    xcommand_t      function;
};


#define MAX_ARGS  80

static int _cmdArgC;
cString cmd_argv[MAX_ARGS];
static cString _cmdNullString = "";
static cString _cmdArgS = NULL;

cmd_source_t cmd_source;


static CmdFunction_p _cmdFunctions;  // possible commands to execute

/*
    ============
    Cmd_Init
    ============
*/
void Cmd_Init() {
    //
    // register our commands
    //
    Cmd_AddCommand("stuffcmds", Cmd_StuffCmds_f);
    Cmd_AddCommand("exec", Cmd_Exec_f);
    Cmd_AddCommand("echo", Cmd_Echo_f);
    Cmd_AddCommand("alias", Cmd_Alias_f);
    Cmd_AddCommand("cmd", Cmd_ForwardToServer);
    Cmd_AddCommand("wait", Cmd_Wait_f);
}

/*
    ============
    Cmd_Argc
    ============
*/
int Cmd_Argc() { return _cmdArgC; }

/*
    ============
    Cmd_Argv
    ============
*/
cString Cmd_Argv(int arg) {
    return
        ((uint32_t)arg >= _cmdArgC) ?
        _cmdNullString : cmd_argv[arg];
}

/*
    ============
    Cmd_Args
    ============
*/
cString Cmd_Args() { return _cmdArgS; }


/*
    ============
    Cmd_TokenizeString

    Parses the given string into command line tokens.
    ============
*/
void Cmd_TokenizeString(cString text) {
    // clear the args from the last string
    for (int i = 0; i < _cmdArgC; i++) {
        Z_Free(cmd_argv[i]);
    }

    _cmdArgC = 0;
    _cmdArgS = NULL;

    while (1) {
        // skip whitespace up to a /n
        while ((*text) &&
            (*text < ' ') &&
            (*text != '\n')
            ) {
            text++;
        }

        if (*text == '\n') { // a newline seperates commands in the buffer
            text++;
            break;
        }

        if (!*text) { return; }

        if (_cmdArgC == 1) {
            _cmdArgS = text;
        }

        text = COM_Parse(text);
        if (!text) { return; }

        if (_cmdArgC < MAX_ARGS) {
            cmd_argv[_cmdArgC] = Z_Malloc(Q_strlen(com_token) + 1);
            Q_strcpy(cmd_argv[_cmdArgC], com_token);
            _cmdArgC++;
        }
    }

}


/*
    ============
    Cmd_AddCommand
    ============
*/
void Cmd_AddCommand(cString cmd_name, xcommand_t function) {
    if (host_initialized) // because hunk allocation would get stomped
        Sys_Error("Cmd_AddCommand after host_initialized");

    // fail if the command is a variable name
    if (Cvar_VariableString(cmd_name)[0]) {
        Con_Printf("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
        return;
    }

    // fail if the command already exists
    CmdFunction_p cmd;
    for (cmd = _cmdFunctions; cmd; cmd = cmd->next) {
        if (!Q_strcmp(cmd_name, cmd->name)) {
            Con_Printf("Cmd_AddCommand: %s already defined\n", cmd_name);
            return;
        }
    }

    cmd = Hunk_Alloc(sizeof(CmdFunction_t));
    cmd->name = cmd_name;
    cmd->function = function;
    cmd->next = _cmdFunctions;
    _cmdFunctions = cmd;
}

/*
    ============
    Cmd_Exists
    ============
*/
bool Cmd_Exists(cString cmd_name) {
    for (CmdFunction_p cmd = _cmdFunctions; cmd; cmd = cmd->next) {
        if (!Q_strcmp(cmd_name, cmd->name)) { return true; }
    }
    return false;
}



/*
    ============
    Cmd_CompleteCommand
    ============
*/
cString Cmd_CompleteCommand(cString partial) {
    int len = Q_strlen(partial);

    if (!len) return NULL;

    // check functions
    for (CmdFunction_p cmd = _cmdFunctions; cmd; cmd = cmd->next) {
        if (!Q_strncmp(partial, cmd->name, len)) {
            return cmd->name;
        }
    }
    return NULL;
}

/*
    ============
    Cmd_ExecuteString

    A complete command line has been parsed, so try to execute it
    FIXME: lookupnoadd the token to speed search?
    ============
*/
void Cmd_ExecuteString(cString text, cmd_source_t src) {
    cmd_source = src;
    Cmd_TokenizeString(text);

    // execute the command line
    if (!Cmd_Argc()) { return; } // no tokens

    // check functions
    for (CmdFunction_p cmd = _cmdFunctions; cmd; cmd = cmd->next) {
        if (!Q_strcasecmp(cmd_argv[0], cmd->name)) {
            cmd->function();
            return;
        }
    }

    if (checkAlias()) { return; }

#if 0
    // check alias
    for (cmdalias_p aliasIt = _cmdAlias; aliasIt; aliasIt = aliasIt->next) {
        if (!Q_strcasecmp(cmd_argv[0], aliasIt->name)) {
            Cbuf_InsertText(aliasIt->value);
            return;
        }
}
#endif

    // check cvars
    if (!Cvar_Command()) {
        Con_Printf("Unknown command \"%s\"\n", Cmd_Argv(0));
    }

}


/*
    ===================
    Cmd_ForwardToServer

    Sends the entire command line over to the server
    ===================
*/
void Cmd_ForwardToServer() {
    if (cls.state != ca_connected) {
        Con_Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
        return;
    }

    if (cls.demoplayback) { return; }  // not really connected

    MSG_WriteByte(&cls.message, clc_stringcmd);
    if (Q_strcasecmp(Cmd_Argv(0), "cmd") != 0) {
        SZ_Print(&cls.message, Cmd_Argv(0));
        SZ_Print(&cls.message, " ");
    }
    SZ_Print(&cls.message,
        (Cmd_Argc() > 1) ?
        Cmd_Args() : "\n"
    );
}


/*
    ================
    Cmd_CheckParm

    Returns the position (1 to argc-1) in the command's argument list
    where the given parameter appears, or 0 if not present
    ================
*/
int Cmd_CheckParm(cString parm) {
    if (!parm) { Sys_Error("Cmd_CheckParm: NULL"); }

    for (int i = 1; i < Cmd_Argc(); i++) {
        if (!Q_strcasecmp(parm, Cmd_Argv(i))) { return i; }
    }

    return 0;
}
