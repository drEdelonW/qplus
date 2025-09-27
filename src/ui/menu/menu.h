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

//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//
#include "keys.h"
typedef enum {
    MNET_IPX = 1,
    MNET_TCP = 2
} mnet_type_t;


typedef enum {
    m_none,
    m_main,
    m_singleplayer,
    m_load,
    m_save,
    m_multiplayer,
    m_setup,
    m_net,
    m_options,
    m_video,
    m_keys,
    m_help,
    m_quit,
    m_serialconfig,
    m_modemconfig,
    m_lanconfig,
    m_gameoptions,
    m_search,
    m_slist
} m_state_t;

extern m_state_t m_state;
extern m_state_t m_return_state;

extern int m_activenet;

//
// menus
//
void M_Init();
void M_Keydown(keycode_t key);
void M_Draw();
void M_ToggleMenu_f();


