# Aeolus internals

## The stops directory

The aeolus binary itself is a generic organ synth and
does not define any instrument. In order to use Aeolus
you need a directory containing definitions of stops
and of an instrument. The one supplied with the current
release has this structure:

```
 stops
     |
     |___ Aeolus
     |        |          
     |        |____ definition
     |        |____ presets
     |
     |___ Aeolus1
     |        |          
     |        |____ definition
     |        |____ presets
     |
     |___ Aeolus2
     |        |          
     |        |____ definition
     |        |____ presets
     |
     |___ ***.ae0
     |___ ***.ae0 
     |___ ***.ae0
     |___ ...
     |
     |___ waves
              |
              |___ ***.ae1
              |___ ***.ae1
              |___ ***.ae1
              |___ ...
```

The 'Aeolus' directory is the default instrument
directory. It contains two files: a 'definition'
file that specifies the layout of the organ, and
'presets' that contains registration and MIDI presets.

There can be more than one instrument definition
directory (in fact there are two more examples
in the release stops-0.3.0). These can be selected
with the `-I` command line option followed by the
instrument name.

The data in 'presets' only makes sense for one
particular instrument, and that's why these files
are kept together with the corresponding instrument
definition. For binary distributions, there is a
configuration option that will make Aeolus save the
presets in the users's home directory. This will allow
the user to save presets for one instrument at a time.

The *.ae0 files contain parameters for the additive
synthesis. There is one such file for each rank of
pipes in the organ. These are binary files and they
should not be edited. 'Power users' can use the built-in
synthesis editor to modify them.
  
The waves directory will be empty initially. When 
Aeolus starts up for the first time, it will compute
wavetables, one for each pipe. This is indicated by the
flashing stop buttons. The same will happen whenever the
tuning or temperament is changed.

These wavetables can be saved (so Aeolus will be
ready for use much faster next time), but only if the stops
directory is writeable for the user. This will not be the
case for a binary installation as the stops dir will be
system-wide (e.g. /usr/share/aeolus/stops).

In order to be able to save wavetables or edited stops
the stops directory must be copied to a location where
it can be modified by the user, (e.g. ~/stops-0.3.0).

## MIDI control of stop buttons

The protocol uses one controller number. The default is #98, but you
can change this in global.h. The message is accepted only on channels
enabled for control in the MIDI matrix.

The value is interpreted as follows:
```
  v = 01mm0ggg 

      This type of messages (bit 6 set) selects a group, and either
      resets all stops in that group or sets the mode for the second
      form below.
      
      mm = mode. This can be:
    
         00  disabled, also resets all elements in the group.
         01  set off
         10  set on
         11  toggle

      ggg = group, one of the button groups as defined in the instrument
      definition. In the GUI groups start at the top, the first one (for
      division III) being group #0.

      The values of mm and ggg are stored and need not be repeated unless
      they change.

  v = 000bbbbb

      According to the current state of mode, this command switches a
      stop on or off, or toggles its state, or does nothing at all.
      
      bbbbb = button index within the group.

      Buttons are numbered left to right, top to bottom within each
      group. The first one is #0.
```

## Virtual divisions

Aeolus can support 'virtual divisions' which do not relate
to a specific keyboard or row in the GUI, but can be defined
as part of one or more other divisions and be mapped to
keyboards there. A virtual division can have a separate MIDI
channel for swell, tremulant speed and depth.
 
For example, Division IV is defined in the definition file
for the instrument Aeolus1:
```
# Divisions
#
/divis/new    IV       0  1
/rank         W   0  I_trumpet.ae0
/rank         R  27  new_oboe_fa.ae0
/rank         W  24  cromhorne.ae0
/swell
/divis/end
```

Division IV is mapped to MIDI channel 5 for control purposes
as seen in the Divisions rows of the MIDI window grid.
However, sending MIDI notes to this instrument on channel 5
results in no sound at all.

Instead, these three stops Trumpet, Oboe and Krumhorn in 
Division IV are defined in the 'Interface groups' of the 
definition file as part of Division III (the dark green 
stops in the second row of the GUI, corresponding to the
second group of 1,2,3 in the right column of the
definition file):
```
/group/new    III
/stop         0   2   1
/stop         0   2   2
/stop         0   2   3
/stop         0   2   4
/stop         0   2   5
/stop         0   2   6
/stop         0   2   7
/stop         0   2   8
/stop         0   2   9
/stop         0   2  10
/stop         1   1   1
/stop         1   1   2
/stop         1   1   3
/tremul       2       TR       Tremulant
/group/end
```

They are also part of Division I in the same way:
```
/group/new    I
/stop         0   4   1
/stop         0   4   2
/stop         0   4   3
/stop         0   4   4
/stop         0   4   5
/stop         0   4   6
/stop         0   4   7
/stop         0   4   8
/stop         0   4   9
/stop         0   4  10
/stop         0   4  11
/stop         0   4  12
/stop         0   4  13
/stop         3   1   1
/stop         3   1   2
/stop         3   1   3
/coupler      3   3   C1+2     I+II
/coupler      3   2   C1+3     I+III
/group/end
```

These stops are triggered by sending MIDI notes to the
channel numbers of the divisions they are configured to be
part of, not to the number of their control channel.
