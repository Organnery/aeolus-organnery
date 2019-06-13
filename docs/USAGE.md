# Running Aeolus

## Run-time configuration

Aeolus takes run-time options from three sources:

*  the file `/etc/aeolus.conf`
*  the file `~/.aeolusrc`
*  command line options

Apart from empty lines and comments (lines starting with '#')
either of the files should just contain the command line options  
you want to use, on a single line (examples given below).

If the file in the home directory exists (even empty) then the
one in /etc is not used. Options given on the command line override
those given in either file.

Command line options are:

(audio interface)

```
  -J     Use JACK. This is also the default. This option 
         can be used to override -A in the config files.

  -A     Use ALSA. Aeolus should work with the "default"
         device, but in recent ALSA releases this has a
         very large buffer size, and this may result in 
         excessive latency. Use of a hardware device and
         the options below is recommended. 

         Sub-options for ALSA and their defaults are:

         -d <device>             (default)
         -r <sample rate>        (48000)
         -p <period size>        (1024)
         -n <number of periods>  (2) 
```

(output format)


```
  -B     This options selects direct Ambisionics first order
         B-format output, to be used for recording or with
         an external decoder. The default is stereo output.
```

(resources)

```
  -S  <stops directory>

         The default is ./stops.

  -I  <instrument directory>

         This is relative to the stops directory.
         The default is 'Aeolus'.
            
  -W  <waves directory> 

         This is relative to the stops directory.
         The default is 'waves'. This options mainly exists
         for use during development and should not be used.

  -u     This option is for use with binary distributions
         only. When used, the presets file will be stored
         into the user's home directory instead of within
         the system wide instrument directory.
```

(general)

```
  -t     Selects the text mode user interface. With this
         option Aeolus does not in any way depend on X11.
         In the current version the text mode UI is just
         an empty stub and should not be used.

  -h     Prints version information and a summary of all
         command line options. 
```

For example, if you always use Aeolus with ALSA device hw:0,
using 3 periods of 512 frames and a sample rate of 44.1 kHz,
and you have copied the stops to your home directory, then
your `~/.aeolusrc` file could look like this:

```
# Aeolus default options
-A -d hw:0 -n 2 -p 512 -r 44100 -S /home/login/stops-0.3.0
```

where 'login' is your login name.

## Running with ALSA and PulseAudio

On a desktop PC with PulseAudio installed, Aeolus can get
direct access to a soundcard using `pasuspender` as follows.

For example, where the ALSA soundcard device is `hw:0` and
the 'stops' package from Debian is installed:
```
pasuspender -- aeolus -A -d hw:0 -S /usr/share/aeolus/stops/
```

## MIDI connections

You can connect MIDI keyboards and pedalboards to Aeolus
using `qjackctl` on the desktop, or a command line tool such
as `aconnect`.

When using `qjackctl` with ALSA MIDI, _Enable ALSA Sequencer
support_ must be checked in the Setup -> Misc tab of the
`qjackctl` GUI in order for the ALSA tab to appear.

On the command line, `aconnect -i` lists MIDI inputs to the
ALSA sequencer (keyboards and MIDI adaptors). To search for
a particular item of hardware, such as a MidiSport adaptor,
you can combine `aconnect` with `grep` as follows:

```
aconnect -i | grep MidiSport
```

The command above returns the port number of the MidiSport
hardware:
```
client 24: 'MidiSport 1x1' [type=kernel,card=2]
    0 'MidiSport 1x1 MIDI 1'
```

The `aconnect -o` command lists MIDI output connections
from the sequencer (e.g. instruments):
```
aconnect -o | grep aeolus
```

The command above returns the port number of Aeolus:
```
client 128: 'aeolus' [type=user,pid=22239]
```

Now we can connect MIDI hardware on port 24 and software
instrument on port 128 with the command:
```
aconnect 24:0 128:0
```
