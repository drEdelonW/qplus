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
// gl_mesh.c: triangle model functions


/*
=================================================================

ALIAS MODEL DISPLAY LIST GENERATION

=================================================================
*/
#include "Model_st.h"
#include "model.h"
#include "AliasModel.h"
#include "BrushModel.h"
#include "console.h"
#include "common.h"
#include <string.h>
#include "z_hunk.h"


typedef enum {
    USED_FREE = 0u,
    USED_INUSE,
    USED_TEMP
} gl_used_t; // 0 is free, 1 is use, 2 is temp_used
static gl_used_t _used[8192];

// the command list holds counts and s/t values that are valid for every frame
static int _commands[8192];
static int _numCommands;

// all frames will have their vertexes rearranged and expanded so they are in the order expected by the command list
static int _vertexOrder[8192];
static int _numOrder;

#ifdef DEBUG
static int _allVerts;
static int _allTris;
#endif

static int _stripVerts[128];
static int _stripTris[128];
static int _stripCount;

/*
================
StripLength
================
*/

int StripLength(int starttri, int startv) {
    _used[starttri] = USED_TEMP;

    mTriangle_p last = &triangles[starttri];

    _stripVerts[0] = last->vertindex[(startv + 0) % 3];
    _stripVerts[1] = last->vertindex[(startv + 1) % 3];
    _stripVerts[2] = last->vertindex[(startv + 2) % 3];

    _stripTris[0] = starttri;
    _stripCount = 1;

    int m1 = last->vertindex[(startv + 2) % 3];
    int m2 = last->vertindex[(startv + 1) % 3];

    // look for a matching triangle
nexttri:
    {
        mTriangle_p check = &triangles[starttri + 1];
        for (int j = starttri + 1; j < pHeader->numtris; j++, check++)
            if (check->facesfront == last->facesfront)
                for (int k = 0; k < 3; k++)
                    if ((check->vertindex[k] == m1) &&
                        (check->vertindex[(k + 1) % 3] == m2)
                        ) {
                        // this is the next part of the fan
                        // if we can't use this triangle, this tristrip is done
                        if (_used[j])    goto done;

                        // the new edge
                        if (_stripCount & 1)     m2 = check->vertindex[(k + 2) % 3];
                        else                    m1 = check->vertindex[(k + 2) % 3];

                        _stripVerts[_stripCount + 2] = check->vertindex[(k + 2) % 3];
                        _stripTris[_stripCount] = j;
                        _stripCount++;

                        _used[j] = USED_TEMP;
                        goto nexttri;
                    }
    }
done:

    // clear the temp _used flags
    for (int j = starttri + 1; j < pHeader->numtris; j++)
        if (_used[j] == USED_TEMP)
            _used[j] = USED_FREE;

    return _stripCount;
}

/*
===========
FanLength
===========
*/
int FanLength(int starttri, int startv) {
    _used[starttri] = USED_TEMP;

    mTriangle_p last = &triangles[starttri];

    _stripVerts[0] = last->vertindex[(startv + 0) % 3];
    _stripVerts[1] = last->vertindex[(startv + 1) % 3];
    _stripVerts[2] = last->vertindex[(startv + 2) % 3];

    _stripTris[0] = starttri;
    _stripCount = 1;

    int m1 = last->vertindex[(startv + 0) % 3];
    int m2 = last->vertindex[(startv + 2) % 3];


    // look for a matching triangle
nexttri:
    {
        mTriangle_p check = &triangles[starttri + 1];
        for (int j = starttri + 1; j < pHeader->numtris; j++, check++)
            if (check->facesfront == last->facesfront)
                for (int k = 0; k < 3; k++)
                    if ((check->vertindex[k] == m1) &&
                        (check->vertindex[(k + 1) % 3] == m2)
                        ) {
                        // this is the next part of the fan

                        // if we can't use this triangle, this tristrip is done
                        if (_used[j])
                            goto done;

                        // the new edge
                        m2 = check->vertindex[(k + 2) % 3];

                        _stripVerts[_stripCount + 2] = m2;
                        _stripTris[_stripCount] = j;
                        _stripCount++;

                        _used[j] = USED_TEMP;
                        goto nexttri;
                    }
    }
done:

    // clear the temp _used flags
    for (int j = starttri + 1; j < pHeader->numtris; j++)
        if (_used[j] == USED_TEMP)
            _used[j] = USED_FREE;

    return _stripCount;
}


/*
================
BuildTris

Generate a list of trifans or strips
for the model, which holds for all frames
================
*/
void BuildTris() {
    //
    // build tristrips
    //
    _numOrder = 0;
    _numCommands = 0;
    memset(_used, USED_FREE, sizeof(_used));
    for (int i = 0; i < pHeader->numtris; i++) {
        // pick an unused triangle and start the trifan
        if (_used[i])    continue;

        int bestLen = 0;
        int besttype;
        int bestverts[1024];
        int besttris[1024];
        for (int type = 0; type < 2; type++) {
            for (int startv = 0; startv < 3; startv++) {
                int len = (type == 1) ?
                    StripLength(i, startv) : FanLength(i, startv);

                if (len > bestLen) {
                    besttype = type;
                    bestLen = len;
                    for (int j = 0; j < bestLen + 2; j++)
                        bestverts[j] = _stripVerts[j];

                    for (int j = 0; j < bestLen; j++)
                        besttris[j] = _stripTris[j];
                }
            }
        }

        // mark the tris on the best strip as _used
        for (int j = 0; j < bestLen; j++)
            _used[besttris[j]] = USED_INUSE;

        if (besttype == 1)  _commands[_numCommands++] = (bestLen + 2);
        else                _commands[_numCommands++] = -(bestLen + 2);

        for (int j = 0; j < bestLen + 2; j++) {
            // emit a vertex into the reorder buffer
            int k = bestverts[j];
            _vertexOrder[_numOrder++] = k;

            // emit s/t coords into the commands stream
            float s = stverts[k].s;
            float t = stverts[k].t;
            if (!(triangles[besttris[0]].facesfront) &&
                (stverts[k].onseam)
                )   s += HALF(pHeader->skinwidth); // on back side

            s = (s + 0.5) / pHeader->skinwidth;
            t = (t + 0.5) / pHeader->skinheight;

            *(float_p)&_commands[_numCommands++] = s;
            *(float_p)&_commands[_numCommands++] = t;
        }
    }

    _commands[_numCommands++] = 0;  // end of list marker

    Con_DPrintf(
        "%3i tri %3i vert %3i cmd\n",
        pHeader->numtris, _numOrder, _numCommands
    );

#ifdef DEBUG
    _allVerts += _numOrder;
    _allTris += pHeader->numtris;
#endif
}


/*
================
GL_MakeAliasModelDisplayLists
================
*/
void GL_MakeAliasModelDisplayLists(Model_p m, AliasHdr_p hdr) {
    pAliasHdr = hdr; // (AliasHdr_t *)Mod_Extradata (m);

    //
    // look for a cached version
    //
    qPath_t cache; strcpy(cache, "glquake/");
    COM_StripExtension(m->name + strlen("progs/"), cache + strlen("glquake/"));
    strcat(cache, ".ms2");

    FILE* f;
    COM_FOpenFile(cache, &f);
    if (f) {
        fread(&_numCommands, 4, 1, f);
        fread(&_numOrder, 4, 1, f);
        fread(&_commands, _numCommands * sizeof(_commands[0]), 1, f);
        fread(&_vertexOrder, _numOrder * sizeof(_vertexOrder[0]), 1, f);
        fclose(f);
    }
    else {
        //
        // build it from scratch
        //
        Con_Printf("meshing %s...\n", m->name);

        BuildTris();  // trifans or lists

        //
        // save out the cached version
        //
        fsPath_t fullpath;  snprintf(fullpath, sizeof(fullpath), "%s/%s", com.gamedir, cache);
        f = fopen(fullpath, "wb");
        if (f) {
            fwrite(&_numCommands, 4, 1, f);
            fwrite(&_numOrder, 4, 1, f);
            fwrite(&_commands, _numCommands * sizeof(_commands[0]), 1, f);
            fwrite(&_vertexOrder, _numOrder * sizeof(_vertexOrder[0]), 1, f);
            fclose(f);
        }
    }


    // save the data out
    pAliasHdr->poseverts = _numOrder;

    int* cmds = Hunk_Alloc(_numCommands * 4);
    pAliasHdr->commands = (uint8_p)cmds - (uint8_p)pAliasHdr;
    memcpy(cmds, _commands, _numCommands * 4);

    TriVertx_p pVerts = Hunk_Alloc(pAliasHdr->numposes * pAliasHdr->poseverts * sizeof(TriVertx_t));
    pAliasHdr->posedata = (uint8_p)pVerts - (uint8_p)pAliasHdr;
    for (int i = 0; i < pAliasHdr->numposes; i++)
        for (int j = 0; j < _numOrder; j++)
            *pVerts++ = poseverts[i][_vertexOrder[j]];
}

