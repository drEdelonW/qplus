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
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include "sound.h"
#include "common.h"
#include "console.h"

static int _audio_fd;
static bool _snd_inited = false;

static int _tryrates[] = {
    11025,
    22051,
    44100,
    8000
};

bool SNDDMA_Init() {
    _snd_inited = false;

    // open /dev/dsp, confirm capability to mmap, and get size of dma buffer

    _audio_fd = open("/dev/dsp", O_RDWR);
    if (_audio_fd < 0) {
        perror("/dev/dsp");
        Con_Printf("Could not open /dev/dsp\n");
        return 0;
    }

    int rc = ioctl(_audio_fd, SNDCTL_DSP_RESET, 0);
    if (rc < 0) {
        perror("/dev/dsp");
        Con_Printf("Could not reset /dev/dsp\n");
        close(_audio_fd);
        return 0;
    }

    int dsp_cap_flags;
    if (ioctl(_audio_fd, SNDCTL_DSP_GETCAPS, &dsp_cap_flags) == -1) {
        perror("/dev/dsp");
        Con_Printf("Sound driver too old\n");
        close(_audio_fd);
        return 0;
    }

    if (!(dsp_cap_flags & DSP_CAP_TRIGGER) || !(dsp_cap_flags & DSP_CAP_MMAP)) {
        Con_Printf("Sorry but your soundcard can't do this\n");
        close(_audio_fd);
        return 0;
    }

    audio_buf_info info;
    if (ioctl(_audio_fd, SNDCTL_DSP_GETOSPACE, &info) == -1) {
        perror("GETOSPACE");
        Con_Printf("Um, can't do GETOSPACE?\n");
        close(_audio_fd);
        return 0;
    }

    shm = &sn;
    shm->splitbuffer = 0;

    // set sample bits & speed

    cString s = getenv("QUAKE_SOUND_SAMPLEBITS");
    int param;
    if (s) shm->samplebits = atoi(s);
    else if ((param = COM_CheckParm("-sndbits")) != 0)
        shm->samplebits = atoi(com.argv[param + 1]);
    if ((shm->samplebits != 16) &&
        (shm->samplebits != 8)) {
        int fmt;
        ioctl(_audio_fd, SNDCTL_DSP_GETFMTS, &fmt);
        if (fmt & AFMT_S16_LE)
            shm->samplebits = 16;
        else if (fmt & AFMT_U8)
            shm->samplebits = 8;
    }

    s = getenv("QUAKE_SOUND_SPEED");
    if (s) shm->speed = atoi(s);
    else if ((param = COM_CheckParm("-sndspeed")) != 0)
        shm->speed = atoi(com.argv[param + 1]);
    else {
        for (int i = 0; i < sizeof(_tryrates) / 4; i++)
            if (!ioctl(_audio_fd, SNDCTL_DSP_SPEED, &_tryrates[i])) break;
        shm->speed = _tryrates[param];
    }

    s = getenv("QUAKE_SOUND_CHANNELS");
    if (s) shm->channels = atoi(s);
    else if ((param = COM_CheckParm("-sndmono")) != 0)
        shm->channels = 1;
    else if ((param = COM_CheckParm("-sndstereo")) != 0)
        shm->channels = 2;
    else
        shm->channels = 2;

    shm->samples = info.fragstotal * info.fragsize / (shm->samplebits / 8);
    shm->submission_chunk = 1;

    // memory map the dma buffer

    shm->buffer = (uint8_p)mmap(NULL, info.fragstotal
        * info.fragsize, PROT_WRITE, MAP_FILE | MAP_SHARED, _audio_fd, 0);
    if (!shm->buffer || shm->buffer == (uint8_p)-1) {
        perror("/dev/dsp");
        Con_Printf("Could not mmap /dev/dsp\n");
        close(_audio_fd);
        return 0;
    }

    int tmp = 0;
    if (shm->channels == 2)
        tmp = 1;
    rc = ioctl(_audio_fd, SNDCTL_DSP_STEREO, &tmp);
    if (rc < 0) {
        perror("/dev/dsp");
        Con_Printf("Could not set /dev/dsp to stereo=%d", shm->channels);
        close(_audio_fd);
        return 0;
    }
    if (tmp)
        shm->channels = 2;
    else
        shm->channels = 1;

    rc = ioctl(_audio_fd, SNDCTL_DSP_SPEED, &shm->speed);
    if (rc < 0) {
        perror("/dev/dsp");
        Con_Printf("Could not set /dev/dsp speed to %d", shm->speed);
        close(_audio_fd);
        return 0;
    }

    if (shm->samplebits == 16) {
        rc = AFMT_S16_LE;
        rc = ioctl(_audio_fd, SNDCTL_DSP_SETFMT, &rc);
        if (rc < 0) {
            perror("/dev/dsp");
            Con_Printf("Could not support 16-bit data.  Try 8-bit.\n");
            close(_audio_fd);
            return 0;
        }
    }
    else if (shm->samplebits == 8) {
        rc = AFMT_U8;
        rc = ioctl(_audio_fd, SNDCTL_DSP_SETFMT, &rc);
        if (rc < 0) {
            perror("/dev/dsp");
            Con_Printf("Could not support 8-bit data.\n");
            close(_audio_fd);
            return 0;
        }
    }
    else {
        perror("/dev/dsp");
        Con_Printf("%d-bit sound not supported.", shm->samplebits);
        close(_audio_fd);
        return 0;
    }

    // toggle the trigger & start her up

    tmp = 0;
    rc = ioctl(_audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
    if (rc < 0) {
        perror("/dev/dsp");
        Con_Printf("Could not toggle.\n");
        close(_audio_fd);
        return 0;
    }
    tmp = PCM_ENABLE_OUTPUT;
    rc = ioctl(_audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
    if (rc < 0) {
        perror("/dev/dsp");
        Con_Printf("Could not toggle.\n");
        close(_audio_fd);
        return 0;
    }

    shm->samplepos = 0;

    _snd_inited = true;
    return 1;

}

int SNDDMA_GetDMAPos() {
    if (!_snd_inited)
        return 0;

    count_info count;
    if (ioctl(_audio_fd, SNDCTL_DSP_GETOPTR, &count) == -1) {
        perror("/dev/dsp");
        Con_Printf("Uh, sound dead.\n");
        close(_audio_fd);
        _snd_inited = false;
        return 0;
    }
    // shm->samplepos = (count.bytes / (shm->samplebits / 8)) & (shm->samples-1);
    // fprintf(stderr, "%d    \r", count.ptr);
    shm->samplepos = count.ptr / (shm->samplebits / 8);

    return shm->samplepos;
}

void SNDDMA_Shutdown() {
    if (_snd_inited) {
        close(_audio_fd);
        _snd_inited = false;
    }
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit() {
}

