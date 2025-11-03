#pragma once
#include "types.h"

// cmd.h -- Command buffer and command execution

//===========================================================================

/*
    Any number of commands can be added in a frame, from several different sources.
    Most commands come from either keybindings or console line input, but remote
    servers can also send across commands and entire text files can be execed.

    The + command line options are also added to the command buffer.

    The game starts with a Cbuf_AddText("exec quake.rc\n"); Cbuf_Execute();
*/

#ifdef __cplusplus
extern "C" {
#endif
    /*
        =============================================================================

                                COMMAND BUFFER

        =============================================================================
    */
    /*
        ============
        Cbuf_Init
        ============
    */
    // allocates an initial text buffer that will grow as needed
    void Cbuf_Init();


    /*
        ============
        Cbuf_AddText

        Adds command text at the end of the buffer
        ============
    */
    // as new commands are generated from the console or keybindings,
    // the text is added to the end of the command buffer.
    void Cbuf_AddText(cStringRO text);

    /*
        ============
        Cbuf_InsertText

        Adds command text immediately after the current command
        Adds a \n to the text
        FIXME: actually change the command buffer to do less copying
        ============
    */
    // when a command wants to issue other commands immediately, the text is
    // inserted at the beginning of the buffer, before any remaining unexecuted
    // commands.
    void Cbuf_InsertText(cStringRO text);

    /*
        ============
        Cbuf_Execute
        ============
    */
    // Pulls off \n terminated lines of text from the command buffer and sends
    // them through Cmd_ExecuteString.  Stops when the buffer is empty.
    // Normally called once per frame, but may be explicitly invoked.
    // Do not call inside a command function!
    void Cbuf_Execute();


    /*
        ============
        Cmd_Wait_f

        Causes execution of the remainder of the command buffer to be delayed until
        next frame.  This allows commands like:
        bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
        ============
    */
    void Cmd_Wait_f();
    //=====================================================
#ifdef __cplusplus
}
#endif