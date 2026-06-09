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
// #include "Alias.h"
#include "Light.h"

#define MAXHEIGHT  1024
#define MAXWIDTH  1280

extern uint8_t	d_15to8table[65536];
extern vec3_t   lightspot;
extern int      solidskytexture;
extern int      alphaskytexture;
extern float    speedscale;  // for top sky and bottom sky
extern bool     isPermedia;
extern Model_p  _loadModel;
#define NUMVERTEXNORMALS 162
extern float r_avertexnormals[NUMVERTEXNORMALS][3];


bool VID_Is8bit();
int R_LightPoint(vec3_t p);
void R_DrawBrushModel(r_Entity_p e);
void R_DrawWorld();
void R_AnimateLight();
void R_RenderDlights();
void R_DrawParticles();
void R_DrawWaterSurfaces();
void R_RenderBrushPoly(mSurface_p fa);
void R_ClearParticles();
void R_InitParticles();
void R_InitSky(Texture_p mt);
void GL_SubdivideSurface(mSurface_p fa);
void GL_MakeAliasModelDisplayLists(Model_p m, AliasHdr_p hdr);
void GL_Upload8_EXT(uint8_p data, int width, int height, bool mipmap, bool alpha);
void GL_BuildLightmaps();
void EmitWaterPolys(mSurface_p fa);
void EmitSkyPolys(mSurface_p fa);
void EmitBothSkyLayers(mSurface_p fa);
void R_DrawSkyChain(mSurface_p s);
bool R_CullBox(vec3_t mins, vec3_t maxs);
void R_MarkLights(dLight_p light, int bit, mNode_p node);
void R_RotateForEntity(r_Entity_p e);
void R_StoreEfrags(efrag_ar ppefrag);
void GL_Set2D();
