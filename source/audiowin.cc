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

#include <math.h>
#include "guiscale.h"
#include "audiowin.h"
#include "callbacks.h"
#include "styles.h"


Audiowin::Audiowin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm) :
    X_window (parent, xp, yp, 200, 100, Colors.main_bg),
    _callb (callb),
    _xresm (xresm),
    _xp (xp),
    _yp (yp)
{
    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);
}


Audiowin::~Audiowin (void)
{
}


void Audiowin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case ClientMessage:
        handle_xmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Audiowin::handle_xmesg (XClientMessageEvent *E)
{
    if (E->message_type == _atom) x_unmap ();
}


void Audiowin::handle_callb (int k, X_window *W, XEvent *E)
{
    int c;

    switch (k)
    {
        case SLIDER | X_slider::MOVE:
        case SLIDER | X_slider::STOP:
        {
            X_slider *X = (X_slider *) W;
            c = X->cbid ();
            _asect = (c >> ASECT_BIT0) - 1;
            _parid = c & ASECT_MASK;
            _value = X->get_val ();
            _final = k == (X_callback::SLIDER | X_slider::STOP);
            _callb->handle_callb (CB_AUDIO_ACT, this, E);
            break;
    }
    }
}


void Audiowin::setup (M_ifc_init *M)
{
    int      i, j, k, x;
    char     s [256];
    Asect    *S;
    X_hints  H;

    //enum { XOFFS = AUscale(90), XSTEP = AUscale(215), YSIZE = AUscale(330) };
	int XOFFS =  AUscale(90);
	int XSTEP = AUscale(215);
	int YSIZE = AUscale(330);

    but1.size.x = 20;
    but1.size.y = 20;
    _nasect = M->_nasect;
    for (i = 0; i < _nasect; i++)
    {
    	// draw sliders for each division
		S = _asectd + i;
        x = XOFFS + XSTEP * i;
        k = ASECT_STEP * (i + 1);

        // define and draw horizontal fader
        (S->_slid [0] = new  X_hslider (this, this, &sli1, &sca_azim, x,  AUscale(40), AUscale(20), k + 0))->x_map ();
        (S->_slid [1] = new  X_hslider (this, this, &sli1, &sca_difg, x,  AUscale(75), AUscale(20), k + 1))->x_map ();
        (S->_slid [2] = new  X_hslider (this, this, &sli1, &sca_dBsh, x, AUscale(110), AUscale(20), k + 2))->x_map ();
        (S->_slid [3] = new  X_hslider (this, this, &sli1, &sca_dBsh, x, AUscale(145), AUscale(20), k + 3))->x_map ();
        (S->_slid [4] = new  X_hslider (this, this, &sli1, &sca_dBsh, x, AUscale(180), AUscale(20), k + 4))->x_map ();

        // draw horizontal scale
        (new X_hscale (this, &sca_azim, x,  AUscale(30), AUscale(10)))->x_map ();
        (new X_hscale (this, &sca_difg, x,  AUscale(65), AUscale(10)))->x_map ();
        (new X_hscale (this, &sca_dBsh, x, AUscale(133), AUscale(10)))->x_map ();
        (new X_hscale (this, &sca_dBsh, x, AUscale(168), AUscale(10)))->x_map ();

        // add division name on top of faders
        S->_label [0] = 0;
        for (j = 0; j <  M->_ndivis; j++)
		{
		    if (M->_divisd [j]._asect == i)
		    {
			if (S->_label [0]) strcat (S->_label, " + ");
	                strcat (S->_label, M->_divisd [j]._label);
	                add_text (x, AUscale(5), AUscale(200), AUscale(20), S->_label, &text0);
		    }
		}
    }

    // add fader labels on the left
    add_text ( AUscale(10),  AUscale(40), AUscale(60), AUscale(20), "Azimuth", &text0);
    add_text ( AUscale(10),  AUscale(75), AUscale(60), AUscale(20), "Width",   &text0);
    add_text ( AUscale(10), AUscale(110), AUscale(60), AUscale(20), "Direct ", &text0);
    add_text ( AUscale(10), AUscale(145), AUscale(60), AUscale(20), "Reflect", &text0);
    add_text ( AUscale(10), AUscale(180), AUscale(60), AUscale(20), "Reverb",  &text0);

    // define and draw Common reverb and volume faders
    (_slid [0] = new  X_hslider (this, this, &sli1, &sca_dBsh, AUscale(520), AUscale(275), AUscale(20), 0))->x_map ();
    (_slid [1] = new  X_hslider (this, this, &sli1, &sca_size,  AUscale(70), AUscale(240), AUscale(20), 1))->x_map ();
    (_slid [2] = new  X_hslider (this, this, &sli1, &sca_trev,  AUscale(70), AUscale(275), AUscale(20), 2))->x_map ();
    (_slid [3] = new  X_hslider (this, this, &sli1, &sca_spos, AUscale(305), AUscale(275), AUscale(20), 3))->x_map ();

    // draw horizontal scale
    (new X_hscale (this, &sca_size,  AUscale(70), AUscale(230), AUscale(10)))->x_map ();
    (new X_hscale (this, &sca_trev,  AUscale(70), AUscale(265), AUscale(10)))->x_map ();
    (new X_hscale (this, &sca_spos, AUscale(305), AUscale(265), AUscale(10)))->x_map ();
    (new X_hscale (this, &sca_dBsh, AUscale(520), AUscale(265), AUscale(10)))->x_map ();

    // add fader labels
    add_text ( AUscale(10), AUscale(240), AUscale(50), AUscale(20), "Delay",    &text0);
    add_text ( AUscale(10), AUscale(275), AUscale(50), AUscale(20), "Time",     &text0);
    add_text (AUscale(135), AUscale(305), AUscale(60), AUscale(20), "Reverb",   &text0);
    add_text (AUscale(355), AUscale(305), AUscale(80), AUscale(20), "Position", &text0);
    add_text (AUscale(570), AUscale(305), AUscale(60), AUscale(20), "Volume",   &text0);

    // define windows title
    sprintf (s, "Audio settings");
    x_set_title (s);

    H.position (_xp, _yp);
    H.minsize (200, 100);
    H.maxsize (XOFFS + _nasect * XSTEP, YSIZE);
    H.rname (_xresm->rname ());
    H.rclas (_xresm->rclas ());
    x_apply (&H);
    x_resize (XOFFS + _nasect * XSTEP, YSIZE);
}


void Audiowin::set_aupar (M_ifc_aupar *M)
{
    if (M->_asect < 0)
    {
        if ((M->_parid >= 0) && (M->_parid < 4))
        {
            _slid [M->_parid]->set_val (M->_value);
        }
    }
    else if (M->_asect < _nasect)
    {
        if ((M->_parid >= 0) && (M->_parid < 5))
        {
            _asectd [M->_asect]._slid [M->_parid]->set_val (M->_value);
        }
    }
}


void Audiowin::add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style)
{
    (new X_textln (this, style, xp, yp, xs, ys, text, -1))->x_map ();
}
