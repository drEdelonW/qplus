#pragma once

#include "menu.h"
#include "keys.h"
#include "draw.h"
// #include "q_tools.h"
// #include "client.h"
// #include "cvar_q1.h"
#include "common.h"
// #include <string.h>
// #include "server.h"
#include "sound.h"
// #include "console.h"
#include "cmd.h"
// #include "host.h"

extern m_state_t m_state;
extern m_state_t m_return_state;
extern bool m_entersound; // play after drawing a frame, so caching
// won't disrupt the sound

extern bool m_recursiveDraw;
extern bool m_return_onerror;
extern char m_return_reason[];

int blink(char sym);
int curAnimFrame();
int curSymb();
int inpSymb();


void M_DrawTextBox(int x, int y, int width, int lines);
void M_DrawCharacter(int cx, int line, int num);

void M_Print(int cx, int cy, cString str);
void M_PrintWhite(int cx, int cy, cString str);
void M_BuildTranslationTable(int top, int bottom);

void M_DrawTransPicTranslate(int x, int y, qPic_p pic);
void M_DrawTransPic(int x, int y, qPic_p pic);
int M_DrawPicHC(int y, qPic_p pic);
void M_DrawPic(int x, int y, qPic_p pic);


void M_DrawSlider(int x, int y, float range);
void M_DrawCheckbox(int x, int y, int on);

void M_ConfigureNetSubsystem();

void M_FindKeysForCommand(cString command, int* twokeys);


void M_Menu_Main_f();
void M_Main_Draw();
void M_Main_Key(keycode_t key);

void M_Menu_SinglePlayer_f();
void M_SinglePlayer_Draw();
void M_SinglePlayer_Key(keycode_t key);

void M_Menu_Load_f();
void M_Load_Draw();
void M_Load_Key(keycode_t key);

void M_Menu_Save_f();
void M_Save_Draw();
void M_Save_Key(keycode_t key);

void M_Menu_MultiPlayer_f();
void M_MultiPlayer_Draw();
void M_MultiPlayer_Key(keycode_t key);

void M_Menu_Setup_f();
void M_Setup_Draw();
void M_Setup_Key(keycode_t key);

void M_Menu_Net_f();
void M_Net_Draw();
void M_Net_Key(keycode_t key);

void M_Menu_Options_f();
void M_Options_Draw();
void M_Options_Key(keycode_t key);

void M_Menu_Keys_f();
void M_Keys_Draw();
void M_Keys_Key(keycode_t key);

void M_Menu_Video_f();
void M_Video_Draw();
void M_Video_Key(keycode_t key);

void M_Menu_Help_f();
void M_Help_Draw();
void M_Help_Key(keycode_t key);

void M_Menu_Quit_f();
void M_Quit_Draw();
void M_Quit_Key(keycode_t key);

void M_Menu_SerialConfig_f();
void M_SerialConfig_Draw();
void M_SerialConfig_Key(keycode_t key);

void M_Menu_ModemConfig_f();
void M_ModemConfig_Draw();
void M_ModemConfig_Key(keycode_t key);

void M_Menu_LanConfig_f();
void M_LanConfig_Draw();
void M_LanConfig_Key(keycode_t key);

void M_Menu_GameOptions_f();
void M_GameOptions_Draw();
void M_GameOptions_Key(keycode_t key);

void M_Menu_Search_f();
void M_Search_Draw();
void M_Search_Key(keycode_t key);

void M_Menu_ServerList_f();
void M_ServerList_Draw();
void M_ServerList_Key(keycode_t key);

