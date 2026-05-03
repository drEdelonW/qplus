#pragma once

#include "enginedefs.h"
#include "types.h"
#include "vid.h"
#include "vector.h"
#include "Surface.h"
#include "Leaf.h"
#include "render.h"
#include "Texture.h"
#include "cvar.h"
#include "render.h"

#include "glquake.h"
#include "Model_st.h"
#include "Alias.h"

#define MAXHEIGHT  1024
#define MAXWIDTH  1280

extern uint8_t	d_15to8table[65536];
extern vec3_t   lightspot;

bool VID_Is8bit();
int R_LightPoint(vec3_t p);
void R_DrawBrushModel(r_Entity_p e);
void R_DrawWorld();
void R_AnimateLight();
void R_RenderDlights();
void R_DrawParticles();
void R_DrawWaterSurfaces();
void R_RenderBrushPoly(mSurface_p fa);
void R_InitSky(Texture_p mt);
void GL_SubdivideSurface(mSurface_p fa);
void GL_MakeAliasModelDisplayLists(Model_p m, AliasHdr_p hdr);