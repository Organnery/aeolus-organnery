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


#include "imidi.h"



Imidi::Imidi (Lfq_u32 *qnote, Lfq_u8 *qmidi, uint16_t *midimap, const char *appname) :
    A_thread ("Imidi"),
    _qnote (qnote),
    _qmidi (qmidi),
    _midimap (midimap),
    _appname (appname)
{
}


Imidi::~Imidi (void)
{
}


void Imidi::terminate (void)
{
    snd_seq_event_t E;

    if (_handle)
    {
        snd_seq_ev_clear (&E);
        snd_seq_ev_set_direct (&E);
        E.type = SND_SEQ_EVENT_USR0;
        E.source.port = _opport;
        E.dest.client = _client;
        E.dest.port   = _ipport;
        snd_seq_event_output_direct (_handle, &E);
    }
}


void Imidi::thr_main (void)
{
    open_midi ();
    proc_midi ();
    close_midi ();
    send_event (EV_EXIT, 1);
}


void Imidi::open_midi (void)
{
    snd_seq_client_info_t *C;
    M_midi_info *M;

    if (snd_seq_open (&_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    {
        fprintf(stderr, "Error opening ALSA sequencer.\n");
        exit(1);
    }

    snd_seq_client_info_alloca (&C);
    snd_seq_get_client_info (_handle, C);
    _client = snd_seq_client_info_get_client (C);
    snd_seq_client_info_set_name (C, _appname);
    snd_seq_set_client_info (_handle, C);

    if ((_ipport = snd_seq_create_simple_port (_handle, "In",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION)) < 0)
    {
        fprintf(stderr, "Error creating sequencer input port.\n");
        exit(1);
    }

    if ((_opport = snd_seq_create_simple_port (_handle, "Out",
         SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
         SND_SEQ_PORT_TYPE_APPLICATION)) < 0)
    {
        fprintf(stderr, "Error creating sequencer output port.\n");
        exit(1);
    }

    M = new M_midi_info ();
    M->_client = _client;
    M->_ipport = _ipport;
    M->_opport = _opport;
    M->_seq = _handle;
    memcpy (M->_chbits, _midimap, 16 * sizeof (uint16_t));
    send_event (TO_MODEL, M);
}


void Imidi::close_midi (void)
{
    if (_handle) snd_seq_close (_handle);
}


void Imidi::proc_midi (void)
{
    snd_seq_event_t  *E;
    int              c, f, m, n, p, t, v, g;

    // Read and process MIDI commands from the ALSA port.
    // Events related to keyboard state are sent to the
    // audio thread via the qnote queue. All the rest is
    // sent as raw MIDI to the model thread via qmidi.

    while (true)
    {
        snd_seq_event_input(_handle, &E);
        c = E->data.note.channel;
        m = _midimap [c] & 127;        // Keyboard and hold bits
//        d = (_midimap [c] >>  8) & 7;  // Division number if (f & 2)
        f = (_midimap [c] >> 12) & 7;  // Control enabled if (f & 4)

        t = E->type;
        switch (t)
        {
            case SND_SEQ_EVENT_NOTEON:
            case SND_SEQ_EVENT_NOTEOFF:
                n = E->data.note.note;
                v = E->data.note.velocity;
                // get interface group attached to midi channel
                g = 0;
                switch (m & 0x0F) {
                    case 0x01:
                        g = 0;
                    break;
                    case 0x02:
                        g = 1;
                    break;
                    case 0x04:
                        g = 2;
                    break;
                    case 0x08:
                        g = 3;
                    break;
                    case 0x0F:
                        g = 4;
                    break;
                }
                if ((t == SND_SEQ_EVENT_NOTEON) && v)
                {
                    // Note on.
                    if (n < 36)
                    {
                        if ((f & 4) && (n >= 0) && (n < 24))
                        {
                            // Stop enable, sent to model thread
                            if (_qmidi->write_avail () >= 3)
                            {
                            // send group and set on
                            _qmidi->write (0, 0xB0 | c);
                            _qmidi->write (1, MIDICTL_IFELM);
                            _qmidi->write (2, 0x60 | g);
                            _qmidi->write_commit (3);
                            // send stop number
                            _qmidi->write (0, 0xB0 | c);
                            _qmidi->write (1, MIDICTL_IFELM);
                            _qmidi->write (2, n); // stop number based on note
                            _qmidi->write_commit (3);

                            }
                        }

                        if ((f & 4) && (n >= 24) && (n < 34))
                        {
                            // Preset selection, sent to model thread
                            // if on control-enabled channel.
                            if (_qmidi->write_avail () >= 3)
                            {
                                _qmidi->write (0, 0x90);
                                _qmidi->write (1, n);
                                _qmidi->write (2, v);
                                _qmidi->write_commit (3);
                            }
                        }
                    }
                    else if (n <= 96)
                    {
                        if (m)
                        {
                            if (_qnote->write_avail () > 0)
                            {
                                _qnote->write (0, (1 << 24) | ((n - 36) << 8) | m);
                                    _qnote->write_commit (1);
                            }
                        }
                    }
                }
                else
                {
                    // Note off.
                    if (n < 36)
                    {
                        if ((f & 4) && (n >= 0) && (n < 24))
                        {
                            // Stop disable, sent to model thread
                            if (_qmidi->write_avail () >= 3)
                            {
                            // send group and set off
                            _qmidi->write (0, 0xB0 | c);
                            _qmidi->write (1, MIDICTL_IFELM);
                            _qmidi->write (2, 0x50 | g);
                            _qmidi->write_commit (3);
                            // send stop number
                            _qmidi->write (0, 0xB0 | c);
                            _qmidi->write (1, MIDICTL_IFELM);
                            _qmidi->write (2, n);
                            _qmidi->write_commit (3);
                            }
                        }
                    }
                    else if (n <= 96)
                    {
                        if (m)
                        {
                            if (_qnote->write_avail () > 0)
                            {
                                _qnote->write (0, ((n - 36) << 8) | m);
                                    _qnote->write_commit (1);
                            }
                        }
                    }
                }
                break;

            case SND_SEQ_EVENT_CONTROLLER:
                p = E->data.control.param;
                v = E->data.control.value;
                debug("cc c=0x%02x p=0x%02x v=0x%02x", c, p, v);

                switch (p)
                {
                    case MIDICTL_HOLD:
                    // Hold pedal.
                    if (m & HOLD_MASK)
                    {
                        v = (v > 63) ? 9 : 8;
                            if (_qnote->write_avail () > 0)
                            {
                                _qnote->write (0, (v << 24) | (m << 16));
                                    _qnote->write_commit (1);
                            }
                    }
                    break;

                    case MIDICTL_ASOFF:
                    // All sound off, accepted on control channels only.
                    // Clears all keyboards, including held notes.
                    if (f & 4)
                    {
                        if (_qnote->write_avail () > 0)
                        {
                            _qnote->write (0, (2 << 24) | ( ALL_MASK << 16) | ALL_MASK);
                            _qnote->write_commit (1);
                        }
                    }
                    break;

                    case MIDICTL_ANOFF:
                    // All notes off, accepted on channels controlling
                    // a keyboard. Does not clear held notes.
                    if (m)
                    {
                        if (_qnote->write_avail () > 0)
                        {
                            _qnote->write (0, (2 << 24) | (m << 16) | m);
                                _qnote->write_commit (1);
                        }
                    }
                    break;

                    case MIDICTL_DAZIM:
                    case MIDICTL_DWIDT:
                    case MIDICTL_DDIRE:
                    case MIDICTL_DREFL:
                    case MIDICTL_DREVB:
                    // Divison audio params, sent to model thread if controlling
                    // a keyboard.
                    if (m)
                    {
                        if (_qmidi->write_avail () >= 3)
                        {
                        // send the division channel rather than midi channel
                        //_qmidi->write (0, 0xB0 | c);
                        switch (m & 0x0F)
                        {
                        case 1: // III
                            _qmidi->write (0, 0xB0 | 0);
                            break;
                        case 2: // II
                            _qmidi->write (0, 0xB0 | 1);
                            break;
                        case 4: // I
                            _qmidi->write (0, 0xB0 | 2);
                            break;
                        case 8: // P
                            _qmidi->write (0, 0xB0 | 3);
                            break;
                        }
                        _qmidi->write (1, p);
                        _qmidi->write (2, v);
                        _qmidi->write_commit (3);
                        }
                    }
                    break;

                    case MIDICTL_MAVOL:
                    case MIDICTL_RDELY:
                    case MIDICTL_RTIME:
                    case MIDICTL_RPOSI:
                    case MIDICTL_PNEXT:
                    case MIDICTL_PPREV:
                    case MIDICTL_PSTOR:
                    case MIDICTL_CANCL:
                    case MIDICTL_TUTTI:
                    case MIDICTL_TRNSP:
                    case MIDICTL_BANK:
                    case MIDICTL_IFELM:
                    // Program bank selection, audio param or stop control, sent
                    // to model thread if on control-enabled channel.
                    if (f & 4)
                    {
                        if (_qmidi->write_avail () >= 3)
                        {
                        _qmidi->write (0, 0xB0 | c);
                        _qmidi->write (1, p);
                        _qmidi->write (2, v);
                        _qmidi->write_commit (3);
                        }
                    }
                    break;

                    case MIDICTL_SWELL:
                    case MIDICTL_TFREQ:
                    case MIDICTL_TMODD:
                    // Per-division performance controls, sent to model
                            // thread if on a channel that controls a division.
                    if (f & 2)
                    {
                        if (_qmidi->write_avail () >= 3)
                        {
                        _qmidi->write (0, 0xB0 | c);
                        _qmidi->write (1, p);
                        _qmidi->write (2, v);
                        _qmidi->write_commit (3);
                        }
                    }
                    break;

                    case MIDICTL_PRCAL:
                        // Program change sent to model thread
                        // if on control-enabled channel.
                        if (f & 4)
                        {
                            if (_qmidi->write_avail () >= 3)
                            {
                            _qmidi->write (0, 0xC0);
                            _qmidi->write (1, v);
                            _qmidi->write (2, 0);
                            _qmidi->write_commit (3);
                            }
                        }
                    }
                    break;

            case SND_SEQ_EVENT_PGMCHANGE:
                    // Program change sent to model thread
                    // if on control-enabled channel.
                if (f & 4)
                {
                    if (_qmidi->write_avail () >= 3)
                    {
                    _qmidi->write (0, 0xC0);
                    _qmidi->write (1, E->data.control.value);
                    _qmidi->write (2, 0);
                    _qmidi->write_commit (3);
                    }
                }
                break;

            case SND_SEQ_EVENT_USR0:
                // User event, terminates this trhead if we sent it.
                if (E->source.client == _client) return;
        }
    }
}

