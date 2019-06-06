// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2019 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#ifndef __GLOBAL_H
#define __GLOBAL_H

#define debug_nlf(M, ...) printf("%s[%d] %s: " M, __FILE__, __LINE__, __func__,  ##__VA_ARGS__)
#define debug(M, ...) printf("%s[%d] %s: " M "\n", __FILE__, __LINE__, __func__,  ##__VA_ARGS__)

#ifdef __APPLE__
#include <machine/endian.h>
#define __LITTLE_ENDIAN	__DARWIN_LITTLE_ENDIAN
#define __BIG_ENDIAN	__DARWIN_BIG_ENDIAN
#define __PDP_ENDIAN	__DARWIN_PDP_ENDIAN
#define	__BYTE_ORDER	__DARWIN_BYTE_ORDER
#else
#include <endian.h>
#endif

#ifdef __BYTE_ORDER
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define WR2(p,v) { (p)[0] = v; (p)[1] = v >> 8; }
#define WR4(p,v) { (p)[0] = v; (p)[1] = v >> 8;  (p)[2] = v >> 16;  (p)[3] = v >> 24; }
#define RD2(p) ((p)[0] + ((p)[1] << 8));
#define RD4(p) ((p)[0] + ((p)[1] << 8) + ((p)[2] << 16) + ((p)[3] << 24));
#elif (__BYTE_ORDER == __BIG_ENDIAN)
#define WR2(p,v) { (p)[1] = v; (p)[0] = v >> 8; }
#define WR4(p,v) { (p)[3] = v; (p)[2] = v >> 8;  (p)[1] = v >> 16;  (p)[0] = v >> 24; }
#define RD2(p) ((p)[1] + ((p)[0] << 8));
#define RD4(p) ((p)[3] + ((p)[2] << 8) + ((p)[1] << 16) + ((p)[0] << 24));
#else
#error Byte order is not supported !
#endif
#else
#error Byte order is undefined !
#endif

#include "lfqueue.h"


enum // GLOBAL LIMITS
{
    NASECT = 4,
    NDIVIS = 8,
    NKEYBD = 6,
    NGROUP = 8,
    NNOTES = 61,
    NBANK  = 32,
    NPRES  = 32
};


#define MIDICTL_SWELL 7 //Division Swell
#define SWELL_MIN 0.0f
#define SWELL_MAX 1.0f
#define SWELL_DEF 1.0f

#define MIDICTL_TFREQ 12 //Division Trem freq
#define TFREQ_MIN 2.0f
#define TFREQ_MAX 8.0f
#define TFREQ_DEF 4.0f

#define MIDICTL_TMODD 13 //Division Trem amp
#define TMODD_MIN 0.0f
#define TMODD_MAX 0.6f
#define TMODD_DEF 0.3f

#define MIDICTL_BANK   32 //Preset bank select
#define MIDICTL_HOLD   64 //sustain
#define MIDICTL_IFELM  98 //stops control
#define MIDICTL_ASOFF 120 //all sound off
#define MIDICTL_ANOFF 123 //all notes off

#define MIDICTL_DAZIM 14 //Division Azimuth
#define DAZIM_MIN -0.5f
#define DAZIM_MAX 0.5f

#define MIDICTL_DWIDT 15 //Division Width
#define DWIDT_MIN 0.0f
#define DWIDT_MAX 1.0f

#define MIDICTL_DDIRE 16 //Division Direct
#define DDIRE_MIN 0.0f
#define DDIRE_MAX 1.0f

#define MIDICTL_DREFL 17 //Division Reflect
#define DREFL_MIN 0.0f
#define DREFL_MAX 1.0f

#define MIDICTL_DREVB 18 //Division Reverb
#define DREVB_MIN 0.0f
#define DREVB_MAX 1.0f

#define MIDICTL_RDELY 20 //Reverb delay
#define RDELY_MIN 0.025f
#define RDELY_MAX 0.150f

#define MIDICTL_RTIME 21 //Reverb time
#define RTIME_MIN 2.0f
#define RTIME_MAX 7.0f

#define MIDICTL_RPOSI 22 //Reverb position
#define RPOSI_MIN -1.0f
#define RPOSI_MAX 1.0f

#define MIDICTL_MAVOL 23 //Master volume
#define MAVOL_MIN 0.0f
#define MAVOL_MAX 1.0f

#define MIDICTL_CANCL 28 //Cancel
#define MIDICTL_CANCL_VAL 127
#define MIDICTL_TUTTI 29 //Tutti
#define MIDICTL_TUTTI_OFF_VAL 0
#define MIDICTL_TUTTI_ON_VAL 127
#define MIDICTL_TRNSP 30 //Transpose
#define MIDICTL_CRESC 31 //Crescendo
#define MIDICTL_PBANK 32 //Preset Bank Select
#define MIDICTL_PRCAL 33 //Preset recall
#define MIDICTL_PSTOR 34 //Preset store
#define MIDICTL_PPREV 35 //Preset previous
#define MIDICTL_PNEXT 36 //Preset next
#define MIDICTL_DRCAL 37 //Division recall

// audio thread
#define AU_PARAM 18
#define AU_PARAM_TRANSPOSE 1

#define KEYS_MASK 63
#define HOLD_MASK 64
#define ALL_MASK 127

class Fparm
{
public:

    float  _val;
    float  _min;
    float  _max;
};


#endif

