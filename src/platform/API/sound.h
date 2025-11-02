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
// sound.h -- client sound i/o functions

#include "sound/sound_struct.h"
#include "vector.h"

#define DEFAULT_SOUND_PACKET_VOLUME         (255)
#define DEFAULT_SOUND_PACKET_ATTENUATION    (1.0)

    // ====================================================================
    // User-setable variables
    // ====================================================================

#define	MAX_CHANNELS			(128)
#define	MAX_DYNAMIC_CHANNELS	(8)

extern	channel_t channels[MAX_CHANNELS];
// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds

extern	int total_channels;

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern bool     fakedma;
extern int      fakedma_updates;
extern int      paintedtime;
extern vec3_t   listener_origin;
extern vec3_t   listener_forward;
extern vec3_t   listener_right;
extern vec3_t   listener_up;
extern volatile dma_t* shm;
extern volatile dma_t sn;
extern vec_t    sound_nominal_clip_dist;

extern bool snd_initialized;
extern int      snd_blocked;

#ifdef __cplusplus
extern "C" {
#endif

    void S_Init();
    void S_Startup();
    void S_Shutdown();
    void S_StartSound(int entnum, int entchannel, sfx_p sfx, vec3_t origin, float fvol, float attenuation);
    void S_StaticSound(sfx_p sfx, vec3_t origin, float vol, float attenuation);
    void S_StopSound(int entnum, int entchannel);
    void S_StopAllSounds(bool clear);
    void S_ClearBuffer();
    void S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
    void S_ExtraUpdate();

    sfx_p S_PrecacheSound(cString sample);
    void S_TouchSound(cString sample);
    void S_ClearPrecache();
    void S_BeginPrecaching();
    void S_EndPrecaching();
    void S_PaintChannels(int endtime);
    void S_InitPaintChannels();

    channel_p SND_PickChannel(int entnum, int entchannel);  // picks a channel based on priorities, empty slots, number of channels
    void SND_Spatialize(channel_p ch);      // spatializes a channel
    bool SNDDMA_Init();         // initializes cycling through a DMA buffer and returns information on it
    int  SNDDMA_GetDMAPos();    // gets the current DMA position
    void SNDDMA_Shutdown();     // shutdown the DMA xfer.

    void S_LocalSound(cString s);
    sfxcache_p S_LoadSound(sfx_p s);

    wavinfo_t GetWavinfo(cString name, cString wav, int wavlength);

    void SND_InitScaletable();
    void SNDDMA_Submit();

    void S_AmbientOff();
    void S_AmbientOn();

#ifdef __cplusplus
}
#endif