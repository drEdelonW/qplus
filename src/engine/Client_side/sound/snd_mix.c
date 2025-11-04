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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "sound.h"
#include "common.h"
#include "cvar_q1.h"
#include "q_tools.h"

#ifdef _WIN32
#   include "winquake.h"
#else
#   define DWORD uint32_t
typedef DWORD* LPVOID;
#endif

#define PAINTBUFFER_SIZE 512
static portable_samplepair_t _paintbuffer[PAINTBUFFER_SIZE];
static int _snd_scaletable[32][256];
static int* _snd_p;
static int _snd_linear_count, _snd_vol;
static int16_p _snd_out;

void Snd_WriteLinearBlastStereo16();
#define MAX_SND_VAL (0x7fff)
#define MIN_SND_VAL ((int16_t)0x8000)
#if !id386
void Snd_WriteLinearBlastStereo16() {
    for (int i = 0; i < _snd_linear_count; i += 2) {
        {
            int val = (_snd_p[i] * _snd_vol) >> 8;
            if (val > MAX_SND_VAL)      _snd_out[i] = MAX_SND_VAL;
            else if (val < MIN_SND_VAL) _snd_out[i] = MIN_SND_VAL;
            else                        _snd_out[i] = val;
        }
        {
            int val = (_snd_p[i + 1] * _snd_vol) >> 8;
            if (val > MAX_SND_VAL)      _snd_out[i + 1] = MAX_SND_VAL;
            else if (val < MIN_SND_VAL) _snd_out[i + 1] = MIN_SND_VAL;
            else                        _snd_out[i + 1] = val;
        }
    }
}
#endif

void S_TransferStereo16(int endtime) {
    LPVOID pbuf;
    _snd_vol = volume.value * 256;
    _snd_p = (int*)_paintbuffer;
    int lpaintedtime = paintedtime;
#ifdef _WIN32
    if (pDSBuf) {
        int reps = 0;

        DWORD dwSize, dwSize2;
        LPVOID pbuf2;
        HRESULT hresult;
        while ((hresult = pDSBuf->lpVtbl->Lock(pDSBuf, 0, gSndBufSize, &pbuf, &dwSize,
            &pbuf2, &dwSize2, 0)) != DS_OK) {
            if (hresult != DSERR_BUFFERLOST) {
                Con_Printf("S_TransferStereo16: DS::Lock Sound Buffer Failed\n");
                S_Shutdown();
                S_Startup();
                return;
            }

            if (++reps > 10000) {
                Con_Printf("S_TransferStereo16: DS: couldn't restore buffer\n");
                S_Shutdown();
                S_Startup();
                return;
            }
        }
    }
    else
#endif
    {
        pbuf = (DWORD*)shm->buffer;
    }

    while (lpaintedtime < endtime) {
        // handle recirculating buffer issues
        int lpos = lpaintedtime & ((shm->samples >> 1) - 1);

        _snd_out = (int16_p)pbuf + (lpos << 1);

        _snd_linear_count = (shm->samples >> 1) - lpos;
        if (lpaintedtime + _snd_linear_count > endtime)
            _snd_linear_count = endtime - lpaintedtime;

        _snd_linear_count <<= 1;

        // write a linear blast of samples
        Snd_WriteLinearBlastStereo16();

        _snd_p += _snd_linear_count;
        lpaintedtime += (_snd_linear_count >> 1);
    }

#ifdef _WIN32
    if (pDSBuf)
        pDSBuf->lpVtbl->Unlock(pDSBuf, pbuf, dwSize, NULL, 0);
#endif
}

void S_TransferPaintBuffer(int endtime) {
    if ((shm->samplebits == 16) && (shm->channels == 2)) {
        S_TransferStereo16(endtime);
        return;
    }

    int* p = (int*)_paintbuffer;
    int count = (endtime - paintedtime) * shm->channels;
    int out_mask = shm->samples - 1;
    int out_idx = paintedtime * shm->channels & out_mask;
    int step = 3 - shm->channels;
    int snd_vol = volume.value * 256;

    LPVOID pbuf;
#ifdef _WIN32
    if (pDSBuf) {
        int reps = 0;

        DWORD dwSize, dwSize2;
        LPVOID pbuf2;
        HRESULT hresult;
        while ((hresult = pDSBuf->lpVtbl->Lock(pDSBuf, 0, gSndBufSize, &pbuf, &dwSize,
            &pbuf2, &dwSize2, 0)) != DS_OK) {
            if (hresult != DSERR_BUFFERLOST) {
                Con_Printf("S_TransferPaintBuffer: DS::Lock Sound Buffer Failed\n");
                S_Shutdown();
                S_Startup();
                return;
            }

            if (++reps > 10000) {
                Con_Printf("S_TransferPaintBuffer: DS: couldn't restore buffer\n");
                S_Shutdown();
                S_Startup();
                return;
            }
        }
    }
    else
#endif
    {
        pbuf = (DWORD*)shm->buffer;
    }

    if (shm->samplebits == 16) {
        int16_p out = (int16_p)pbuf;
        while (count--) {
            int val = (*p * snd_vol) >> 8;
            p += step;
            if (val > MAX_SND_VAL)          val = MAX_SND_VAL;
            else if (val < MIN_SND_VAL)     val = MIN_SND_VAL;
            out[out_idx] = val;
            out_idx = (out_idx + 1) & out_mask;
        }
    }
    else if (shm->samplebits == 8) {
        uint8_p out = (uint8_p)pbuf;
        while (count--) {
            int val = (*p * snd_vol) >> 8;
            p += step;
            if (val > MAX_SND_VAL)          val = MAX_SND_VAL;
            else if (val < MIN_SND_VAL)     val = MIN_SND_VAL;
            out[out_idx] = (val >> 8) + 128;
            out_idx = (out_idx + 1) & out_mask;
        }
    }

#ifdef _WIN32
    if (pDSBuf) {
        DWORD dwNewpos, dwWrite;
        int il = paintedtime;
        int ir = endtime - paintedtime;

        ir += il;

        pDSBuf->lpVtbl->Unlock(pDSBuf, pbuf, dwSize, NULL, 0);

        pDSBuf->lpVtbl->GetCurrentPosition(pDSBuf, &dwNewpos, &dwWrite);

        //  if ((dwNewpos >= il) && (dwNewpos <= ir))
        //   Con_Printf("%d-%d p %d c\n", il, ir, dwNewpos);
    }
#endif
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

void SND_PaintChannelFrom8(channel_p ch, sfxcache_p sc, int endtime);
void SND_PaintChannelFrom16(channel_p ch, sfxcache_p sc, int endtime);

void S_PaintChannels(int endtime) {
    while (paintedtime < endtime) {
        // if _paintbuffer is smaller than DMA buffer
        int end = endtime;
        if (endtime - paintedtime > PAINTBUFFER_SIZE)
            end = paintedtime + PAINTBUFFER_SIZE;

        // clear the paint buffer
        Q_memset(_paintbuffer, 0, (end - paintedtime) * sizeof(portable_samplepair_t));

        // paint in the channels.
        channel_p ch = channels;
        for (int i = 0; i < total_channels; i++, ch++) {
            if ((!ch->sfx) ||
                (!ch->leftvol && !ch->rightvol))
                continue;
            sfxcache_p sc = S_LoadSound(ch->sfx);
            if (!sc)
                continue;

            int ltime = paintedtime;

            while (ltime < end) { // paint up to end
                int count = ((ch->end < end) ? ch->end : end) - ltime;

                if (count > 0) {
                    if (sc->width == 1) SND_PaintChannelFrom8(ch, sc, count);
                    else                SND_PaintChannelFrom16(ch, sc, count);

                    ltime += count;
                }

                // if at end of loop, restart
                if (ltime >= ch->end) {
                    if (sc->loopstart >= 0) {
                        ch->pos = sc->loopstart;
                        ch->end = ltime + sc->length - ch->pos;
                    }
                    else { // channel just stopped
                        ch->sfx = NULL;
                        break;
                    }
                }
            }

        }

        // transfer out according to DMA format
        S_TransferPaintBuffer(end);
        paintedtime = end;
    }
}

void SND_InitScaletable() {
    for (int i = 0; i < 32; i++)
        for (int j = 0; j < 256; j++)
            _snd_scaletable[i][j] = ((int8_t)j) * i * 8;
}


#if !id386

void SND_PaintChannelFrom8(channel_p ch, sfxcache_p sc, int count) {
    if (ch->leftvol > 255)
        ch->leftvol = 255;
    if (ch->rightvol > 255)
        ch->rightvol = 255;

    int* lscale = _snd_scaletable[ch->leftvol >> 3];
    int* rscale = _snd_scaletable[ch->rightvol >> 3];
    uint8_p sfx = (uint8_p)sc->data + ch->pos;

    for (int i = 0; i < count; i++) {
        int data = sfx[i];
        _paintbuffer[i].left += lscale[data];
        _paintbuffer[i].right += rscale[data];
    }

    ch->pos += count;
}

#endif // !id386


void SND_PaintChannelFrom16(channel_p ch, sfxcache_p sc, int count) {
    int leftvol = ch->leftvol;
    int rightvol = ch->rightvol;
    int16_p sfx = (int16_p)sc->data + ch->pos;

    for (int i = 0; i < count; i++) {
        int data = sfx[i];
        int left = (data * leftvol) >> 8;
        int right = (data * rightvol) >> 8;
        _paintbuffer[i].left += left;
        _paintbuffer[i].right += right;
    }

    ch->pos += count;
}

