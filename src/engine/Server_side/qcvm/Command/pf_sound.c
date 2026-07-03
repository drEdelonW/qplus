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

#include "progs.h"
#include "progdefs.h"
#include "GlobVars.h"
#include "Edict.h"
#include "msg.h"
#include "host.h"
#include "console.h"
#include "server.h"
#include "protocol.h"
#include <string.h>


/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound() {
    edict_p entity = G_EDICT(OFS_PARM0);
    SndCh_t channel = (SndCh_t)G_FLOAT(OFS_PARM1);
    cString sample = G_STRING(OFS_PARM2);
    int volume = (int)G_FLOAT(OFS_PARM3) * 255;
    float attenuation = G_FLOAT(OFS_PARM4);

    if ((volume < VolSilent) || (volume > VolFull)
    )   Host_SysError("SV_StartSound: volume = %i", volume);

    if ((attenuation < AtnNone) || (attenuation > AtnMax)
    )   Host_SysError("SV_StartSound: attenuation = %f", attenuation);

    if ((channel < SndChAuto) || (channel > SndChMax)
    )   Host_SysError("SV_StartSound: channel = %i", channel);

    SV_StartSound(entity, channel, sample, volume, attenuation);
}


/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound() {
    vec3_t pos = G_VECTOR(OFS_PARM0);
    cString samp = G_STRING(OFS_PARM1);
    float vol = G_FLOAT(OFS_PARM2);
    float attenuation = G_FLOAT(OFS_PARM3);

    // check to see if samp was properly precached
    cStringArray check = sv.sound_precache;
    uint8_t soundnum = 0;
    for (; *check; check++, soundnum++)
        if (!strcmp(*check, samp))
            break;

    if (!*check) {
        Con_Printf("no precache: %s\n", samp);
        return;
    }

    // add an svc_spawnambient command to the level signon packet

    MSG_WriteByte(&sv.signon, svc_spawnstaticsound);
    for (int i = 0; i < VECT_DIM; i++)
        MSG_WriteCoord(&sv.signon, pos.v[i]);

    MSG_WriteByte(&sv.signon, soundnum);

    MSG_WriteByte(&sv.signon, (uint8_t)(vol * 255));
    MSG_WriteByte(&sv.signon, (uint8_t)(attenuation * 64));
}

