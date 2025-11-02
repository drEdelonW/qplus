#include "cbuf.hpp"
#include "cbuf.h"

#define CMD_BUSS_SIZE  (0x2000)  /* 8Kb*/


Cbuf cbuf;
//=============================================================================

/*
    ============
    Cmd_Wait_f

    Causes execution of the remainder of the command buffer to be delayed until
    next frame.  This allows commands like:
    bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
    ============
*/
void Cmd_Wait_f() {
    cbuf.setWait();
}

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
void Cbuf_Init() {  // space for commands and script files
    cbuf.Init();
}



/*
    ============
    Cbuf_AddText

    Adds command text at the end of the buffer
    ============
*/
void Cbuf_AddText(cStringRO text) {
    cbuf.AddText(text);
}


/*
    ============
    Cbuf_InsertText

    Adds command text immediately after the current command
    Adds a \n to the text
    FIXME: actually change the command buffer to do less copying
    ============
*/
void Cbuf_InsertText(cStringRO text) {
    cbuf.InsertText(text);
}

/*
    ============
    Cbuf_Execute
    ============
*/
void Cbuf_Execute() {
    cbuf.Execute();
}
