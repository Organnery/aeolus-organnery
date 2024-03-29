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


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "model.h"
#include "scales.h"
#include "global.h"


Divis::Divis (void) :
    _flags (0),
    _dmask (0),
    _nrank (0)
{
    *_label = 0;
    _param [SWELL]._val = SWELL_DEF;
    _param [SWELL]._min = SWELL_MIN;
    _param [SWELL]._max = SWELL_MAX;
    _param [TFREQ]._val = TFREQ_DEF;
    _param [TFREQ]._min = TFREQ_MIN;
    _param [TFREQ]._max = TFREQ_MAX;
    _param [TMODD]._val = TMODD_DEF;
    _param [TMODD]._min = TMODD_MIN;
    _param [TMODD]._max = TMODD_MAX;
}



Keybd::Keybd (void) :
    _flags (0)
{
    *_label = 0;
}


Ifelm::Ifelm (void) :
    _state (0)
{
    *_label = 0;
    *_mnemo = 0;
}


Group::Group (void) :
    _nifelm (0)
{
    *_label = 0;
}


Model::Model (Lfq_u32      *qcomm,
              Lfq_u8       *qmidi,
          uint16_t     *midimap,
              const char   *appname,
              const char   *stops,
              const char   *instr,
              const char   *waves,
              bool          uhome) :
    A_thread ("Model"),
    _qcomm (qcomm),
    _qmidi (qmidi),
    _midimap (midimap),
    _appname (appname),
    _stops (stops),
    _uhome (uhome),
    _ready (false),
    _tutti (false),
    _transpose (64),
    _nasect (0),
    _ndivis (0),
    _nkeybd (0),
    _ngroup (0),
    _count (0),
    _bank (0),
    _pres (0),
    _sc_cmode (0),
    _sc_group (0),
    _audio (0),
    _midi (0)
{
    sprintf (_instr, "%s/%s", stops, instr);
    sprintf (_waves, "%s/%s", stops, waves);
    memset (_midimap, 0, 16 * sizeof (uint16_t));
    memset (_preset, 0, NBANK * NPRES * sizeof (Preset *));
    memset (_tutti_prev, 0, sizeof (_tutti_prev));
}


Model::~Model (void)
{
    if (_audio) _audio->recover ();
    if (_midi)  _midi->recover ();
}


void Model::thr_main (void)
{
    int E;

    init ();
    set_time (0);
    inc_time (100000);
    while ((E = get_event_timed ()) != EV_EXIT)
    {
        switch (E)
        {
        case FM_AUDIO:
        case FM_IMIDI:
        case FM_SLAVE:
        case FM_IFACE:
            proc_mesg (get_message ());
        break;

        case EV_TIME:
            inc_time (50000);
            proc_qmidi ();
            break;

        case EV_QMIDI:
            proc_qmidi ();
            break;

        default:
            ;
        }
    }
    fini ();
    send_event (EV_EXIT, 1);
}


void Model::init (void)
{
    // cannot call here, file must be read once interface has complete
    //read_aparams ();
    read_instr ();
    read_presets ();
}


void Model::fini (void)
{
    // write_presets (); // disable save on exit
}


void Model::proc_mesg (ITC_mesg *M)
{
    // Handle commands from other threads, including
    // the user interface.
    int i;

    switch (M->type ())
    {
    case MT_IFC_ELCLR:
    case MT_IFC_ELSET:
    case MT_IFC_ELXOR:
    {
    // Set, reset or toggle a stop.
        M_ifc_ifelm *X = (M_ifc_ifelm *) M;
        set_ifelm (X->_group, X->_ifelm, X->type () - MT_IFC_ELCLR);
        break;
    }
    case MT_IFC_ELATT:
    {
        send_event (TO_IFACE, M);
        M = 0;
        break;
    }
    case MT_IFC_GRCLR:
    {
        // Reset a group of stops.
        M_ifc_ifelm *X = (M_ifc_ifelm *) M;
        clr_group (X->_group);
        break;
    }
    case MT_IFC_TUTI:
    {
        // Enable|Restore a group of stops, excluding Tremulant & Couplers.
        M_ifc_tutti *X = (M_ifc_tutti *) M;
        _tutti = X->_state;
        tutti ();

        // output tutti midi message on first enabled Control channel
        for (i = 0; i < 16; i++) {
            if ((_midimap[i] & 0x4000) == 0x4000) {
            if (_tutti)
                midi_tx_cc(i, MIDICTL_TUTTI, MIDICTL_TUTTI_ON_VAL);
            else
                midi_tx_cc(i, MIDICTL_TUTTI, MIDICTL_TUTTI_OFF_VAL);
            break;
            }
        }
        break;
    }
    case MT_IFC_TRNSPDEC:
        // Decrement transpose.
        if (_transpose > 52)
            set_transpose(SRC_GUI_DONE, _transpose - 1);
        break;
    case MT_IFC_TRNSPINC:
        // Increment transpose.
        if (_transpose < 76)
            set_transpose(SRC_GUI_DONE, _transpose + 1);
        break;
    case MT_IFC_AUPAR:
    {
        // Set audio section parameter.
        M_ifc_aupar *X = (M_ifc_aupar *) M;
        set_aupar (X->_srcid, X->_asect, X->_parid, X->_value);
        break;
    }
    case MT_IFC_DIPAR:
    {
        // Set division parameter.
        M_ifc_dipar *X = (M_ifc_dipar *) M;
        set_dipar (X->_srcid, X->_divis, X->_parid, X->_value);
        break;
    }
    case MT_IFC_RETUNE:
    {
        // Recalculate all wavetables.
        M_ifc_retune *X = (M_ifc_retune *) M;
        retune (X->_freq, X->_temp);
        break;
    }
    case MT_IFC_ANOFF:
    {
        // All notes off.
        M_ifc_anoff *X = (M_ifc_anoff *) M;
        midi_off (X->_bits);
        break;
    }
    case MT_IFC_MCSET:
    {
        // Store midi routing preset.
        int index;
        M_ifc_chconf *X = (M_ifc_chconf *) M;
        index = X->_index;
        if (index >= 0)
        {
            if (index >= 8) break;
            memcpy (_chconf [X->_index]._bits, X->_bits, 16 * sizeof (uint16_t));
        }
        set_mconf (X->_index, X->_bits);
    }
    case MT_IFC_MCGET:
    {
       // Recall midi routing preset.
        int index;
        M_ifc_chconf *X = (M_ifc_chconf *) M;
        index = X->_index;
        if (index >= 0)
        {
            if (index >= 8) break;
            set_mconf (X->_index, _chconf [X->_index]._bits);
        }
        break;
    }
    case MT_IFC_PRRCL:
    {
        // Load a preset.
        M_ifc_preset *X = (M_ifc_preset *) M;
        if (X->_bank < 0) X->_bank = _bank;
        set_state (X->_bank, X->_pres);
        break;
    }
    case MT_IFC_PRDEC:
    {
        // Decrement preset and load.
        if (_pres > 0) set_state (_bank, --_pres);
        break;
    }
    case MT_IFC_PRINC:
    {
        // Increment preset and load.
        if (_pres < NPRES - 1) set_state (_bank, ++_pres);
        break;
    }
    case MT_IFC_PRSTO:
    {
        // Store a preset.
        M_ifc_preset  *X = (M_ifc_preset *) M;
        uint32_t       d [NGROUP];
        get_state (d);
        set_preset (X->_bank, X->_pres, d);
        write_presets ();
        break;
    }
    case MT_IFC_PRINS:
    {
        // Insert a preset.
        M_ifc_preset *X = (M_ifc_preset *) M;
        uint32_t     d [NGROUP];
        get_state (d);
        ins_preset (X->_bank, X->_pres, d);
        write_presets ();
        break;
    }
    case MT_IFC_PRDEL:
    {
        // Delete a preset.
        M_ifc_preset *X = (M_ifc_preset *) M;
        del_preset (X->_bank, X->_pres);
        write_presets ();
        break;
    }
    case MT_IFC_PRGET:
    {
        // Read a preset.
        M_ifc_preset *X = (M_ifc_preset *) M;
        X->_stat = get_preset (X->_bank, X->_pres, X->_bits);
        send_event (TO_IFACE, M);
        M = 0;
        break;
    }
    case MT_IFC_EDIT:
    {
        // Start editing a stop.
        M_ifc_edit *X = (M_ifc_edit *) M;
        Rank       *R = find_rank (X->_group, X->_ifelm);
        if (_ready && R)
        {
            X->_synth = R->_sdef;
            send_event (TO_IFACE, M);
            M = 0;
        }
        break;
    }
    case MT_IFC_APPLY:
    {
        // Apply edited stop.
        M_ifc_edit *X = (M_ifc_edit *) M;
        if (_ready) recalc (X->_group, X->_ifelm);
        break;
    }
    case MT_IFC_SAVE:
        // Save presets, midi presets, and wavetables.
        save ();
        break;

    case MT_LOAD_RANK:
    case MT_CALC_RANK:
    {
        // Load a rank into a division.
        M_def_rank *X = (M_def_rank *) M;
        _divis [X->_divis]._ranks [X->_rank]._wave = X->_wave;
        break;
    }
    case MT_AUDIO_INFO:
        // Initialisation info from audio thread.
        _audio = (M_audio_info *) M;
        M = 0;
        if (_midi)
        {
            init_audio ();
            init_iface ();
            init_ranks (MT_LOAD_RANK);
        }
        break;

    case MT_MIDI_INFO:
        // Initialisation info from midi thread.
        _midi = (M_midi_info *) M;
        M = 0;
        debug("midi initialised");
        if (_audio)
        {
            init_audio ();
            init_iface ();
            init_ranks (MT_LOAD_RANK);
        }
        break;

    case MT_AUDIO_SYNC:
        // Wavetable calculation done.
        send_event (TO_IFACE, new ITC_mesg (MT_IFC_READY));

        // Set initial transpose value.
        set_transpose(SRC_GUI_DONE, 64);

        // everything is ready.. send midi settings request
        debug("wavetable calculation done");

        // read audio parameters from saved file
        read_aparams ();

        uint8_t buf[9];
        buf[0] = 0xf0;
        buf[1] = 0x00;
        buf[2] = 0x21;
        buf[3] = 0x7f;
        buf[4] = 0x00;
        buf[5] = 0x1d;
        buf[6] = 0x00;
        buf[7] = 0x00;
        buf[8] = 0xf7;
        midi_tx_sysex(9, buf);

        _ready = true;
        break;

    default:
        fprintf (stderr, "Model: unexpected message, type = %ld\n", M->type ());
    }
    if (M) M->recover ();
}


void Model::proc_qmidi (void)
{
    int c, d, p, t, v;
    int g;

    // Handle commands from the qmidi queue. These are coming
    // from either the midi thread (ALSA), or the audio thread
    // (JACK). They are encoded as raw MIDI, except that all
    // messages are 3 bytes. All command have already been
    // checked at the sending side.

    while (_qmidi->read_avail () >= 3)
    {
        t = _qmidi->read (0);
        p = _qmidi->read (1);
        v = _qmidi->read (2);
        _qmidi->read_commit (3);
        c = t & 0x0F;
        d = (_midimap [c] >>  8) & 7;
        switch (t & 0xF0)
        {
        case 0x90:
            // Program change from notes 24..33.
            set_state (_bank, p - 24);
            break;

        case 0xB0:
            // Controllers.
            switch (p)
            {
            case MIDICTL_MAVOL:
            // Volume control
            set_aupar(SRC_MIDI_PAR, -1, 0, MAVOL_MIN + v * (MAVOL_MAX - MAVOL_MIN) / 127.0f);
            break;
            case MIDICTL_RDELY:
            // Reverb delay
            set_aupar(SRC_MIDI_PAR, -1, 1, RDELY_MIN + v * (RDELY_MAX - RDELY_MIN) / 127.0f);
            break;
            case MIDICTL_RTIME:
            // Reverb time
            set_aupar(SRC_MIDI_PAR, -1, 2, RTIME_MIN + v * (RTIME_MAX - RTIME_MIN) / 127.0f);
            break;
            case MIDICTL_RPOSI:
            // Reverb position
            set_aupar(SRC_MIDI_PAR, -1, 3, RPOSI_MIN + v * (RPOSI_MAX - RPOSI_MIN) / 127.0f);
            break;
            case MIDICTL_DAZIM:
            // Division azimuth
            set_aupar(SRC_MIDI_PAR, c, 0, DAZIM_MIN + v * (DAZIM_MAX - DAZIM_MIN) / 127.0f);
            break;
            case MIDICTL_DWIDT:
            // Division width
            set_aupar(SRC_MIDI_PAR, c, 1, DWIDT_MIN + v * (DWIDT_MAX - DWIDT_MIN) / 127.0f);
            break;
            case MIDICTL_DDIRE:
            // Division direct
            set_aupar(SRC_MIDI_PAR, c, 2, DDIRE_MIN + v * (DDIRE_MAX - DDIRE_MIN) / 127.0f);
            break;
            case MIDICTL_DREFL:
            // Division reflect
            set_aupar(SRC_MIDI_PAR, c, 3, DREFL_MIN + v * (DREFL_MAX - DREFL_MIN) / 127.0f);
            break;
            case MIDICTL_DREVB:
            // Division reverb
            set_aupar(SRC_MIDI_PAR, c, 4, DREVB_MIN + v * (DREVB_MAX - DREVB_MIN) / 127.0f);
            break;
            case MIDICTL_SWELL:
            // Swell pedal
            set_dipar (SRC_MIDI_PAR, d, 0, SWELL_MIN + v * (SWELL_MAX - SWELL_MIN) / 127.0f);
            break;
            case MIDICTL_TFREQ:
            // Tremulant frequency
            set_dipar (SRC_MIDI_PAR, d, 1, TFREQ_MIN + v * (TFREQ_MAX - TFREQ_MIN) / 127.0f);
            break;
            case MIDICTL_TMODD:
            // Tremulant amplitude
            set_dipar (SRC_MIDI_PAR, d, 2, TMODD_MIN + v * (TMODD_MAX - TMODD_MIN) / 127.0f);
            break;
            case MIDICTL_BANK:
            // Preset bank.
            if (v < NBANK) {
                _bank = v;
                set_state (_bank, _pres);
            }
            break;
            
            case MIDICTL_PNEXT:
            // Increment preset and load.
            if (_pres < NPRES - 1) set_state (_bank, ++_pres);
            break;

            case MIDICTL_PPREV:
            // Decrement preset and load.
            if (_pres > 0) set_state (_bank, --_pres);
                break;

            case MIDICTL_PSTOR:
            // Store a preset.
            debug("storing current stops in bank=%d, preset=%d", _bank, _pres);
            uint32_t d [NGROUP];
            get_state (d);
            set_preset (_bank, _pres, d);
            write_presets ();
            break;

            case MIDICTL_CANCL:
            // Cancel.
            for (g = 0; g < _ngroup; g++)
                clr_group (SRC_MIDI_PAR, g);
            break;

            case MIDICTL_TUTTI:
            // Tutti.
            if (/*v == MIDICTL_TUTTI_OFF_VAL && */_tutti)
            {
                _tutti = false;
                tutti ();
            }
            else if (/*v == MIDICTL_TUTTI_ON_VAL && */!_tutti)
            {
                _tutti = true;
                tutti ();
            }
            break;

            case MIDICTL_TRNSP:
            // Transpose.
            set_transpose (SRC_MIDI_PAR, v);
            break;

            case MIDICTL_IFELM:
            // Stop control.
            if (v & 64)
            {
            // Set mode or clear group.
                    _sc_cmode = (v >> 4) & 3;
                    _sc_group = v & 7;
                    if (_sc_cmode == 0) clr_group (_sc_group);
            }
            else if (_sc_cmode)
            {
                // Set, reset or toggle stop.
                        set_ifelm (_sc_group, v & 31, _sc_cmode - 1);
            }
            break;
            }
        break;

        case 0xC0:
            // Program change.
            if (p < NPRES) set_state (_bank, p);
            break;
        }
    }
}


void Model::init_audio (void)
{
    int          d;
    Divis        *D;
    M_new_divis  *M;

    for (d = 0, D = _divis; d < _ndivis; d++, D++)
    {
        M = new M_new_divis ();
        M->_flags = D->_flags;
        M->_dmask = D->_dmask;
        M->_asect = D->_asect;
        M->_swell = D->_param [Divis::SWELL]._val;
        M->_tfreq = D->_param [Divis::TFREQ]._val;
        M->_tmodd = D->_param [Divis::TMODD]._val;
        send_event (TO_AUDIO, M);
    }
}


void Model::init_iface (void)
{
    int          i, j;
    M_ifc_init   *M;
    Keybd        *K;
    Divis        *D;
    Group        *G;

    M = new M_ifc_init;
    M->_stops  = _stops;
    M->_waves  = _waves;
    M->_instr  = _instr;
    M->_appid  = _appname;
    M->_client = _midi->_client;
    M->_ipport = _midi->_ipport;
    M->_opport = _midi->_opport;
    M->_seq    = _midi->_seq;
    M->_nasect = _nasect;
    M->_nkeybd = _nkeybd;
    M->_ndivis = _ndivis;
    M->_ngroup = _ngroup;
    M->_ntempe = NSCALES;
    for (i = 0; i < NKEYBD; i++)
    {
        K = _keybd + i;
        M->_keybdd [i]._label = K->_label;
        M->_keybdd [i]._flags = K->_flags;

        if (i < _nkeybd) debug("_keybdd[%d] _label='%s' _flags=0x%x", i, K->_label, K->_flags);
    }
    for (i = 0; i < NDIVIS; i++)
    {
        D = _divis + i;
        M->_divisd [i]._label = D->_label;
        M->_divisd [i]._flags = D->_flags;
        M->_divisd [i]._asect = D->_asect;

        if (i < _ndivis) debug("_divisd[%d] _label='%s' _flags=0x%x _asect=0x%x", i, D->_label, D->_flags, D->_asect);
    }
    for (i = 0; i < NGROUP; i++)
    {
        G = _group + i;
        M->_groupd [i]._label  = G->_label;
        M->_groupd [i]._nifelm = G->_nifelm;
        if (i < _ngroup) debug("_groupd[%d] _label='%s' _nifelm=%d", i, G->_label, G->_nifelm);

        for (j = 0; j < G->_nifelm; j++)
        {
            M->_groupd [i]._ifelmd [j]._label = G->_ifelms [j]._label;
            M->_groupd [i]._ifelmd [j]._mnemo = G->_ifelms [j]._mnemo;
            M->_groupd [i]._ifelmd [j]._type  = G->_ifelms [j]._type;

            debug("\tifelmd[%d] _label='%s' _mnemo=%s _type=0x%x", j, G->_ifelms[j]._label, G->_ifelms[j]._mnemo, G->_ifelms[j]._type);
        }
    }
    for (i = 0; i < NSCALES; i++)
    {
        M->_temped [i]._label = scales [i]._label;
        M->_temped [i]._mnemo = scales [i]._mnemo;
        debug("_temped[%d] _label='%s' _mnemo='%s'", i, scales[i]._label, scales[i]._mnemo);
    }
    send_event (TO_IFACE, M);

    for (j = 0; j < 4; j++)
    {
        send_event (TO_IFACE, new M_ifc_aupar (0, -1, j, _audio->_instrpar [j]._val));
    }
    for (i = 0; i < _nasect; i++)
    {
        for (j = 0; j < 5; j++)
        {
            send_event (TO_IFACE, new M_ifc_aupar (0, i, j, _audio->_asectpar [i][j]._val));
        }
    }
    for (i = 0; i < _ndivis; i++)
    {
        for (j = 0; j < 3; j++)
        {
            send_event (TO_IFACE, new M_ifc_dipar (0, i, j, _divis [i]._param [j]._val));
        }
    }
    set_mconf (0, _chconf [0]._bits);
}


void Model::init_ranks (int comm)
{
    int    g, i;
    Group  *G;

    _count++;
    _ready = false;
    send_event (TO_IFACE, new M_ifc_retune (_fbase, _itemp));

    for (g = 0; g < _ngroup; g++)
    {
        G = _group + g;
        for (i = 0; i < G->_nifelm; i++) proc_rank (g, i, comm);
    }
    send_event (TO_SLAVE, new ITC_mesg (MT_AUDIO_SYNC));
}


void Model::proc_rank (int g, int i, int comm)
{
    int         d, r;
    M_def_rank  *M;
    Ifelm       *I;
    Rank        *R;

    I = _group [g]._ifelms + i;
    if ((I->_type == Ifelm::DIVRANK) || (I->_type == Ifelm::KBDRANK))
    {
        d = (I->_action0 >> 16) & 255;
        r = (I->_action0 >>  8) & 255;
        R = _divis [d]._ranks + r;
        if (comm == MT_SAVE_RANK)
        {
            if (R->_wave->modif ())
            {
                M = new M_def_rank (comm);
                M->_fsamp = _audio->_fsamp;
                M->_fbase = _fbase;
                M->_scale = scales [_itemp]._data;
                M->_sdef  = R->_sdef;
                M->_wave  = R->_wave;
                M->_path  = _waves;
                send_event (TO_SLAVE, M);
            }
        }
        else if (R->_count != _count)
        {
            R->_count = _count;
            M = new M_def_rank (comm);
            M->_divis = d;
            M->_rank  = r;
            M->_group = g;
            M->_ifelm = i;
            M->_fsamp = _audio->_fsamp;
            M->_fbase = _fbase;
            M->_scale = scales [_itemp]._data;
            M->_sdef  = R->_sdef;
            M->_wave  = R->_wave;
            M->_path  = _waves;
            send_event (TO_SLAVE, M);
        }
    }
}


void Model::set_ifelm (int g, int i, int m)
{
    int    s, j;
    Ifelm  *I;
    Group  *G;

    G = _group + g;
    if ((! _ready) || (g >= _ngroup) || (i >= G->_nifelm)) return;
    I = G->_ifelms + i;
    s = (m == 2) ? I->_state ^ 1 : m;
    if (I->_state != s)
    {
        I->_state = s;

        // output midi note message on ifelm change
        debug("g=%d, i=%d, m=%d, s=%d", g, i, m, s);
        for (j = 0; j < 16; j++) {
            if ((g == 0 && (_midimap[j] & 0x104F) == 0x1001) ||
                (g == 1 && (_midimap[j] & 0x104F) == 0x1002) ||
                (g == 2 && (_midimap[j] & 0x104F) == 0x1004) ||
                (g == 3 && (_midimap[j] & 0x104F) == 0x1048)) {
            midi_tx_note(j, i, 127, s==1);
            break;
            }
        }

        if (_qcomm->write_avail ())
        {
            _qcomm->write (0, s ? I->_action1 : I->_action0);
            _qcomm->write_commit (1);
            send_event (TO_IFACE, new M_ifc_ifelm (MT_IFC_ELCLR + s, g, i));
        }
    }
}


void Model::clr_group (int s, int g)
{
    int     i;
    Ifelm  *I;
    Group  *G;
    bool    clr_tutti = false;

    G = _group + g;
    if ((! _ready) || (g >= _ngroup)) return;

    if (g == 0) {
        // cancel Tutti state
        if (_tutti) {
            // update gui with new tutti state
            _tutti = false;
            clr_tutti = true;
            send_event (TO_IFACE, new M_ifc_tutti (_tutti));
        }

        // output cancel midi message on first enabled Control channel
        for (i = 0; i < 16; i++) {
            if ((_midimap[i] & 0x4000) == 0x4000) {
            if (s != SRC_MIDI_PAR)
                midi_tx_cc(i, MIDICTL_CANCL, MIDICTL_CANCL_VAL);

            // output clear tutti message
            if (clr_tutti)
                midi_tx_cc(i, MIDICTL_TUTTI, MIDICTL_TUTTI_OFF_VAL);
            break;
            }
        }
    }

    for (i = 0; i < G->_nifelm; i++)
    {
        I = G->_ifelms + i;
        if (I->_state)
        {
            I->_state = 0;
            if (_qcomm->write_avail ())
            {
               _qcomm->write (0, I->_action0);
               _qcomm->write_commit (1);
            }
        }
    }
    send_event (TO_IFACE, new M_ifc_ifelm (MT_IFC_GRCLR, g, 0));
}


void Model::clr_group (int g)
{
    clr_group(SRC_GUI_DONE, g);
}


void Model::tutti ()
{
    // enables all stops on all groups
    int     g, i;
    Ifelm  *I;
    Group  *G;

    if (!_ready) return;

    for (g = 0; g < _ngroup; g++)
    {
        G = _group + g;

        for (i = 0; i < G->_nifelm; i++)
        {
            I = G->_ifelms + i;
            debug("felm g=%d, i=%d, I->_state=%d, prev_state=%d, I->_type=%d", g, i, I->_state, _tutti_prev[g][i], I->_type);

            if (_tutti) {
                // store current stop state
                _tutti_prev[g][i] = I->_state;

                // enable stop
                if (I->_state == 0 && I->_type == 0)
                    set_ifelm (g, i, 1);
            }
            else {
                // restore stop state
                if (I->_type == 0) {
                    set_ifelm (g, i, _tutti_prev[g][i]);
                }
            }
        }
    }
    // update gui with new tutti state
    send_event (TO_IFACE, new M_ifc_tutti (_tutti));
}


void Model::get_state (uint32_t *d)
{
    int    g, i;
    uint32_t    s;
    Group  *G;
    Ifelm  *I;

    for (g = 0; g < _ngroup; g++)
    {
        G = _group + g;
        s = 0;
        for (i = 0; i < G->_nifelm; i++)
        {
            I = G->_ifelms + i;
            if (I->_state & 1) s |= 1 << i;
        }
        *d++ = s;
    }
}


void Model::set_state (int bank, int pres)
{
    int    g, i;
    uint32_t    d [NGROUP], s;
    Group  *G;

    _bank = bank;
    _pres = pres;
    if (get_preset (bank, pres, d))
    {
        // clear tutti if enabled
        if (_tutti)
        {
            _tutti = false;
            tutti ();

            // output clear tutti midi message on first enabled Control channel
            for (i = 0; i < 16; i++) {
                if ((_midimap[i] & 0x4000) == 0x4000) {
                    midi_tx_cc(i, MIDICTL_TUTTI, MIDICTL_TUTTI_OFF_VAL);
                    break;
                }
            }
        }

        for (g = 0; g < _ngroup; g++)
        {
            s = d [g];
            G = _group + g;
            for (i = 0; i < G->_nifelm; i++)
            {
                set_ifelm (g, i, s & 1);
                s >>= 1;
            }
        }
        send_event (TO_IFACE, new M_ifc_preset (MT_IFC_PRRCL, bank, pres, _ngroup, d));
    }
    else send_event (TO_IFACE, new M_ifc_preset (MT_IFC_PRRCL, bank, pres, 0, 0));
}


void Model::set_aupar (int s, int a, int p, float v)
{
    Fparm  *P;
    int midi_ch = 0, midi_cc = 0, midi_val = 0;
    int i;
    float min_val = 0.0f, max_val = 0.0f;
    debug("s=%d, a=%d, p=%d, v=%f", s, a, p, v);

    if (s != SRC_MIDI_PAR) {
        midi_ch = -1;
        midi_cc = 0;
        midi_val = 0;

        // master control
        if (a == -1) {
            // determine midi channel
            // output on first enabled midi channel
            for (i = 0; i < 16; i++) {
                if ((_midimap[i] & 0x4000) == 0x4000) {
                    midi_ch = i;
                    break;
                }
            }

            switch (p) {
            case 0:
                midi_cc = MIDICTL_MAVOL;
                min_val = MAVOL_MIN;
                max_val = MAVOL_MAX;
                break;
            case 1:
                midi_cc = MIDICTL_RDELY;
                min_val = RDELY_MIN;
                max_val = RDELY_MAX;
                break;
            case 2:
                midi_cc = MIDICTL_RTIME;
                min_val = RTIME_MIN;
                max_val = RTIME_MAX;
                break;
            case 3:
                midi_cc = MIDICTL_RPOSI;
                min_val = RPOSI_MIN;
                max_val = RPOSI_MAX;
                break;
            }
        }
        // divison control
        else {
            // determine midi channel
            // output on first enabled midi channel
            for (i = 0; i < 16; i++) {
                if ((a == 0 && (_midimap[i] & 0x104F) == 0x1001) ||
                    (a == 1 && (_midimap[i] & 0x104F) == 0x1002) ||
                    (a == 2 && (_midimap[i] & 0x104F) == 0x1004) ||
                    (a == 3 && (_midimap[i] & 0x104F) == 0x1048)) {
                    midi_ch = i;
                    break;
                }
            }

            switch (p) {
            case 0:
                midi_cc = MIDICTL_DAZIM;
                min_val = DAZIM_MIN;
                max_val = DAZIM_MAX;
                break;
            case 1:
                midi_cc = MIDICTL_DWIDT;
                min_val = DWIDT_MIN;
                max_val = DWIDT_MAX;
                break;
            case 2:
                midi_cc = MIDICTL_DDIRE;
                min_val = DDIRE_MIN;
                max_val = DDIRE_MAX;
                break;
            case 3:
                midi_cc = MIDICTL_DREFL;
                min_val = DREFL_MIN;
                max_val = DREFL_MAX;
                break;
            case 4:
                midi_cc = MIDICTL_DREVB;
                min_val = DREVB_MIN;
                max_val = DREVB_MAX;
                break;
            }
        }

        if (midi_ch != -1) {
            midi_val = (127.0f * (v - min_val)) / (max_val - min_val);
            midi_tx_cc(midi_ch, midi_cc, midi_val);
        }
    }

    P = ((a < 0) ? _audio->_instrpar : _audio->_asectpar [a]) + p;
    if (v < P->_min) v = P->_min;
    if (v > P->_max) v = P->_max;
    P->_val = v;
    send_event (TO_IFACE, new M_ifc_aupar (s, a, p, v));
}


void Model::set_dipar (int s, int d, int p, float v)
{
    Fparm  *P;
    union { uint32_t i; float f; } u;
    int midi_ch = 0, midi_cc = 0, midi_val = 0;
    int i;
    float min_val = 0.0f, max_val = 0.0f;

    P = _divis [d]._param + p;
    if (v < P->_min) v = P->_min;
    if (v > P->_max) v = P->_max;
    P->_val = v;

    debug("s=%d, d=%d, p=%d, v=%f", s, d, p, v);
    // midi output cc message
    // III _midimap 0x2000
    // II  _midimap 0x2100
    if (s != SRC_MIDI_PAR) {
    midi_ch = -1;
    midi_cc = 0;
    midi_val = 0;

    // determine midi channel
    // output on first enabled midi channel
    for (i = 0; i < 16; i++) {
        if ((d == 0 && (_midimap[i] & 0x2100) == 0x2000) ||
            (d == 1 && (_midimap[i] & 0x2100) == 0x2100)) {
            midi_ch = i;
            break;
        }
    }

    switch (p) {
        case 0:
        midi_cc = MIDICTL_SWELL;
        min_val = SWELL_MIN;
        max_val = SWELL_MAX;
        break;
        case 1:
        midi_cc = MIDICTL_TFREQ;
        min_val = TFREQ_MIN;
        max_val = TFREQ_MAX;
        break;
        case 2:
        midi_cc = MIDICTL_TMODD;
        min_val = TMODD_MIN;
        max_val = TMODD_MAX;
        break;
    }

    if (midi_ch != -1) {
        midi_val = (127.0f * (v - min_val)) / (max_val - min_val);
        midi_tx_cc(midi_ch, midi_cc, midi_val);
    }
    }

    if (_qcomm->write_avail () >= 2)
    {
        u.f = v;
        _qcomm->write (0, (17 << 24) | (d << 16) | (p << 8));
        _qcomm->write (1, u.i);
        _qcomm->write_commit (2);
        send_event (TO_IFACE, new M_ifc_dipar (s, d, p, v));
    }
}


void Model::set_transpose (int s, int t)
{
    int i;

    // transpose is centered about 64
    // i.e
    // <52 = no change
    //  62 = -2
    //  63 = -1
    //  64 = 0
    //  65 = 1
    //  66 = 2
    // >76 = no change
    if (t < 52 || t > 76)
    return;

    debug("s=%d, t=%d", s, t);
    _transpose = t;

    if (_qcomm->write_avail () >= 1)
    {
        _qcomm->write (0, AU_PARAM << 24 | AU_PARAM_TRANSPOSE << 16 | (t & 0xffff));
            _qcomm->write_commit (1);

        // update transpose value in gui
            send_event (TO_IFACE, new M_ifc_transpose (t - 64));

        // output transpose midi message on first enabled Control channel
        if (s != SRC_MIDI_PAR) {
            for (i = 0; i < 16; i++)
                if ((_midimap[i] & 0x4000) == 0x4000)
                    midi_tx_cc(i, MIDICTL_TRNSP, t);
        }
    }
}


void Model::set_mconf (int i, uint16_t *d)
{
    int j, a, b;

    debug_nlf("i=%d, d[]=", i);

    midi_off (127);
    for (j = 0; j < 16; j++)
    {
        a = d [j];
        b =  (a & 0x1000) ? (_keybd [a & 7]._flags & 127) : 0;
        b |= a & 0x7700;
        if (x_opt) {
            //printf("%04x ", a);
            printf("%04x ", b);	// print the var stored in this class
        }
        _midimap [j] = b;
    }
    if (x_opt) printf("\n");
    send_event (TO_IFACE, new M_ifc_chconf (MT_IFC_MCSET, i, d));
}


void Model::midi_off (int mask)
{
    mask &= 127;
    if (_qcomm->write_avail ())
    {
        _qcomm->write (0, (2 << 24) | (mask << 16) | mask);
        _qcomm->write_commit (1);
    }
}


void Model::retune (float freq, int temp)
{
    if (_ready)
    {
        _fbase = freq;
        _itemp = temp;
        init_ranks (MT_CALC_RANK);
    }
    else send_event (TO_IFACE, new M_ifc_retune (_fbase, _itemp));
}


void Model::recalc (int g, int i)
{
    _count++;
    _ready = false;
    proc_rank (g, i, MT_CALC_RANK);
    send_event (TO_SLAVE, new ITC_mesg (MT_AUDIO_SYNC));
}


void Model::save (void)
{
    int     g, i;
    Group   *G;

    write_aparams ();
    write_instr ();
    write_presets ();
    _ready = false;
    for (g = 0; g < _ngroup; g++)
    {
        G = _group + g;
        for (i = 0; i < G->_nifelm; i++) proc_rank (g, i, MT_SAVE_RANK);
    }
    send_event (TO_SLAVE, new ITC_mesg (MT_AUDIO_SYNC));
}


Rank *Model::find_rank (int g, int i)
{
    int    d, r;
    Ifelm  *I;

    I = _group [g]._ifelms + i;
    if ((I->_type == Ifelm::DIVRANK) || (I->_type == Ifelm::KBDRANK))
    {
        d = (I->_action0 >> 16) & 255;
        r = (I->_action0 >>  8) & 255;
        return _divis [d]._ranks + r;
    }
    return 0;
}


int Model::read_aparams (void)
{
    int i, j;
    double val;
    char           temp [1200];
    unsigned char  *p;
    FILE           *F;

    // open file for reading
    if (_uhome)
    {
        p = (unsigned char *)(getenv ("HOME"));
        if (p) sprintf (temp, "%s/.aeolus-aparams", p);
        else strcpy (temp, ".aeolus-aparams");
    }
    else
    {
        sprintf (temp, "%s/aparams", _instr);
    }
    if (! (F = fopen (temp, "r")))
    {
        fprintf (stderr, "Can't open '%s' for reading\n", temp);
        return 1;
    }
    printf ("Reading '%s'\n", temp);

    // read file line-by-line
    while (fgets (temp, 1024, F))
    {
        if (sscanf(temp, "_instrpar[%d]=%lf", &i, &val)) {
            debug("read _instrpar[%d]=%f", i, val);
            set_aupar(SRC_MIDI_PAR, -1, i, val);
        }

        if (sscanf(temp, "_asectpar[%d][%d]=%lf", &i, &j, &val)) {
            debug("read _asectpar[%d][%d]=%f", i, j, val);
            set_aupar(SRC_MIDI_PAR, i, j, val);
        }

        if (sscanf(temp, "_divis[%d]._param[%d]=%lf", &i, &j, &val)) {
            debug("read _divis[%d]._param[%d]=%f", i, j, val);
            set_dipar(SRC_MIDI_PAR, i, j, val);
        }
    }

    return 0;
}


int Model::write_aparams (void)
{
    int i, j;
    double val;
    char           name [1200];
    unsigned char  *p, data [256];
    FILE           *F;

    // open file for writing
    if (_uhome)
    {
        p = (unsigned char *)(getenv ("HOME"));
        if (p) sprintf (name, "%s/.aeolus-aparams", p);
        else strcpy (name, ".aeolus-aparams");
    }
    else
    {
        sprintf (name, "%s/aparams", _instr);
    }
    if (! (F = fopen (name, "w")))
    {
        fprintf (stderr, "Can't open '%s' for writing\n", name);
        return 1;
    }
    printf ("Writing '%s'\n", name);


    // global audio parameters
    for (i = 0; i < 4; i++)
    {
        val = _audio->_instrpar[i]._val;
        debug("_instrpar[%d]=%f", i, val);

        sprintf((char *) data, "_instrpar[%d]=%f\n", i, val);
        fwrite (data, strlen((char *) data), 1, F);
    }

    // section audio parameters
    for (i = 0; i < _nasect; i++)
    {
        for (j = 0; j < 5; j++)
        {
            val = _audio->_asectpar[i][j]._val;
            debug("_asectpar[%d][%d]=%f", i, j, val);

            sprintf((char *) data, "_asectpar[%d][%d]=%f\n", i, j, val);
            fwrite (data, strlen((char *) data), 1, F);
        }
    }

    // division audio parameters
    for (i = 0; i < _ndivis; i++)
    {
        for (j = 0; j < 3; j++)
        {
            val = _divis[i]._param[j]._val;
            debug("_divis[%d]._param[%d]=%f", i, j, val);

            sprintf((char *) data, "_divis[%d]._param[%d]=%f\n", i, j, val);
            fwrite (data, strlen((char *) data), 1, F);
        }
    }

    fclose (F);

    return 0;
}


int Model::read_instr (void)
{
    FILE          *F;
    int           line, stat, n;
    bool          instr;
    int           d, k, r, s;
    char          c, *p, *q;
    char          buff [1200];
    char          t1 [256];
    char          t2 [256];
    Keybd         *K;
    Divis         *D;
    Rank          *R;
    Group         *G;
    Ifelm         *I;
    Addsynth      *A;

    enum { CONT, DONE, ERROR, COMM, ARGS, MORE, NO_INSTR, IN_INSTR,
           BAD_SCOPE, BAD_ASECT, BAD_RANK, BAD_DIVIS, BAD_KEYBD, BAD_IFACE,
           BAD_STR1, BAD_STR2 };

    sprintf (buff, "%s/definition", _instr);
    if (! (F = fopen (buff, "r")))
    {
        fprintf (stderr, "Can't open '%s' for reading\n", buff);
        return 1;
    }
    printf ("Reading '%s'\n", buff);

    stat = 0;
    line = 0;
    instr = false;
    D = 0;
    G = 0;
    d = k = r = s = 0;

    while (! stat && fgets (buff, 1024, F))
    {
        line++;
        p = buff;
        if (*p != '/')
        {
            while (isspace (*p)) p++;
            if ((*p > ' ') && (*p != '#'))
            {
                fprintf (stderr, "Syntax error in line %d\n", line);
                stat = COMM;
            }
            continue;
        }

        q = p;
        while ((*q >= ' ') && !isspace (*q)) q++;
        *q++ = 0;
        while ((*q >= ' ') && isspace (*q)) q++;

        if (! strcmp (p, "/instr/new"))
        {
            if (instr) stat = IN_INSTR;
            else instr = true;
        }
        else if (! instr)
        {
        stat = NO_INSTR;
        }
        else if (! strcmp (p, "/instr/end"))
        {
            instr = false;
            stat = DONE;
        }
        else if (! strcmp (p, "/manual/new") || ! strcmp (p, "/pedal/new"))
        {
        if (D || G) stat = BAD_SCOPE;
            else if (sscanf (q, "%s%n", t1, &n) != 1) stat = ARGS;
            else
            {
                q += n;
                if (_nkeybd == NKEYBD)
                {
                    fprintf (stderr, "Line %d: can't create more than %d keyboards\n", line, NKEYBD);
                    stat = ERROR;
                }
                else if (strlen (t1) > 15) stat = BAD_STR1;
                else
                {
                    k = _nkeybd++;
                    K = _keybd + k;
                    strcpy (K->_label, t1);
                    K->_flags = 1 << k;
                    if (p [1] == 'p') K->_flags |= HOLD_MASK | Keybd::IS_PEDAL;
                }
            }
        }
        else if (! strcmp (p, "/divis/new"))
        {
            if (D || G) stat = BAD_SCOPE;
            else if (sscanf (q, "%s%d%d%n", t1, &k, &s, &n) != 3) stat = ARGS;
            else
            {
                q += n;
                if (_ndivis == NDIVIS)
                {
                    fprintf (stderr, "Line %d: can't create more than %d divisions\n", line, NDIVIS);
                    stat = ERROR;
                }
                else if (strlen (t1) > 15) stat = BAD_STR1;
                else if ((k < 0) || (k > _nkeybd)) stat = BAD_KEYBD;
                else if ((s < 1) || (s > NASECT))  stat = BAD_ASECT;
                else
                {
                    D = _divis + _ndivis++;
                    strcpy (D->_label, t1);
                    if (_nasect < s) _nasect = s;
                    D->_asect = s - 1;
                    D->_keybd = k - 1;
                    if (k--) D->_dmask = _keybd [k]._flags & 127;
                }
            }
        }
        else if (! strcmp (p, "/divis/end"))
        {
            if (!D || G) stat = BAD_SCOPE;
            else D = 0;
        }
        else if (! strcmp (p, "/group/new"))
        {
            if (D || G) stat = BAD_SCOPE;
            else if (sscanf (q, "%s%n", t1, &n) != 1) stat = ARGS;
            else
            {
                q += n;
                if (_ngroup == NGROUP)
                {
                    fprintf (stderr, "Line %d: can't create more than %d groups\n", line, NGROUP);
                    stat = ERROR;
                }
                else if (strlen (t1) > 15) stat = BAD_STR1;
                else
                {
                    G = _group + _ngroup++;
                    strcpy (G->_label, t1);
                }
            }
        }
        else if (! strcmp (p, "/group/end"))
        {
            if (D || !G) stat = BAD_SCOPE;
            else G = 0;
        }
        else if (! strcmp (p, "/tuning"))
        {
            if (D || G) stat = BAD_SCOPE;
            else if (sscanf (q, "%f%d%n", &_fbase, &_itemp, &n) != 2) stat = ARGS;
            else q += n;
        }
        else if (! strcmp (p, "/rank"))
        {
            if (!D && G) stat = BAD_SCOPE;
            else if (sscanf (q, "%c%d%s%n", &c, &d, t1, &n) != 3) stat = ARGS;
            else
            {
                q += n;
                if (D->_nrank == Divis::NRANK)
                {
                    fprintf (stderr, "Line %d: can't create more than %d ranks per division\n", line, Divis::NRANK);
                    stat = ERROR;
                }
                else if (strlen (t1) > 63) stat = BAD_STR1;
                else
                {
                    A = new Addsynth;
                    strcpy (A->_filename, t1);
                    if (A->load (_stops))
                    {
                        stat = ERROR;
                        delete A;
                    }
                    else
                    {
                        A->_pan = c;
                        A->_del = d;
                        R = D->_ranks + D->_nrank++;
                        R->_count = 0;
                        R->_sdef = A;
                        R->_wave = 0;
                    }
                }
            }
        }
        else if (! strcmp (p, "/tremul"))
        {
            if (D)
            {
                if (sscanf (q, "%f%f%n", &(D->_param [Divis::TFREQ]._val),
                                &(D->_param [Divis::TMODD]._val), &n) != 2) stat = ARGS;
                else
                {
                    q += n;
                    D->_flags |= Divis::HAS_TREM;
                }
            }
            else if (G)
            {
                if (sscanf (q, "%d%s%s%n", &d, t1, t2, &n) != 3) stat = ARGS;
                else
                {
                    q += n;
                    if (G->_nifelm == Group::NIFELM) stat = BAD_IFACE;
                    else if ((d < 1) || (d > _ndivis)) stat = BAD_DIVIS;
                    else if (strlen (t1) >  7) stat = BAD_STR1;
                    else if (strlen (t2) > 31) stat = BAD_STR2;
                    else
                    {
                        d--;
                        I = G->_ifelms + G->_nifelm++;
                        strcpy (I->_mnemo, t1);
                        strcpy (I->_label, t2);
                        I->_type = Ifelm::TREMUL;
                        I->_action0 = (16 << 24) | (d << 16) | 0;
                        I->_action1 = (16 << 24) | (d << 16) | 1;
                    }
               }
            }
            else stat = BAD_SCOPE;
        }
        else if (! strcmp (p, "/swell"))
        {
            if (!D || G) stat = BAD_SCOPE;
            else D->_flags |= Divis::HAS_SWELL;
        }
        else if (! strcmp (p, "/stop"))
        {
            if (D || !G) stat = BAD_SCOPE;
            else if (sscanf (q, "%d%d%d%n", &k, &d, &r, &n) != 3) stat = ARGS;
            else
            {
                q += n;
                if (G->_nifelm == Group::NIFELM) stat = BAD_IFACE;
                else if ((k < 0) || (k > _nkeybd)) stat = BAD_KEYBD;
                else if ((d < 1) || (d > _ndivis)) stat = BAD_DIVIS;
                else if ((r < 1) || (r > _divis [d - 1]._nrank)) stat = BAD_RANK;
                else
                {
                    k--;
                    d--;
                    r--;
                    I = G->_ifelms + G->_nifelm++;
                    R = _divis [d]._ranks + r;
                    strcpy (I->_label, R->_sdef->_stopname);
                    strcpy (I->_mnemo, R->_sdef->_mnemonic);
                    I->_keybd = k;
                    if (k >= 0)
                    {
                        I->_type = Ifelm::KBDRANK;
                        k = _keybd [k]._flags & 127;
                    }
                    else
                    {
                        I->_type = Ifelm::DIVRANK;
                        k = 128;
                    }
                    I->_action0 = (6 << 24) | (d << 16) | (r << 8) | k;
                    I->_action1 = (7 << 24) | (d << 16) | (r << 8) | k;
                }
            }
        }
        else if (! strcmp (p, "/coupler"))
        {
            if (D || !G) stat = BAD_SCOPE;
            else if (sscanf (q, "%d%d%s%s%n", &k, &d, t1, t2, &n) != 4) stat = ARGS;
            else
            {
            q += n;
                if (G->_nifelm == Group::NIFELM) stat = BAD_IFACE;
                else if ((k < 1) || (k > _nkeybd)) stat = BAD_KEYBD;
                else if ((d < 1) || (d > _ndivis)) stat = BAD_DIVIS;
                else if (strlen (t1) >  7) stat = BAD_STR1;
                else if (strlen (t2) > 31) stat = BAD_STR2;
                else
                {
                    k--;
                    d--;
                    I = G->_ifelms + G->_nifelm++;
                    strcpy (I->_mnemo, t1);
                    strcpy (I->_label, t2);
                    I->_type = Ifelm::COUPLER;
                    I->_keybd = k;
                    k = _keybd [k]._flags & 127;
                    I->_action0 = (4 << 24) | (d << 16) | k;
                    I->_action1 = (5 << 24) | (d << 16) | k;
                }
            }
        }
        else stat = COMM;

        if (stat <= DONE)
        {
            while (isspace (*q)) q++;
            if (*q > ' ') stat = MORE;
        }

        switch (stat)
        {
        case COMM:
        fprintf (stderr, "Line %d: unknown command '%s'\n", line, p);
            break;
        case ARGS:
        fprintf (stderr, "Line %d: missing arguments in '%s' command\n", line, p);
            break;
        case MORE:
        fprintf (stderr, "Line %d: extra arguments in '%s' command\n", line, p);
            break;
        case NO_INSTR:
        fprintf (stderr, "Line %d: command '%s' outside instrument scope\n", line, p);
            break;
        case IN_INSTR:
        fprintf (stderr, "Line %d: command '%s' inside instrument scope\n", line, p);
            break;
        case BAD_SCOPE:
        fprintf (stderr, "Line %d: command '%s' in wrong scope\n", line, p);
            break;
        case BAD_ASECT:
        fprintf (stderr, "Line %d: no section '%d'\n", line, s);
            break;
        case BAD_RANK:
        fprintf (stderr, "Line %d: no rank '%d' in division '%d'\n", line, r, d);
            break;
        case BAD_KEYBD:
        fprintf (stderr, "Line %d: no keyboard '%d'\n", line, k);
            break;
        case BAD_DIVIS:
        fprintf (stderr, "Line %d: no division '%d'\n", line, d);
            break;
        case BAD_IFACE:
        fprintf (stderr, "Line %d: can't create more than '%d' elements per group\n", line, Group::NIFELM);
            break;
        case BAD_STR1:
        fprintf (stderr, "Line %d: string '%s' is too long\n", line, t1);
            break;
        case BAD_STR2:
        fprintf (stderr, "Line %d: string '%s' is too long\n", line, t1);
            break;
        }
    }

    fclose (F);
    return (stat <= DONE) ? 0 : 2;
}


int Model::write_instr (void)
{
    FILE          *F;
    int           d, g, i, k, r;
    char          buff [1200];
    time_t        t;
    Divis         *D;
    Rank          *R;
    Group         *G;
    Ifelm         *I;
    Addsynth      *A;

    sprintf (buff, "%s/definition", _instr);
    if (! (F = fopen (buff, "w")))
    {
        fprintf (stderr, "Can't open '%s' for writing\n", buff);
        return 1;
    }
    printf ("Writing '%s'\n", buff);
    t = time (0);

    fprintf (F, "# Aeolus instrument definition file\n");
    fprintf (F, "# Created by Aeolus-%s at %s\n", VERSION, ctime (&t));

    fprintf (F, "\n/instr/new\n");
    fprintf (F, "/tuning %5.1f %d\n", _fbase, _itemp);

    fprintf (F, "\n# Keyboards\n#\n");
    for (k = 0; k < _nkeybd; k++)
    {
        if (_keybd [k]._flags & Keybd::IS_PEDAL) fprintf (F, "/pedal/new    %s\n", _keybd [k]._label);
        else                                     fprintf (F, "/manual/new   %s\n", _keybd [k]._label);
    }

    fprintf (F, "\n# Divisions\n#\n");
    for (d = 0; d < _ndivis; d++)
    {
        D = _divis + d;
        fprintf (F, "/divis/new    %-7s  %d  %d\n", D->_label, D->_keybd + 1, D->_asect + 1);
        for (r = 0; r < D->_nrank; r++)
    {
        R = D->_ranks + r;
        A = R->_sdef;
        fprintf (F, "/rank         %c %3d  %s\n", A->_pan, A->_del, A->_filename);
    }
        if (D->_flags & Divis::HAS_SWELL) fprintf (F, "/swell\n");
        if (D->_flags & Divis::HAS_TREM) fprintf (F, "/tremul       %3.1f  %3.1f\n",
                                                  D->_param [Divis::TFREQ]._val, D->_param [Divis::TMODD]._val);
        fprintf (F, "/divis/end\n\n");
    }

    fprintf (F, "# Interface groups\n#\n");
    for (g = 0; g < _ngroup; g++)
    {
        G = _group + g;
        fprintf (F, "/group/new    %-7s\n", G->_label);
        for (i = 0; i < G->_nifelm; i++)
        {
            I = G->_ifelms + i;
            switch (I->_type)
            {
            case Ifelm::DIVRANK:
            case Ifelm::KBDRANK:
                    k = I->_keybd;
                    d = (I->_action0 >> 16) & 255;
                    r = (I->_action0 >>  8) & 255;
                    fprintf (F, "/stop         %d   %d  %2d\n", k + 1, d + 1, r + 1);
            break;

            case Ifelm::COUPLER:
                    k = I->_keybd;
                    d = (I->_action0 >> 16) & 255;
                    fprintf (F, "/coupler      %d   %d   %-7s  %s\n", k + 1, d + 1, I->_mnemo, I->_label);
            break;

            case Ifelm::TREMUL:
                    d = (I->_action0 >> 16) & 255;
                    D = _divis + d;
                    fprintf (F, "/tremul       %d       %-7s  %s\n", d + 1, I->_mnemo, I->_label);
            break;
            }
        }
        fprintf (F, "/group/end\n\n");
    }

    fprintf (F, "\n/instr/end\n");
    fclose (F);

    return 0;
}


int Model::get_preset (int bank, int pres, uint32_t *bits)
{
    int     k;
    Preset  *P;

    if ((bank < 0) | (pres < 0) || (bank >= NBANK) || (pres >= NPRES)) return 0;
    P = _preset [bank][pres];
    if (P)
    {
        for (k = 0; k < _ngroup; k++) *bits++ = P->_bits [k];
        return k;
    }
    return 0;
}


void Model::set_preset (int bank, int pres, uint32_t *bits)
{
    int     k;
    Preset  *P;

    if ((bank  < 0) | (pres < 0) || (bank >= NBANK) || (pres >= NPRES)) return;
    P = _preset [bank][pres];
    if (! P)
    {
        P = new Preset;
        _preset [bank][pres] = P;
    }
    for (k = 0; k < _ngroup; k++) P->_bits [k] = *bits++;
}


void Model::ins_preset (int bank, int pres, uint32_t *bits)
{
    int     j, k;
    Preset  *P;

    if ((bank < 0) | (pres < 0) || (bank >= NBANK) || (pres >= NPRES)) return;
    P = _preset [bank][NPRES - 1];
    for (j = NPRES - 1; j > pres; j--) _preset [bank][j] = _preset [bank][j - 1];
    if (! P)
    {
        P = new Preset;
        _preset [bank][pres] = P;
    }
    for (k = 0; k < _ngroup; k++) P->_bits [k] = *bits++;
}


void Model::del_preset (int bank, int pres)
{
    int j;

    if ((bank < 0) | (pres < 0) || (bank >= NBANK) || (pres >= NPRES)) return;
    delete _preset [bank][pres];
    for (j = pres; j < NPRES - 1; j++) _preset [bank][j] = _preset [bank][j + 1];
    _preset [bank][NPRES - 1] = 0;
}


int Model::read_presets (void)
{
    int            i, j, k, n;
    char           name [1200];
    unsigned char  *p, data [256];
    FILE           *F;
    Preset         *P;

    if (_uhome)
    {
        p = (unsigned char *)(getenv ("HOME"));
        if (p) sprintf (name, "%s/.aeolus-presets", p);
        else strcpy (name, ".aeolus-presets");
    }
    else
    {
        sprintf (name, "%s/presets", _instr);
    }
    if (! (F = fopen (name, "r")))
    {
        fprintf (stderr, "Can't open '%s' for reading\n", name);
        return 1;
    }

    fread (data, 16, 1, F);
    if (strcmp ((char *) data, "PRESET") || data [7])
    {
        fprintf (stderr, "File '%s' is not a valid preset file\n", name);
        fclose (F);
        return 1;
    }
    printf ("Reading '%s'\n", name);
    n = RD2 (data + 14);

    if (fread (data, 256, 1, F) != 1)
    {
        fprintf (stderr, "No valid data in file '%s'\n", name);
        fclose (F);
        return 1;
    }
    p = data;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 16; j++)
        {
            _chconf [i]._bits [j] = RD2 (p);
            p += 2;
        }
    }

    if (n != _ngroup)
    {
        fprintf (stderr, "Presets in file '%s' are not compatible\n", name);
        fclose (F);
        return 1;
    }
    while (fread (data, 4 + 4 * _ngroup, 1, F) == 1)
    {
        p = data;
        i = *p++;
        j = *p++;
        p++;
        p++;
        if ((i < NBANK) && (j < NPRES))
        {
            P = new Preset;
            for (k = 0; k < _ngroup; k++)
            {
                P->_bits [k] = RD4 (p);
                p += 4;
            }
            _preset [i][j] = P;
        }
    }

    fclose (F);
    return 0;
}


int Model::write_presets (void)
{
    int            i, j, k, v;
    char           name [1200];
    unsigned char  *p, data [256];
    FILE           *F;
    Preset         *P;

    if (_uhome)
    {
        p = (unsigned char *)(getenv ("HOME"));
        if (p) sprintf (name, "%s/.aeolus-presets", p);
        else strcpy (name, ".aeolus-presets");
    }
    else
    {
        sprintf (name, "%s/presets", _instr);
    }
    if (! (F = fopen (name, "w")))
    {
        fprintf (stderr, "Can't open '%s' for writing\n", name);
        return 1;
    }
    printf ("Writing '%s'\n", name);

    strcpy ((char *) data, "PRESET");
    data [7] = 0;
    WR2 (data +  8, 0);
    WR2 (data + 10, 0);
    WR2 (data + 12, 0);
    WR2 (data + 14, _ngroup);
    fwrite (data, 16, 1, F);

    p = data;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 16; j++)
        {
            v = _chconf [i]._bits [j];
            WR2 (p, v);
                p += 2;
        }
    }
    fwrite (data, 256, 1, F);

    for (i = 0; i < NBANK; i++)
    {
        for (j = 0; j < NPRES; j++)
        {
            P = _preset [i][j];
            if (P)
            {
                p = data;
                *p++ = i;
                *p++ = j;
                *p++ = 0;
                *p++ = 0;
                for (k = 0; k < _ngroup; k++)
                {
                    v = P->_bits [k];
                    WR4 (p, v);
                    p += 4;
                }
                fwrite (data, 4 + 4 * _ngroup, 1, F);
            }
        }
    }

    fclose (F);
    return 0;
}


void Model::midi_tx_note(int ch, int key, int vel, bool state)
{
    snd_seq_event_t ev;

    debug("ch=0x%02x key=0x%02x vel=0x%02x s=%s", ch, key, vel, state==true ? "on" : "off");

    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, _midi->_opport);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    if (state)
    snd_seq_ev_set_noteon(&ev, ch, key, vel);
    else
    snd_seq_ev_set_noteoff(&ev, ch, key, vel);
    snd_seq_event_output_direct(_midi->_seq, &ev);
}


void Model::midi_tx_cc(int ch, int cc, int val)
{
    snd_seq_event_t ev;

    debug("cc ch=0x%02x cc=0x%02x val=0x%02x", ch, cc, val);

    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, _midi->_opport);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_controller(&ev, ch, cc, val);
    snd_seq_event_output_direct(_midi->_seq, &ev);
}


void Model::midi_tx_sysex(int data_len, uint8_t *data_ptr)
{
    int i;
    snd_seq_event_t ev;

    // print to console
    if (x_opt) {
        debug_nlf("data_len=%d data=", data_len);
        for (i = 0; i < data_len; i++)
            printf("0x%02X ", data_ptr[i]);
        printf("\n");
    }

    // send midi event
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, _midi->_opport);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_sysex(&ev, data_len, data_ptr);
    snd_seq_event_output_direct(_midi->_seq, &ev);
}
