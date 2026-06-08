#include "cbuf.hpp"
#include "cmd.h"

#include <string.h>
#include "q_tools.h"
#include "z_zone.h"
#include "console.h"

#define CMD_BUSS_SIZE  (0x2000)  /* 8Kb*/

Cbuf cbuf;


//=============================================================================

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
void Cbuf::Init() {
    SZ_Alloc(&_cmdText, CMD_BUSS_SIZE);
} // space for commands and script files



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

#include <stdio.h>
void* Debug_Memcpy(void* dst, const void* src, size_t count) {
    // Приводим к указателям на байты для побайтовой арифметики
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;

    // printf("[MEMCPY START] Src: %p, Dst: %p, Total Bytes: %zu\n", src, dst, count);
    // printf("--------------------------------------------------------------------\n");

    for (size_t index = 0; index < count; index++) {
        // Выводим текущее состояние ДО копирования байта.
        // Это важно: если на текущем индексе случится HardFault, 
        // последний принт в консоли покажет, на каком именно адресе все сломалось.
        // printf("Idx: %4zu | Read Src: %p [0x%02X] -> Write Dst: %p\n", 
        //        index, 
        //        (void*)(s + index), 
        //        *(s + index), 
        //        (void*)(d + index));
        
        // Само копирование
        d[index] = s[index];
    }

    // printf("--------------------------------------------------------------------\n");
    // printf("[MEMCPY SUCCESS] Successfully copied %zu bytes\n", count);
    
    return dst;
}
/*
    ============
    Cbuf::Execute
    ============
*/
void Cbuf::Execute() {
    char line[1024];
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


#if 
        Debug_Memcpy(line, text, i);
#else
        memcpy(line, text, i);
#endif
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

