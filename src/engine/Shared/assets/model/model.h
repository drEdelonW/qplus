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
#include "Model_st.h"

#define TOP_RANGE  (16)   // soldier uniform colors
#define BOTTOM_RANGE (96)
/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/
#ifdef GLQUAKE
#   include "Alias.h"
#   include "Vertex.h"

#   define MAXALIASVERTS 1024
#   define MAXALIASFRAMES 256
#   define MAXALIASTRIS 2048
extern AliasHdr_p pheader;
extern stvert_t stverts[MAXALIASVERTS];
extern mTriangle_t triangles[MAXALIASTRIS];
extern TriVertx_p poseverts[MAXALIASFRAMES];
#endif
extern char Mod_loadName[32]; // for hunk tags
extern uint8_p mod_base;
#ifdef GLQUAKE
#   define _loadModel loadmodel
#endif
extern Model_p _loadModel;

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

typedef enum {
    SIDE_FRONT  = 0u, // point is in front of plane
    SIDE_BACK   = 1u, // point is behind plane
    SIDE_ON     = 2u  // point is on plane
} Side_t;


//===================================================================

//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

    void Mod_Init();
    void Mod_ClearAll();

    Model_p Mod_ForName(cString name, bool crash);
    TypeLess_ptr Mod_Extradata(Model_p mod); // handles caching
    Model_p Mod_FindName(cString name);
    
    mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model);
    uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model);
    void Mod_TouchModel(cString name);
    void Mod_Print();

    void    PrintFrameName(Model_p mdl, int frame);

#ifdef __cplusplus
}
#endif