# Aeolus instrument definition file explanation

```
# always start with this line to open creation of a new instrument
/instr/new

# define tuning in Hertz
# second number is the instrument temperament number as defined in scales.cc
/tuning 440.0 5

# Keyboards
# create the number of keyboards you need
# maximum is defined by NKEYBD in global.h

# The string at the end of the line is used as a reference in this file for the divisions, it must be unique
# this will be the keyboard 0
/manual/new   II
# this will be the keyboard 1
/manual/new   I
# there can be only one pedal
/pedal/new    P

# Divisions
# create the number of divisions you need 
# not necessarily equal to the number of keyboards

# first string is the division label
# second number is the keyboard number, starting at 0 (?)
# third number is the audio section number, starting at 1 (?)
/divis/new    II       1  1

# first letter is the pan for the audio engine (A->_pan)
# it can be L=left, C=center, R=right, W=wide
# second number is delay in milliseconds (A->_del)
# third string is the wavetable filename to be used for this rank (and its corresponding stop)
/rank         C  10  I_principal_16.ae0

# add as many ranks as you need, maximum is 32 per division
/rank         C  10  I_principal_8.ae0
/rank         L  23  rohrflute8.ae0
/rank         L  16  I_octave_4.ae0
/rank         C  21  spitzflote4.ae0
/rank         R  19  I_quinte_223.ae0
/rank         L  31  superoctave2.ae0
/rank         C  21  I_quinte_113.ae0
/rank         L  18  I_mixturIV.ae0
/rank         W   0  I_trumpet.ae0
/rank         R  27  new_oboe_fa.ae0

# add /swell if you want a swell control for this division
/swell

# add /tremul if you want a tremulant for this division
# first number is the default frequency (betweem 2 and 8)
# second number is the  default amplitude (between 0 and 0.6)
/tremul       5.0  0.3

# close the division definition
/divis/end

# create another keyboard division
/divis/new    I        2  2
/rank         C  20  bourdon16.ae0
/rank         C  20  bourdon16.ae0
/rank         C  13  celesta.ae0
/rank         R   8  flute4.ae0
/rank         C  25  nasard.ae0
/rank         L  13  III_cymbel.ae0
/swell
/divis/end

# create the pedal division
/divis/new    P        3  3
/rank         C  20  bourdon16.ae0
/rank         L  10  P_octave_2.ae0
/rank         W  10  trombone.ae0
/divis/end

# Interface groups
# maximum number of groups is defined in global.h (8)
# groups allow you to:
# - group a keyboard with a number of stops
# - create groups of stops which can control various ranks, even from different divisions
# this will set the layout of buttons for the GUI

# /group first string is the label displayed on the GUI on the left of each group
/group/new    ManII

# /stop first number is the keyboard number as defined on top of this file, default is 0
#  button color: 0=dark_blue (keyboard rank?) 1,2,3=dark_green (division rank?)
# /stop second number is the division number where this group is attached, starting at 1
# /stop third number is the rank number for this division, sequential number starting at 1
/stop         0   1   1
/stop         0   1   2
/stop         0   1   3
/stop         0   1   4
/stop         0   1   5
/stop         0   1   6
/stop         0   1   7
/stop         0   1   8
/stop         0   1   9
/stop         0   1  10
/stop         0   1  11

# if a tremulant is defined in the division you will create the stop here
# /tremul first number is the division number where this tremulant is active, starting at 1
# /tremul button color is light_green by default
# second string is the mnemonic
# third string is the label for GUI
/tremul       2       TR       Tremulant

# end group definition
/group/end

# create another group
/group/new    ManI
/stop         0   2   1
/stop         0   2   2
/stop         0   2   3
/stop         0   2   4
/stop         0   2   5
/stop         0   2   6

# /coupler add a keyboard coupler
#  firt number is the manual number to be coupled
#  second number is the manual number to be master
#  third string is the mnemonic
#  fourth string is the label for the GUI button
/coupler      2   1   C1+2     I+II
/group/end

# create another group for pedal
/group/new    Pedal
/stop         0   3   1
/stop         0   3   2
/stop         0   3   3
/coupler      3   2   CP+1     P+I
/coupler      3   1   CP+2     P+II
/group/end

# end instrument definition
/instr/end
```
