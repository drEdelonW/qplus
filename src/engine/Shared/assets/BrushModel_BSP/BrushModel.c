#include "BrushModel.h"
#include "model.h"
#include "bspfile.h"
#include "endian_tools.h"
#include "host.h"
#include <string.h>
#include <stdio.h>

/*
=================
Mod_LoadBrushModel
=================
*/

void Mod_LoadBrushModel(Model_p mod, TypeLess_ptr buffer) {
    _loadModel->type = mod_brush;
    dHeader_p header = (dHeader_p)buffer;

    int ver = LittleLong(header->version);
    if (ver != BSPVERSION)
        Host_SysError(
            "Mod_LoadBrushModel: %s has wrong version number "
            "(%i should be %i)",
            mod->name, ver, BSPVERSION
        );


    // swap all the lumps
    mod_base = (uint8_p)header;

    for (int i = 0; i < (sizeof(dHeader_t) / 4); i++) {
        ((int*)header)[i] = LittleLong(((int*)header)[i]);
    }

    // load into heap

    Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
    Mod_LoadEdges(&header->lumps[LUMP_EDGES]);
    Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
    Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
    Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
    Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
    Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
    Mod_LoadFaces(&header->lumps[LUMP_FACES]);
    Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES]);
    Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
    Mod_LoadLeafs(&header->lumps[LUMP_LEAFS]);
    Mod_LoadNodes(&header->lumps[LUMP_NODES]);
    Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES]);
    Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
    Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);

    Mod_MakeHull0();

    mod->numframes = 2;  // regular and alternate animation
    mod->flags = 0;

    //
    // set up the SubModels (FIXME: this is confusing)
    //
    for (int i = 0; i < mod->numSubModels; i++) {
        dModel_p bm = &mod->SubModels[i];

        mod->hulls[0].firstclipnode = bm->headnode[0];
        for (int j = 1; j < MAX_MAP_HULLS; j++) {
            mod->hulls[j].firstclipnode = bm->headnode[j];
            mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
        }

        mod->firstModelSurface = bm->firstface;
        mod->numModelSurfaces = bm->numfaces;

        VectorCopy(bm->maxs, mod->maxs);
        VectorCopy(bm->mins, mod->mins);
        mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

        mod->numleafs = bm->visleafs;

        if (i < (mod->numSubModels - 1)) { // duplicate the basic information
            char name[10];

            snprintf(name, sizeof(name), "*%i", i + 1);
            _loadModel = Mod_FindName(name);
            *_loadModel = *mod;
            strcpy(_loadModel->name, name);
            mod = _loadModel;
        }
    }
}
