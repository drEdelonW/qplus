#pragma once
#include "types.h"
#include "sizebuf.h"

// cmd.h -- Command buffer and command execution

//===========================================================================

/*
    Any number of commands can be added in a frame, from several different sources.
    Most commands come from either keybindings or console line input, but remote
    servers can also send across commands and entire text files can be execed.

    The + command line options are also added to the command buffer.

    The game starts with a AddText("exec quake.rc\n"); Execute();
*/

class Cbuf {
public:
    void Init();    // allocates an initial text buffer that will grow as needed

    // as new commands are generated from the console or keybindings,
    // the text is added to the end of the command buffer.
    void AddText(cStringRO text);

    // when a command wants to issue other commands immediately, the text is
    // inserted at the beginning of the buffer, before any remaining unexecuted
    // commands.
    void InsertText(cStringRO text);

    // Pulls off \n terminated lines of text from the command buffer and sends
    // them through Cmd_ExecuteString.  Stops when the buffer is empty.
    // Normally called once per frame, but may be explicitly invoked.
    // Do not call inside a command function!
    void Execute();

    void setWait();
private:
    bool        _cmdWait;
    sizebuf_t   _cmdText;

};
// void Cmd_Wait_f();
//=====================================================

extern Cbuf cbuf;