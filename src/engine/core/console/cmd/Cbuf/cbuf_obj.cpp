#include "cbuf.hpp"
#include "cbuf.h"
#include "cmd.h"

#include "sizebuf.h"
#include <string.h>
#include "q_tools.h"
#include "zone.h"
#include "console.h"

#define CMD_BUSS_SIZE  (0x2000)  /* 8Kb*/



//=============================================================================

/*
    ============
    Cmd_Wait_f

    Causes execution of the remainder of the command buffer to be delayed until
    next frame.  This allows commands like:
    bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
    ============
*/
void Cbuf::setWait() {
    _cmdWait = true;
}
/*
    =============================================================================

                            COMMAND BUFFER

    =============================================================================
*/


/*
    ============
    Cbuf::Init
    ============
*/
void Cbuf::Init() { SZ_Alloc(&_cmdText, CMD_BUSS_SIZE); } // space for commands and script files



/*
    ============
    Cbuf::AddText

    Adds command text at the end of the buffer
    ============
*/
void Cbuf::AddText(cStringRO text) {
    if ((_cmdText.cursize + Q_strlen(text)) >= _cmdText.maxsize) {
        Con_Printf("Cbuf::AddText: overflow\n");
        return;
    }

    SZ_Write(&_cmdText, (TypeLess_ptr)text, Q_strlen(text));
}


/*
    ============
    Cbuf::InsertText

    Adds command text immediately after the current command
    Adds a \n to the text
    FIXME: actually change the command buffer to do less copying
    ============
*/
void Cbuf::InsertText(cStringRO text) {
    cString   temp;

    // copy off any commands still remaining in the exec buffer
    int32_t templen = _cmdText.cursize;
    if (templen) {
        temp = (cString)Z_Malloc(templen);
        Q_memcpy(temp, _cmdText.data, templen);
        SZ_Clear(&_cmdText);
    }
    else temp = NULL; // shut up compiler


    // add the entire text of the file
    Cbuf::AddText(text);

    // add the copied off data
    if (templen) {
        SZ_Write(&_cmdText, temp, templen);
        Z_Free(temp);
    }
}

/*
    ============
    Cbuf::Execute
    ============
*/
void Cbuf::Execute() {

    while (_cmdText.cursize) {
        // find a \n or ; line break
        cString text = (cString)_cmdText.data;

        uint8_t quotes = 0;
        int i = 0;
        for (; i < _cmdText.cursize; i++) {
            if (text[i] == '"')
                quotes++;
            if (((!(quotes & 1)) &&
                (text[i] == ';')) ||  // don't break if inside a quoted string
                (text[i] == '\n')) {
                break;
            }
        }


        char line[1024];
        memcpy(line, text, i);
        line[i] = 0;

        // delete the text from the command buffer and move remaining commands down
        // this is necessary because commands (exec, alias) can insert data at the
        // beginning of the text buffer

        if (i == _cmdText.cursize) { _cmdText.cursize = 0; }
        else {
            i++;
            _cmdText.cursize -= i;
            Q_memcpy(text, text + i, _cmdText.cursize);
        }

        // execute the command line
        Cmd_ExecuteString(line, src_command);

        // skip out while text still remains in buffer, leaving it
        // for next frame
        if (_cmdWait) {
            _cmdWait = false;
            break;
        }
    }
}


Cbuf cbuf;