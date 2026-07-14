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
#include "progdefs.h"
#include "crc.h"

//============================================================================

extern int32_t      pr_argc;        // number of op_call
extern bool         pr_trace;
// extern uint16_t     pr_crc;

//============================================================================
#ifdef __cplusplus
extern "C" {
#endif

    CRC_t PR_getCRC();
    void PR_Init();
    void PR_LoadProgs();
    void PR_ExecuteProgram(func_t fnum);
    void PR_Profile_f();
    Q_NORETURN void PR_RunError(cString error, ...);

#ifdef __cplusplus
}
#endif