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
#include <stdio.h>

#include "midimatrix.h"
#include "callbacks.h"
#include "messages.h"
#include "styles.h"



Midimatrix::Midimatrix (X_window *parent, X_callback *callb, int xp, int yp) :
    X_window (parent, xp, yp, 100, 100, Colors.midi_bg),
    _callb (callb),
    _mapped (false)
{
    x_add_events (ExposureMask | ButtonPressMask | StructureNotifyMask);
    x_set_bit_gravity (NorthWestGravity);
}


Midimatrix::~Midimatrix (void)
{
}


void Midimatrix::init (M_ifc_init *M)
{
    int i;

    _nkeybd = M->_nkeybd;
    _ndivis = 0;
    for (i = 0; i < M->_nkeybd; i++)
    {
        _label [i] = M->_keybdd [i]._label;
        _flags [i] = M->_keybdd [i]._flags;
    }
    for (i = 0; i < M->_ndivis; i++)
    {
        if (M->_divisd [i]._flags)
        {
            _ndivis++;
            _label [_nkeybd + i] = M->_divisd [i]._label;
        }
    }
    for (i = 0; i < 16; i++) _chconf [i] = 0;
    _xs = mmatrix.XL + 16 * mmatrix.DX + mmatrix.XR;
    _ys = mmatrix.YT + (_nkeybd + _ndivis + 1) * mmatrix.DY + mmatrix.YB;
    x_resize (_xs, _ys);
    x_map ();
}


void Midimatrix::set_chconf (uint16_t *d)
{
    plot_allconn ();
    memcpy (_chconf, d, 16 * sizeof (uint16_t));
    plot_allconn ();
}


void Midimatrix::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case MapNotify:
        _mapped = true;
        break;

    case UnmapNotify:
        _mapped = false;
        break;

    case Expose:
        expose ((XExposeEvent *) E);
        break;

    case ButtonPress:
        bpress ((XButtonEvent *) E);
        break;
    }
}


void Midimatrix::expose (XExposeEvent *E)
{
    if (E->count) return;
    redraw ();
}


void Midimatrix::redraw (void)
{
    int     i, x, y, d;
    char    s [4];
    X_draw  D (dpy (), win (), dgc (), xft ());

    if (! _mapped) return;
    D.clearwin ();
    D.setfunc (GXcopy);
    D.setcolor (Colors.midi_gr1);
    for (i = 0, x = mmatrix.XL + mmatrix.DX; i < 16; i++, x += mmatrix.DX)
    {
        D.move (x, mmatrix.YT);
        D.draw (x, _ys - mmatrix.YT);
    }
    for (i = 0, y = mmatrix.YT; i <= _nkeybd + _ndivis + 1; i++, y += mmatrix.DY)
    {
        D.move (0, y);
        D.draw (_xs - mmatrix.XR, y);
    }
    D.setcolor (XftColors.midi_fg);
    D.setfont (XftFonts.midimt);
    d = (mmatrix.DY + D.textascent () - D.textdescent ()) / 2;
    for (i = 0, y = mmatrix.YT; i < _nkeybd + _ndivis; i++, y += mmatrix.DY)
    {
        D.move (mmatrix.XL - 40, y + d);
        D.drawstring (_label [i], 0);
    }
    x = mmatrix.XL + mmatrix.DX / 2;
    y += mmatrix.DY;
    for (i = 1; i <= 16; i++)
    {
        sprintf (s, "%d", i);
        D.move (x, y + d);
        D.drawstring (s, 0);
        x += mmatrix.DX;
    }
    D.setcolor (Colors.midi_gr2);
    D.move (mmatrix.XL, mmatrix.YT);
    D.rdraw (0, _ys - 2 * mmatrix.YT);
    y = mmatrix.YT;
    D.move (mmatrix.XR, y);
    D.rdraw (_xs - 2 * mmatrix.XR, 0);
    D.setcolor (XftColors.midi_fg);
    D.move (10, y + d);
    D.drawstring ("Keyboards", -1);
    y += _nkeybd * mmatrix.DY;
    D.setcolor (Colors.midi_gr2);
    D.move (mmatrix.XR, y);
    D.rdraw (_xs - 2 * mmatrix.XR, 0);
    D.setcolor (XftColors.midi_fg);
    D.move (10, y + d);
    D.drawstring ("Division controls", -1);
    y += _ndivis * mmatrix.DY;
    D.setcolor (Colors.midi_gr2);
    D.move (mmatrix.XR, y);
    D.rdraw (_xs - 2 * mmatrix.XR, 0);
    D.setcolor (XftColors.midi_fg);
    D.move (10, y + d);
    D.drawstring ("Global controls", -1);
    y += mmatrix.DY;
    D.setcolor (Colors.midi_gr2);
    D.move (mmatrix.XR, y);
    D.rdraw (_xs - 2 * mmatrix.XR, 0);
    D.setcolor (Colors.midi_gr2);
    D.move (_xs - 1, 0);
    D.rdraw (0, _ys - 1);
    D.rdraw (1 - _xs, 0);
    plot_allconn ();
}


void Midimatrix::plot_allconn (void)
{
    int i, m;

    for (i = 0; i < 16; i++)
    {
        m = _chconf [i];
        if (m & 0x1000) plot_conn (i, m & 7);
        if (m & 0x2000) plot_conn (i, _nkeybd + ((m >> 8) & 7));
        if (m & 0x4000) plot_conn (i, _nkeybd + _ndivis);
    }
}


void Midimatrix::plot_conn (int x, int y)
{
    X_draw D (dpy (), win (), dgc (), 0);

    if (y < _nkeybd)                 D.setcolor (Colors.midi_bg ^ Colors.midi_co1);
    else if (y < _nkeybd + _ndivis)  D.setcolor (Colors.midi_bg ^ Colors.midi_co2);
    else                             D.setcolor (Colors.midi_bg ^ Colors.midi_co3);
    D.setfunc (GXxor);
    x = mmatrix.XL + x * mmatrix.DX + 5;
    y = mmatrix.YT + y * mmatrix.DY + 5;
    D.fillrect (x, y, x + mmatrix.DX - 9, y + mmatrix.DY - 9);
}


void Midimatrix::bpress (XButtonEvent *E)
{
    unsigned int i, j, k, x, y;

    i = (E->x - mmatrix.XL) / mmatrix.DX;
    j = (E->y - mmatrix.YT) / mmatrix.DY;
    x = E->x - mmatrix.XL - 4 - i * mmatrix.DX;
    y = E->y - mmatrix.YT - 4 - j * mmatrix.DY;
    if ((i > 15) || ((int) j > _nkeybd + _ndivis) || (x > mmatrix.DX - 2) || (y > mmatrix.DY - 2)) return;
    _chan = i;

    if ((int) j < _nkeybd)
    {
        k = (_chconf [i] & 0x1000) ? (_chconf [i] & 7) : 8;
        _chconf [i] &= 0x6700;
        if (k != j)
        {
            _chconf [i] |= 0x1000 | j;
            if (k < 8) plot_conn (i, k);
        }
        plot_conn (i, j);
    }
    else if ((int) j < _nkeybd + _ndivis)
    {
        j -= _nkeybd;
        k = (_chconf [i] & 0x2000) ? ((_chconf [i] >> 8) & 7) : 8;
        _chconf [i] &= 0x5007;
        if (k != j)
        {
            _chconf [i] |= 0x2000 | (j << 8);
            if (k < 8) plot_conn (i, k + _nkeybd);
        }
        plot_conn (i, j + _nkeybd);
    }
    else
    {
        _chconf [i] ^= 0x4000;
        plot_conn (i, _nkeybd + _ndivis);
    }

    if (_callb) _callb->handle_callb (CB_MIDI_MODCONF, this, 0);
}


