#pragma once

#include "keys.h"
#include "draw.h"


void M_DrawCharacter(int cx, int line, int num);
void M_DrawTransPic(int x, int y, qpic_p pic);
void M_DrawPic(int x, int y, qpic_p pic);

void M_ConfigureNetSubsystem();

void M_Menu_Main_f();
void M_Menu_SinglePlayer_f();
void M_Menu_Load_f();
void M_Menu_Save_f();
void M_Menu_MultiPlayer_f();
void M_Menu_Setup_f();
void M_Menu_Net_f();
void M_Menu_Options_f();
void M_Menu_Keys_f();
void M_Menu_Video_f();
void M_Menu_Help_f();
void M_Menu_Quit_f();
void M_Menu_SerialConfig_f();
void M_Menu_ModemConfig_f();
void M_Menu_LanConfig_f();
void M_Menu_GameOptions_f();
void M_Menu_Search_f();
void M_Menu_ServerList_f();

void M_Main_Draw();
void M_SinglePlayer_Draw();
void M_Load_Draw();
void M_Save_Draw();
void M_MultiPlayer_Draw();
void M_Setup_Draw();
void M_Net_Draw();
void M_Options_Draw();
void M_Keys_Draw();
void M_Video_Draw();
void M_Help_Draw();
void M_Quit_Draw();
void M_SerialConfig_Draw();
void M_ModemConfig_Draw();
void M_LanConfig_Draw();
void M_GameOptions_Draw();
void M_Search_Draw();
void M_ServerList_Draw();

void M_Main_Key(keycode_t key);
void M_SinglePlayer_Key(keycode_t key);
void M_Load_Key(keycode_t key);
void M_Save_Key(keycode_t key);
void M_MultiPlayer_Key(keycode_t key);
void M_Setup_Key(keycode_t key);
void M_Net_Key(keycode_t key);
void M_Options_Key(keycode_t key);
void M_Keys_Key(keycode_t key);
void M_Video_Key(keycode_t key);
void M_Help_Key(keycode_t key);
void M_Quit_Key(keycode_t key);
void M_SerialConfig_Key(keycode_t key);
void M_ModemConfig_Key(keycode_t key);
void M_LanConfig_Key(keycode_t key);
void M_GameOptions_Key(keycode_t key);
void M_Search_Key(keycode_t key);
void M_ServerList_Key(keycode_t key);