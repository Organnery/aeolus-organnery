// ----------------------------------------------------------------------------
//
//  Copyright (C) 2019-2019 Raphaël Mouneyres <raphael@audiotronic.fr>
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

#ifndef __GUISCALE_H
#define __GUISCALE_H


extern float AEOLUS_MAIN_WINDOW_SCALING;
extern float AEOLUS_AUDIO_WINDOW_SCALING;
extern float AEOLUS_MIDI_WINDOW_SCALING;
extern float AEOLUS_INSTR_WINDOW_SCALING;
extern float AEOLUS_EDIT_WINDOW_SCALING;

extern int MAscale (int std_size);
extern int AUscale (int std_size);
extern int MIscale (int std_size);
extern int INscale (int std_size);
extern int EDscale (int std_size);

#endif
