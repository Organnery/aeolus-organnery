# Aeolus Midi controls

| Midi Channel# | Msg# | Values | Description | Input | Output |
| -------- | -------- | -------- | -------- | -------- | -------- |
| midi matrix "Divisons" | CC 7    | 0-127 | Swell     | Y | Y |
| midi matrix "Divisons" | CC 12   | 0-127 | Tremulant frequency | Y | Y |
| midi matrix "Divisons" | CC 13   | 0-127 | Tremulant amplitude  | Y | Y |
| -------- | -------- | -------- | -------- | -------- |--------- |
| midi matrix "Keyboards" | CC 14   | 0-127 |  Division Azimuth   | Y | Y |
| midi matrix "Keyboards" | CC 15   | 0-127 |  Division Width     | Y | Y |
| midi matrix "Keyboards" | CC 16   | 0-127 |  Division Direct    | Y | Y |
| midi matrix "Keyboards" | CC 17   | 0-127 |  Division Reflect   | Y | Y |
| midi matrix "Keyboards" | CC 18   | 0-127 |  Division Reverb    | Y | Y |
| -------- | -------- | -------- | -------- | -------- | -------- |
| midi matrix "Control" | CC 20   | 0-127 | Reverb delay    | Y | Y |
| midi matrix "Control" | CC 21   | 0-127 | Reverb time     | Y | Y |
| midi matrix "Control" | CC 22   | 0-127 | Reverb position | Y | Y |
| midi matrix "Control" | CC 23   | 0-127 | Master volume   | Y | Y |
| -------- | CC24-27 | --------  | **reserved** | -------- | -------- |
| midi matrix "Control" | CC 28   | any | Cancel | Y | Y |
| midi matrix "Control" | CC 29   | any | Tutti  | Y | Y |
| midi matrix "Control" | CC 30   | any | Transpose | Y | Y |
| midi matrix "Control" | CC 31   | any | Crescendo | see note(2) | see note(2) |
| midi matrix "Control" | CC 32   | 0-32 | Bank Select     | Y | todo |
| midi matrix "Control" | CC 33   | 0-32 | Preset recall   | Y | todo |
| midi matrix "Control" | CC 34   | 0-32 | Preset store    | Y | todo |
| midi matrix "Control" | CC 35   | any | Preset previous  | Y | todo |
| midi matrix "Control" | CC 36   | any | Preset next      | Y | todo |
| -------- | -------- | -------- | -------- | -------- | -------- |
| midi matrix "Division"  | CC 37   | 0-32  | Division recall  | todo | todo |
| -------- | -------- | -------- | -------- | -------- | -------- |
| midi matrix "Control" | CC 64   | off=<64<on |  sustain    | Y | -- |
| midi matrix "Control" | CC 98   | see manual | stops control | Y | see note(1) |
| midi matrix "Control" | CC 120  | 0 |  all sound off    | Y | --|
| midi matrix "Control" | CC 123  | 0 |  all notes off    | Y | -- |
| -------- | -------- | -------- | -------- | -------- | -------- |
| midi matrix "Control" | PC      | 0-32 | recall preset# in current bank | Y | todo |
| -------- | -------- | -------- | -------- | -------- | -------- |

# (1) Stops output messages
* when a stop goes on  : send a NOTE_ON event with the stop rank as note number, using the midi channel from the keyboard where the stop is attached
* when a stop goes off : send a NOTE_OFF event with the stop rank as note number, using the midi channel from the keyboard where the stop is attached

# (2) Crescendo
* for now crescendo is defined with a set of preset stored in memory slots XX to XX. An external filter can be configured to recall these presets from an incoming CC or set of PC messages.
