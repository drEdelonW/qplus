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
gl_used_t used[8192];

// the command list holds counts and s/t values that are valid for every frame
int  commands[8192];
int  numcommands;

// all frames will have their vertexes rearranged and expanded so they are in the order expected by the command list
int  vertexorder[8192];
int  numorder;

int  allverts, alltris;

int  stripverts[128];
int  striptris[128];
int  stripcount;

/*
================
StripLength
================
*/

int StripLength(int starttri, int startv) {
    used[starttri] = USED_TEMP;

    mTriangle_p last = &triangles[starttri];

    stripverts[0] = last->vertindex[(startv + 0) % 3];
    stripverts[1] = last->vertindex[(startv + 1) % 3];
    stripverts[2] = last->vertindex[(startv + 2) % 3];

    striptris[0] = starttri;
    stripcount = 1;

    int m1 = last->vertindex[(startv + 2) % 3];
    int m2 = last->vertindex[(startv + 1) % 3];

    // look for a matching triangle
nexttri:
    {
        mTriangle_p check = &triangles[starttri + 1];
        for (int j = starttri + 1; j < pheader->numtris; j++, check++)
            if (check->facesfront == last->facesfront)
                for (int k = 0; k < 3; k++)
                    if ((check->vertindex[k] == m1) &&
                        (check->vertindex[(k + 1) % 3] == m2)
                        ) {
                        // this is the next part of the fan
                        // if we can't use this triangle, this tristrip is done
                        if (used[j])    goto done;

                        // the new edge
                        if (stripcount & 1)     m2 = check->vertindex[(k + 2) % 3];
                        else                    m1 = check->vertindex[(k + 2) % 3];

                        stripverts[stripcount + 2] = check->vertindex[(k + 2) % 3];
                        striptris[stripcount] = j;
                        stripcount++;

                        used[j] = USED_TEMP;
                        goto nexttri;
                    }
    }
done:

    // clear the temp used flags
    for (int j = starttri + 1; j < pheader->numtris; j++)
        if (used[j] == USED_TEMP)
            used[j] = USED_FREE;

    return stripcount;
}

/*
===========
FanLength
===========
*/
int FanLength(int starttri, int startv) {
    used[starttri] = USED_TEMP;

    mTriangle_p last = &triangles[starttri];

    stripverts[0] = last->vertindex[(startv + 0) % 3];
    stripverts[1] = last->vertindex[(startv + 1) % 3];
    stripverts[2] = last->vertindex[(startv + 2) % 3];

    striptris[0] = starttri;
    stripcount = 1;

    int m1 = last->vertindex[(startv + 0) % 3];
    int m2 = last->vertindex[(startv + 2) % 3];


    // look for a matching triangle
nexttri:
    {
        mTriangle_p check = &triangles[starttri + 1];
        for (int j = starttri + 1; j < pheader->numtris; j++, check++)
            if (check->facesfront == last->facesfront)
                for (int k = 0; k < 3; k++)
                    if ((check->vertindex[k] == m1) &&
                        (check->vertindex[(k + 1) % 3] == m2)
                        ) {
                        // this is the next part of the fan

                        // if we can't use this triangle, this tristrip is done
                        if (used[j])
                            goto done;

                        // the new edge
                        m2 = check->vertindex[(k + 2) % 3];

                        stripverts[stripcount + 2] = m2;
                        striptris[stripcount] = j;
                        stripcount++;

                        used[j] = USED_TEMP;
                        goto nexttri;
                    }
    }
done:

    // clear the temp used flags
    for (int j = starttri + 1; j < pheader->numtris; j++)
        if (used[j] == USED_TEMP)
            used[j] = USED_FREE;

    return stripcount;
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
    numorder = 0;
    numcommands = 0;
    memset(used, 0, sizeof(used));
    for (int i = 0; i < pheader->numtris; i++) {
        // pick an unused triangle and start the trifan
        if (used[i])    continue;

        int bestLen = 0;
        int besttype;
        int bestverts[1024];
        int besttris[1024];
        for (int type = 0; type < 2; type++)
            // type = 1;
        {
            for (int startv = 0; startv < 3; startv++) {
                int len = (type == 1) ?
                    StripLength(i, startv) :
                    FanLength(i, startv);

                if (len > bestLen) {
                    besttype = type;
                    bestLen = len;
                    for (int j = 0; j < bestLen + 2; j++)
                        bestverts[j] = stripverts[j];

                    for (int j = 0; j < bestLen; j++)
                        besttris[j] = striptris[j];
                }
            }
        }

        // mark the tris on the best strip as used
        for (int j = 0; j < bestLen; j++)
            used[besttris[j]] = USED_INUSE;

        if (besttype == 1)  commands[numcommands++] = (bestLen + 2);
        else                commands[numcommands++] = -(bestLen + 2);

        for (int j = 0; j < bestLen + 2; j++) {
            // emit a vertex into the reorder buffer
            int k = bestverts[j];
            vertexorder[numorder++] = k;

            // emit s/t coords into the commands stream
            float s = stverts[k].s;
            float t = stverts[k].t;
            if (!triangles[besttris[0]].facesfront &&
                stverts[k].onseam
                )
                s += pheader->skinwidth / 2; // on back side
            s = (s + 0.5) / pheader->skinwidth;
            t = (t + 0.5) / pheader->skinheight;

            *(float_p)&commands[numcommands++] = s;
            *(float_p)&commands[numcommands++] = t;
        }
    }

    commands[numcommands++] = 0;  // end of list marker

    Con_DPrintf("%3i tri %3i vert %3i cmd\n", pheader->numtris, numorder, numcommands);

    allverts += numorder;
    alltris += pheader->numtris;
}


/*
================
GL_MakeAliasModelDisplayLists
================
*/
void GL_MakeAliasModelDisplayLists(Model_p m, AliasHdr_p hdr) {
    paliashdr = hdr; // (AliasHdr_t *)Mod_Extradata (m);

    //
    // look for a cached version
    //
    char cache[MAX_QPATH]; strcpy(cache, "glquake/");
    COM_StripExtension(m->name + strlen("progs/"), cache + strlen("glquake/"));
    strcat(cache, ".ms2");

    FILE* f;
    COM_FOpenFile(cache, &f);
    if (f) {
        fread(&numcommands, 4, 1, f);
        fread(&numorder, 4, 1, f);
        fread(&commands, numcommands * sizeof(commands[0]), 1, f);
        fread(&vertexorder, numorder * sizeof(vertexorder[0]), 1, f);
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
        char fullpath[MAX_OSPATH];  snprintf(fullpath, sizeof(fullpath), "%s/%s", com.gamedir, cache);
        f = fopen(fullpath, "wb");
        if (f) {
            fwrite(&numcommands, 4, 1, f);
            fwrite(&numorder, 4, 1, f);
            fwrite(&commands, numcommands * sizeof(commands[0]), 1, f);
            fwrite(&vertexorder, numorder * sizeof(vertexorder[0]), 1, f);
            fclose(f);
        }
    }


    // save the data out

    paliashdr->poseverts = numorder;

    int* cmds = Hunk_Alloc(numcommands * 4);
    paliashdr->commands = (byte*)cmds - (byte*)paliashdr;
    memcpy(cmds, commands, numcommands * 4);

    TriVertx_p verts = Hunk_Alloc(paliashdr->numposes * paliashdr->poseverts * sizeof(TriVertx_t));
    paliashdr->posedata = (byte*)verts - (byte*)paliashdr;
    for (int i = 0; i < paliashdr->numposes; i++)
        for (int j = 0; j < numorder; j++)
            *verts++ = poseverts[i][vertexorder[j]];
}

