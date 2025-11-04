#pragma once

#include "platformdefs.h"
#include "zone.h"
#include "vector.h"
// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct {
    int left;
    int right;
} portable_samplepair_t;

typedef struct sfx_s {
    char        name[MAX_QPATH];
    CacheUser_t cache;
} sfx_t;
typedef sfx_t* sfx_p;


// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct {
    int length;
    int loopstart;
    int speed;
    int width;
    int stereo;
    uint8_t data[1];  // variable sized
} sfxcache_t;
typedef sfxcache_t* sfxcache_p;

typedef struct {
    bool    gamealive;
    bool    soundalive;
    bool    splitbuffer;
    int     channels;
    int     samples;    // mono samples in buffer
    int     submission_chunk;  // don't mix less than this #
    int     samplepos;    // in mono samples
    int     samplebits;
    int     speed;
    uint8_p buffer;
} dma_t;
typedef dma_t* dma_p;


// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct {
    sfx_p   sfx;   // sfx number
    int     leftvol;  // 0-255 volume
    int     rightvol;  // 0-255 volume
    int     end;   // end time in global paintsamples
    int     pos;   // sample position in sfx
    int     looping;  // where to loop, -1 = no looping
    int     entnum;   // to allow overriding a specific sound
    int     entchannel;  //
    vec3_t  origin;   // origin of sound effect
    vec_t   dist_mult;  // distance multiplier (attenuation/clipK)
    int     master_vol;  // 0-255 master volume
} channel_t;
typedef channel_t* channel_p;

typedef struct {
    int rate;
    int width;
    int channels;
    int loopstart;
    int samples;
    int dataofs;  // chunk starts this many bytes from file start
} wavinfo_t;
