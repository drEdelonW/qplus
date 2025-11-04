#pragma once
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

#include "types.h"


/*
    Command execution takes a null terminated string, breaks it into tokens,
    then searches for a command or variable that matches the first token.

    Commands can come from three sources, but the handler functions may choose
    to dissallow the action or forward it to a remote server if the source is
    not apropriate.
*/

typedef void(*xcommand_t)();

typedef enum {
    src_client,  // came in over a net connection as a clc_stringcmd
    // remoteClient will be valid during this state.
    src_command  // from the command buffer
} cmd_source_t;

extern cmd_source_t cmd_source;
extern cString cmd_argv[];

#ifdef __cplusplus
extern "C" {
#endif
    void Cmd_Init();

    // called by the init functions of other parts of the program to
    // register commands and functions to call for them.
    // The cmd_name is referenced later, so it should not be in temp memory
    void Cmd_AddCommand(cStringRO cmd_name, xcommand_t function);

    // used by the cvar code to check for cvar / command name overlap
    bool Cmd_Exists(cString cmd_name);

    // attempts to match a partial command for automatic command line completion
    // returns NULL if nothing fits
    cString Cmd_CompleteCommand(cString partial);

    // The functions that execute commands get their parameters with these
    // functions. Cmd_Argv() will return an empty string, not a NULL
    // if arg > argc, so string operations are allways safe.
    int  Cmd_Argc();
    cString Cmd_Argv(int arg);
    cString Cmd_Args();

    // Returns the position (1 to argc-1) in the command's argument list
    // where the given parameter apears, or 0 if not present
    int Cmd_CheckParm(cString parm);

    // Takes a null terminated string.  Does not need to be /n terminated.
    // breaks the string up into arg tokens.
    void Cmd_TokenizeString(cString text);

    // Parses a single line of text into arguments and tries to execute it.
    // The text can come from the command buffer, a remote client, or stdin.
    void Cmd_ExecuteString(cString text, cmd_source_t src);

    // adds the current command line as a clc_stringcmd to the client message.
    // things like godmode, noclip, etc, are commands directed to the server,
    // so when they are typed in at the console, they will need to be forwarded.
    void Cmd_ForwardToServer();

    // used by command functions to send output to either the graphics console or
    // passed as a print message to the client
    void Cmd_Print(cString text);

    void Cmd_Alias_f();
    bool checkAlias();
    cString CopyString(cString in);

#ifdef __cplusplus
}
#endif