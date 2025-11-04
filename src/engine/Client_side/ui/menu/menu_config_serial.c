#include "menu.h"
#include "menu_prv.h"
#include "menu_net.h"
#include "net.h"
#include "cbuf.h"
#include <string.h>

//=============================================================================

/* SERIAL CONFIG MENU */

static int serialConfigCursor;
#define NUM_SERIALCONFIG_CMDS (6)
static int _serialConfigCursorTable[NUM_SERIALCONFIG_CMDS] = { 48, 64, 80, 96, 112, 132 };

#define NUM_BAUDRATES (6)
static int _serialConfigBaudRate[NUM_BAUDRATES] = { 9600, 14400, 19200, 28800, 38400, 57600 };

#define ISA_CONFIGS (4)
static int _ISA_uarts[ISA_CONFIGS] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
static int _ISA_IRQs[ISA_CONFIGS] = { 4, 3, 4, 3 };

int  serialConfig_comport;
int32_t  serialConfig_irq;
int  serialConfig_baud;
char serialConfig_phone[16];

void M_Menu_SerialConfig_f() {

    key.dest = key_menu;
    m_state = m_serialconfig;
    m_entersound = true;
    serialConfigCursor = (is_JoinGame() && SerialConfig) ? 4 : 5;

    int32_t port;
    int32_t baudrate;
    bool useModem;
    (*GetComPortConfig) (0, &port, &serialConfig_irq, &baudrate, &useModem);

    // map uart's port to COMx
    {
        int n = 0;
        for (; n < ISA_CONFIGS; n++)
            if (_ISA_uarts[n] == port)
                break;
        if (n == ISA_CONFIGS) {
            n = 0;
            serialConfig_irq = 4;
        }
        serialConfig_comport = n + 1;
    }
    // map baudrate to index
    {
        int n = 0;
        for (; n < NUM_BAUDRATES; n++)
            if (_serialConfigBaudRate[n] == baudrate)
                break;
        if (n == NUM_BAUDRATES)
            n = 5;
        serialConfig_baud = n;
    }
    m_return_onerror = false;
    m_return_reason[0] = 0;
}


void M_SerialConfig_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));

    int basex = M_DrawPicHC(4, Draw_CachePic("gfx/p_multi.lmp"));

    cString startJoin = (is_CreateGame()) ? "New Game" : "Join Game";
    cString directModem = (SerialConfig) ? "Modem" : "Direct Connect";
    M_Print(basex, 32, va("%s - %s", startJoin, directModem));
    basex += 8;

    M_Print(basex, _serialConfigCursorTable[0], "Port");
    M_DrawTextBox(160, 40, 4, 1); {
        M_Print(168, _serialConfigCursorTable[0], va("COM%u", serialConfig_comport));
    }

    M_Print(basex, _serialConfigCursorTable[1], "IRQ");
    M_DrawTextBox(160, _serialConfigCursorTable[1] - 8, 1, 1); {
        M_Print(168, _serialConfigCursorTable[1], va("%u", serialConfig_irq));
    }
    M_Print(basex, _serialConfigCursorTable[2], "Baud");
    M_DrawTextBox(160, _serialConfigCursorTable[2] - 8, 5, 1); {
        M_Print(168, _serialConfigCursorTable[2], va("%u", _serialConfigBaudRate[serialConfig_baud]));
    }

    if (SerialConfig) {
        M_Print(basex, _serialConfigCursorTable[3], "Modem Setup...");
        if (is_JoinGame()) {
            M_Print(basex, _serialConfigCursorTable[4], "Phone number");
            M_DrawTextBox(160, _serialConfigCursorTable[4] - 8, 16, 1); {
                M_Print(168, _serialConfigCursorTable[4], serialConfig_phone);
            }
        }
    }

    if (is_JoinGame()) {
        M_DrawTextBox(basex, _serialConfigCursorTable[5] - 8, 7, 1); {
            M_Print(basex + 8, _serialConfigCursorTable[5], "Connect");
        }
    }
    else {
        M_DrawTextBox(basex, _serialConfigCursorTable[5] - 8, 2, 1); {
            M_Print(basex + 8, _serialConfigCursorTable[5], "OK");
        }
    }

    M_DrawCharacter(basex - 8, _serialConfigCursorTable[serialConfigCursor], curSymb());

    if (serialConfigCursor == 4)
        M_DrawCharacter(168 + 8 * strlen(serialConfig_phone), _serialConfigCursorTable[serialConfigCursor], inpSymb());

    if (*m_return_reason)
        M_PrintWhite(basex, 148, m_return_reason);
}


void M_SerialConfig_Key(keycode_t Key) {
    switch (Key) {
    case K_ESCAPE:  M_Menu_Net_f(); break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        serialConfigCursor--;
        if (serialConfigCursor < 0)
            serialConfigCursor = NUM_SERIALCONFIG_CMDS - 1;
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        serialConfigCursor++;
        if (serialConfigCursor >= NUM_SERIALCONFIG_CMDS)
            serialConfigCursor = 0;
        break;

    case K_LEFTARROW:
        if (serialConfigCursor > 2)
            break;
        S_LocalSound("misc/menu3.wav");

        if (serialConfigCursor == 0) {
            serialConfig_comport--;
            if (serialConfig_comport == 0)
                serialConfig_comport = 4;
            serialConfig_irq = _ISA_IRQs[serialConfig_comport - 1];
        }

        if (serialConfigCursor == 1) {
            serialConfig_irq--;
            if (serialConfig_irq == 6)
                serialConfig_irq = 5;
            if (serialConfig_irq == 1)
                serialConfig_irq = 7;
        }

        if (serialConfigCursor == 2) {
            serialConfig_baud--;
            if (serialConfig_baud < 0)
                serialConfig_baud = 5;
        }

        break;

    case K_RIGHTARROW:
        if (serialConfigCursor > 2)
            break;
    forward:
        S_LocalSound("misc/menu3.wav");

        if (serialConfigCursor == 0) {
            serialConfig_comport++;
            if (serialConfig_comport > 4)
                serialConfig_comport = 1;
            serialConfig_irq = _ISA_IRQs[serialConfig_comport - 1];
        }

        if (serialConfigCursor == 1) {
            serialConfig_irq++;
            if (serialConfig_irq == 6)
                serialConfig_irq = 7;
            if (serialConfig_irq == 8)
                serialConfig_irq = 2;
        }

        if (serialConfigCursor == 2) {
            serialConfig_baud++;
            if (serialConfig_baud > 5)
                serialConfig_baud = 0;
        }

        break;

    case K_ENTER:
        if (serialConfigCursor < 3)
            goto forward;

        m_entersound = true;

        if (serialConfigCursor == 3) {
            (*SetComPortConfig) (0, _ISA_uarts[serialConfig_comport - 1], serialConfig_irq, _serialConfigBaudRate[serialConfig_baud], SerialConfig);

            M_Menu_ModemConfig_f();
            break;
        }

        if (serialConfigCursor == 4) {
            serialConfigCursor = 5;
            break;
        }

        // serialConfigCursor == 5 (OK/CONNECT)
        (*SetComPortConfig) (0, _ISA_uarts[serialConfig_comport - 1], serialConfig_irq, _serialConfigBaudRate[serialConfig_baud], SerialConfig);

        M_ConfigureNetSubsystem();

        if (is_CreateGame()) {
            M_Menu_GameOptions_f();
            break;
        }

        m_return_state = m_state;
        m_return_onerror = true;
        key.dest = key_game;
        m_state = m_none;

        if (SerialConfig)   Cbuf_AddText(va("connect \"%s\"\n", serialConfig_phone));
        else                Cbuf_AddText("connect\n");
        break;

    case K_BACKSPACE:
        if (serialConfigCursor == 4) {
            if (strlen(serialConfig_phone))
                serialConfig_phone[strlen(serialConfig_phone) - 1] = 0;
        }
        break;

    default:
        if ((Key < 32) ||
            (Key > 127))
            break;
        if (serialConfigCursor == 4) {
            int l = strlen(serialConfig_phone);
            if (l < 15) {
                serialConfig_phone[l + 1] = 0;
                serialConfig_phone[l] = Key;
            }
        }
    }

    if ((DirectConfig) &&
        ((serialConfigCursor == 3) ||
            (serialConfigCursor == 4))
        ) {
        serialConfigCursor =
            (Key == K_UPARROW) ? 2 : 5;
    }

    if (SerialConfig &&
        is_CreateGame() &&
        (serialConfigCursor == 4)
        ) {
        serialConfigCursor =
            (Key == K_UPARROW) ? 3 : 5;
    }
}
