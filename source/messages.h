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


#ifndef __MESSAGES_H
#define __MESSAGES_H

#include <string.h>
#include <alsa/asoundlib.h>
#include "rankwave.h"
#include "asection.h"
#include "addsynth.h"
#include "global.h"

enum
{
    FM_SLAVE =  8,
    FM_IFACE =  9,
    FM_MODEL = 10,
    FM_IMIDI = 11,
    FM_AUDIO = 12,
    FM_TXTIP = 13,
    TO_SLAVE =  8,
    TO_IFACE =  9,
    TO_MODEL = 10,
    TO_IMIDI = 11,
    TO_AUDIO = 12,
    EV_RLINE = 0,
    EV_XWIN  = 16,
    EV_QMIDI = 24,
    EV_SYNC  = 30,
    EV_EXIT  = 31
};


enum
{
    MT_AUDIO_INFO,
    MT_AUDIO_SYNC,
    MT_MIDI_INFO,
    MT_NEW_DIVIS,
    MT_CALC_RANK,
    MT_LOAD_RANK,
    MT_SAVE_RANK,

    MT_IFC_INIT,
    MT_IFC_READY,
    MT_IFC_ELCLR,   // must be in this order : disable a stop
    MT_IFC_ELSET,   // enable a stop 
    MT_IFC_ELXOR,   // toggle a stop (if we send 1) ?? TBC
    MT_IFC_ELATT,   //
    MT_IFC_GRCLR,   // disable all stop of a specific group
    MT_IFC_TUTI,    // enable all stops
    MT_IFC_TRNSP,   // refresh GUI transpose value
    MT_IFC_AUPAR,   // change an audio element value
    MT_IFC_DIPAR,   // ?? instrument parameters
    MT_IFC_RETUNE,  // retune organ
    MT_IFC_ANOFF,   // all notes off
    MT_IFC_MCSET,   // set midi configuration
    MT_IFC_MCGET,   // get midi configuration
    MT_IFC_PRRCL,   // recall a preset
    MT_IFC_PRDEC,   // decrement preset
    MT_IFC_PRINC,   // increment preset
    MT_IFC_PRSTO,   // store preset
    MT_IFC_PRINS,   // insert preset
    MT_IFC_PRDEL,   // delete preset
    MT_IFC_PRGET,   // get a preset state ?? TBC
    MT_IFC_TRNSPDEC,// increment transpose
    MT_IFC_TRNSPINC,// decrement transpose
    MT_IFC_EDIT,    // open pipe editor
    MT_IFC_APPLY,
    MT_IFC_SAVE,    // save current settings to file
    MT_IFC_TXTIP
};


#define SRC_GUI_DRAG  100
#define SRC_GUI_DONE  101
#define SRC_MIDI_PAR  200


class M_audio_info : public ITC_mesg
{
public:

    M_audio_info (void) : ITC_mesg (MT_AUDIO_INFO) {}

    float           _fsamp;
    int             _fsize;
    int             _nasect;
    Fparm          *_instrpar;
    Fparm          *_asectpar [NASECT];
};


class M_midi_info : public ITC_mesg
{
public:

    M_midi_info (void) : ITC_mesg (MT_MIDI_INFO) {}

    int       _client;
    int       _ipport;
    int       _opport;
    snd_seq_t *_seq;
    uint16_t  _chbits [16];
};


class M_new_divis : public ITC_mesg
{
public:

    M_new_divis (void) : ITC_mesg (MT_NEW_DIVIS) {}

    int             _flags;
    int             _dmask;
    int             _asect;
    float           _swell;
    float           _tfreq;
    float           _tmodd;
};


class M_def_rank : public ITC_mesg
{
public:

    M_def_rank (int type) : ITC_mesg (type) {}

    int             _divis;
    int             _rank;
    int             _group;
    int             _ifelm;
    float           _fsamp;
    float           _fbase;
    float          *_scale;
    Addsynth       *_sdef;
    Rankwave       *_wave;
    const char     *_path;
};


class M_ifc_init : public ITC_mesg
{
public:

    M_ifc_init (void) : ITC_mesg (MT_IFC_INIT) {}

    const char         *_stops;
    const char         *_waves;
    const char         *_instr;
    const char         *_appid;
    int                 _client;
    int                 _ipport;
    int                 _opport;
    snd_seq_t          *_seq;
    int                 _nasect;
    int                 _nkeybd;
    int                 _ndivis;
    int                 _ngroup;
    int                 _ntempe;
    struct
    {
        const char     *_label;
        int             _flags;
    }                   _keybdd [NKEYBD];
    struct
    {
        const char     *_label;
        int             _asect;
        int             _flags;
    }                   _divisd [NDIVIS];
    struct
    {
        const char     *_label;
        int             _nifelm;
        struct
        {
            const char *_label;
            const char *_mnemo;
            int         _type;
        }               _ifelmd [32];
    }                   _groupd [8];
    struct
    {
        const char     *_label;
        const char     *_mnemo;
    }                   _temped [16];
};


class M_ifc_ifelm : public ITC_mesg
{
public:

    M_ifc_ifelm (int type, int g, int i) :
        ITC_mesg (type),
        _group (g),
        _ifelm (i)
    {}

    int      _group;
    int      _ifelm;
};


class M_ifc_tutti : public ITC_mesg
{
public:

    M_ifc_tutti (bool s) :
        ITC_mesg (MT_IFC_TUTI),
        _state (s)
    {}

    bool     _state;
};


class M_ifc_transpose : public ITC_mesg
{
public:

    M_ifc_transpose (int v) :
        ITC_mesg (MT_IFC_TRNSP),
        _transpose (v)
    {}

    int      _transpose;
};


class M_ifc_aupar : public ITC_mesg
{
public:

    M_ifc_aupar (int s, int a, int p, float v) :
        ITC_mesg (MT_IFC_AUPAR),
        _srcid (s),
        _asect (a),
        _parid (p),
        _value (v)
    {}

    int    _srcid;
    int    _asect;
    int    _parid;
    float  _value;
};


class M_ifc_dipar : public ITC_mesg
{
public:

    M_ifc_dipar (int s, int d, int p, float v) :
        ITC_mesg (MT_IFC_DIPAR),
        _srcid (s),
        _divis (d),
        _parid (p),
        _value (v)
    {}

    int    _srcid;
    int    _divis;
    int    _parid;
    float  _value;
};


class M_ifc_retune : public ITC_mesg
{
public:

    M_ifc_retune (float f, int t) :
        ITC_mesg (MT_IFC_RETUNE),
        _freq (f),
        _temp (t)
    {}

    float  _freq;
    int    _temp;
};


class M_ifc_anoff : public ITC_mesg
{
public:

    M_ifc_anoff (int bits) :
        ITC_mesg (MT_IFC_ANOFF),
        _bits (bits)
    {
    }

    int  _bits;
};


class M_ifc_chconf : public ITC_mesg
{
public:

    M_ifc_chconf (int type, int index, uint16_t *bits) :
        ITC_mesg (type),
        _index (index)
    {
        if (bits) memcpy (_bits, bits, 16 * sizeof (uint16_t));
        else      memset (_bits, 0, 16 * sizeof (uint16_t));
    }

    int       _index;
    uint16_t  _bits [16];
};


class M_ifc_preset : public ITC_mesg
{
public:

    M_ifc_preset (int type, int bank, int pres, int stat, uint32_t *bits) :
        ITC_mesg (type),
        _bank (bank),
        _pres (pres),
        _stat (stat)
    {
        if (bits) memcpy (_bits, bits, NGROUP * sizeof (uint32_t));
        else      memset (_bits, 0, NGROUP * sizeof (uint32_t));
    }

    int       _bank;
    int       _pres;
    int       _stat;
    uint32_t  _bits [NGROUP];
};


class M_ifc_edit : public ITC_mesg
{
public:

    M_ifc_edit (int type, int group, int ifelm, Addsynth *synth) :
        ITC_mesg (type),
        _group (group),
        _ifelm (ifelm),
        _synth (synth)
    {}

    int        _group;
    int        _ifelm;
    Addsynth  *_synth;
};


class M_ifc_txtip : public ITC_mesg
{
public:

    M_ifc_txtip (void) :
        ITC_mesg (MT_IFC_TXTIP),
        _line (0)
    {}

    char  *_line;
};


#endif

