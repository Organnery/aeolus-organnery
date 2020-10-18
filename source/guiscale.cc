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


#include <stdio.h>
#include <string.h>
#include <math.h>
#include "guiscale.h"

static float getenv_float (char const *name, float def)
{
    float tmp = def;
    char *s;

    s = getenv(name);
    if (s != NULL)
	{
        sscanf(s, "%f", &tmp);
		if ( tmp < 1.0f ) tmp = 1;
	}

    printf("getenv %s=\"%s\" (%f)\n", name, s, tmp);
    return tmp;
}

float AEOLUS_MAIN_WINDOW_SCALING = getenv_float("AEOLUS_MAIN_WINDOW_SCALING", 1.0f);
float AEOLUS_AUDIO_WINDOW_SCALING = getenv_float("AEOLUS_AUDIO_WINDOW_SCALING", 1.0f);
float AEOLUS_MIDI_WINDOW_SCALING = getenv_float("AEOLUS_MIDI_WINDOW_SCALING", 1.0f);
float AEOLUS_INSTR_WINDOW_SCALING = getenv_float("AEOLUS_INSTR_WINDOW_SCALING", 1.0f);
float AEOLUS_EDIT_WINDOW_SCALING = getenv_float("AEOLUS_EDIT_WINDOW_SCALING", 1.0f);

int MAscale (int std_size)
{
	return round(std_size*AEOLUS_MAIN_WINDOW_SCALING);
}

int AUscale (int std_size)
{
	return round(std_size*AEOLUS_AUDIO_WINDOW_SCALING);
}

int MIscale (int std_size)
{
	return round(std_size*AEOLUS_MIDI_WINDOW_SCALING);
}

int INscale (int std_size)
{
	return round(std_size*AEOLUS_INSTR_WINDOW_SCALING);
}

int EDscale (int std_size)
{
	return round(std_size*AEOLUS_EDIT_WINDOW_SCALING);
}
