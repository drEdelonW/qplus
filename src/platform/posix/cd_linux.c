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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <linux/cdrom.h>

#include "quakedef.h"
#include "cvar_q1.h"


static qboolean _cdValid = false;
static qboolean	_playing = false;
static qboolean	_wasPlaying = false;
static qboolean	_initialized = false;
static qboolean	_enabled = true;
static qboolean _playLooping = false;
static float    _cdvolume;
static uint8_t  _remap[100];
static uint8_t  _playTrack;
static uint8_t  _maxTrack;

static int  _cdfile = -1;
static char _cd_dev[64] = "/dev/cdrom";

static void CDAudio_Eject() {
    if ((_cdfile == -1) || !_enabled)
        return; // no cd init'd

    if (ioctl(_cdfile, CDROMEJECT) == -1)
        Con_DPrintf("ioctl cdromeject failed\n");
}


static void CDAudio_CloseDoor() {
    if ((_cdfile == -1) || !_enabled)
        return; // no cd init'd

    if (ioctl(_cdfile, CDROMCLOSETRAY) == -1)
        Con_DPrintf("ioctl cdromclosetray failed\n");
}

static int CDAudio_GetAudioDiskInfo() {
    struct cdrom_tochdr tochdr;

    _cdValid = false;

    if (ioctl(_cdfile, CDROMREADTOCHDR, &tochdr) == -1) {
        Con_DPrintf("ioctl cdromreadtochdr failed\n");
        return -1;
    }

    if (tochdr.cdth_trk0 < 1) {
        Con_DPrintf("CDAudio: no music tracks\n");
        return -1;
    }

    _cdValid = true;
    _maxTrack = tochdr.cdth_trk1;

    return 0;
}


void CDAudio_Play(uint8_t track, qboolean looping) {
    struct cdrom_tocentry entry;
    struct cdrom_ti ti;

    if ((_cdfile == -1) || !_enabled)
        return;

    if (!_cdValid) {
        CDAudio_GetAudioDiskInfo();
        if (!_cdValid)
            return;
    }

    track = _remap[track];

    if (track < 1 || track > _maxTrack) {
        Con_DPrintf("CDAudio: Bad track number %u.\n", track);
        return;
    }

    // don't try to play a non-audio track
    entry.cdte_track = track;
    entry.cdte_format = CDROM_MSF;
    if (ioctl(_cdfile, CDROMREADTOCENTRY, &entry) == -1) {
        Con_DPrintf("ioctl cdromreadtocentry failed\n");
        return;
    }
    if (entry.cdte_ctrl == CDROM_DATA_TRACK) {
        Con_Printf("CDAudio: track %i is not audio\n", track);
        return;
    }

    if (_playing) {
        if (_playTrack == track)
            return;
        CDAudio_Stop();
    }

    ti.cdti_trk0 = track;
    ti.cdti_trk1 = track;
    ti.cdti_ind0 = 1;
    ti.cdti_ind1 = 99;

    if (ioctl(_cdfile, CDROMPLAYTRKIND, &ti) == -1) {
        Con_DPrintf("ioctl cdromplaytrkind failed\n");
        return;
    }

    if (ioctl(_cdfile, CDROMRESUME) == -1)
        Con_DPrintf("ioctl cdromresume failed\n");

    _playLooping = looping;
    _playTrack = track;
    _playing = true;

    if (_cdvolume == 0.0)
        CDAudio_Pause();
}


void CDAudio_Stop() {
    if (((_cdfile == -1) || !_enabled) ||
        (!_playing))
        return;

    if (ioctl(_cdfile, CDROMSTOP) == -1)
        Con_DPrintf("ioctl cdromstop failed (%d)\n", errno);

    _wasPlaying = false;
    _playing = false;
}

void CDAudio_Pause() {
    if ((_cdfile == -1) || !_enabled)
        return;

    if (!_playing)
        return;

    if (ioctl(_cdfile, CDROMPAUSE) == -1)
        Con_DPrintf("ioctl cdrompause failed\n");

    _wasPlaying = _playing;
    _playing = false;
}


void CDAudio_Resume() {
    if ((_cdfile == -1) || !_enabled)
        return;

    if (!_cdValid)
        return;

    if (!_wasPlaying)
        return;

    if (ioctl(_cdfile, CDROMRESUME) == -1)
        Con_DPrintf("ioctl cdromresume failed\n");
    _playing = true;
}

static void CD_f() {
    cString command;
    int		ret;
    int		n;

    if (Cmd_Argc() < 2)
        return;

    command = Cmd_Argv(1);

    if (Q_strcasecmp(command, "on") == 0) {
        _enabled = true;
        return;
    }

    if (Q_strcasecmp(command, "off") == 0) {
        if (_playing)
            CDAudio_Stop();
        _enabled = false;
        return;
    }

    if (Q_strcasecmp(command, "reset") == 0) {
        _enabled = true;
        if (_playing)
            CDAudio_Stop();
        for (n = 0; n < 100; n++)
            _remap[n] = n;
        CDAudio_GetAudioDiskInfo();
        return;
    }

    if (Q_strcasecmp(command, "remap") == 0) {
        ret = Cmd_Argc() - 2;
        if (ret <= 0) {
            for (n = 1; n < 100; n++)
                if (_remap[n] != n)
                    Con_Printf("  %u -> %u\n", n, _remap[n]);
            return;
        }
        for (n = 1; n <= ret; n++)
            _remap[n] = Q_atoi(Cmd_Argv(n + 1));
        return;
    }

    if (Q_strcasecmp(command, "close") == 0) {
        CDAudio_CloseDoor();
        return;
    }

    if (!_cdValid) {
        CDAudio_GetAudioDiskInfo();
        if (!_cdValid) {
            Con_Printf("No CD in player.\n");
            return;
        }
    }

    if (Q_strcasecmp(command, "play") == 0) {
        CDAudio_Play((uint8_t)Q_atoi(Cmd_Argv(2)), false);
        return;
    }

    if (Q_strcasecmp(command, "loop") == 0) {
        CDAudio_Play((uint8_t)Q_atoi(Cmd_Argv(2)), true);
        return;
    }

    if (Q_strcasecmp(command, "stop") == 0) {
        CDAudio_Stop();
        return;
    }

    if (Q_strcasecmp(command, "pause") == 0) {
        CDAudio_Pause();
        return;
    }

    if (Q_strcasecmp(command, "resume") == 0) {
        CDAudio_Resume();
        return;
    }

    if (Q_strcasecmp(command, "eject") == 0) {
        if (_playing)
            CDAudio_Stop();
        CDAudio_Eject();
        _cdValid = false;
        return;
    }

    if (Q_strcasecmp(command, "info") == 0) {
        Con_Printf("%u tracks\n", _maxTrack);
        if (_playing)
            Con_Printf("Currently %s track %u\n", _playLooping ? "looping" : "playing", _playTrack);
        else if (_wasPlaying)
            Con_Printf("Paused %s track %u\n", _playLooping ? "looping" : "playing", _playTrack);
        Con_Printf("Volume is %f\n", _cdvolume);
        return;
    }
}

void CDAudio_Update() {
    if (!_enabled)
        return;

    if (bgmvolume.value != _cdvolume) {
        if (_cdvolume) {
            Cvar_SetValue("bgmvolume", 0.0);
            _cdvolume = bgmvolume.value;
            CDAudio_Pause();
        }
        else {
            Cvar_SetValue("bgmvolume", 1.0);
            _cdvolume = bgmvolume.value;
            CDAudio_Resume();
        }
    }

    static time_t lastchk;
    if (_playing && (lastchk < time(NULL))) {
        lastchk = time(NULL) + 2; //two seconds between chks
        struct cdrom_subchnl subchnl = { .cdsc_format = CDROM_MSF };
        if (ioctl(_cdfile, CDROMSUBCHNL, &subchnl) == -1) {
            Con_DPrintf("ioctl cdromsubchnl failed\n");
            _playing = false;
            return;
        }
        if ((subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY) &&
            (subchnl.cdsc_audiostatus != CDROM_AUDIO_PAUSED)) {
            _playing = false;
            if (_playLooping)
                CDAudio_Play(_playTrack, true);
        }
    }
}

int CDAudio_Init() {
    if ((cls.state == ca_dedicated) ||
        (COM_CheckParm("-nocdaudio")))
        return -1;

    int param = COM_CheckParm("-cddev");
    if ((param != 0) && (param < com_argc - 1)) {
        strncpy(_cd_dev, com_argv[param + 1], sizeof(_cd_dev));
        _cd_dev[sizeof(_cd_dev) - 1] = 0;
    }

    if ((_cdfile = open(_cd_dev, O_RDONLY)) == -1) {
        Con_Printf("CDAudio_Init: open of \"%s\" failed (%i)\n", _cd_dev, errno);
        _cdfile = -1;
        return -1;
    }

    for (int i = 0; i < 100; i++)
        _remap[i] = i;
    _initialized = true;
    _enabled = true;

    if (CDAudio_GetAudioDiskInfo()) {
        Con_Printf("CDAudio_Init: No CD in player.\n");
        _cdValid = false;
    }

    Cmd_AddCommand("cd", CD_f);

    Con_Printf("CD Audio Initialized\n");

    return 0;
}


void CDAudio_Shutdown() {
    if (!_initialized)
        return;
    CDAudio_Stop();
    close(_cdfile);
    _cdfile = -1;
}
