# Aeolus instrument definition file explanation (up to version 0.9.10)

# always start with this line to open creation of a new instrument
/instr/new

# define tuning in Hertz
# second number is the instrument temperament number as defined in scales.cc
/tuning 440.0 5

# Keyboards
# create the number of keyboards you need
# maximum is defined by NKEYBD in global.h, default is 6
# keyboard order will define the layout for the gui from top to bottom in default horinzontal mode, or left to right in vertical mode

# The string at end of line is used as a reference in this file for the divisions, it must be unique
# this will be the keyboard 1
/manual/new   II
# this will be the keyboard 2
/manual/new   I
# there can be only one pedal, here will be keyboard 3
/pedal/new    P

# Divisions
# create the number of divisions you need 
# not necessarly equal to number of keyboards

#              v- division label for the GUI
#              v       v-- keyboard number (0 means no keyboard attached)
#              v       v  v-- audio section number, starting at 1 (?)
/divis/new    II       1  1

#             v- pan for the audio engine (A->_pan), can be L=left, C=center, R=right, W=wide
#             v   v- delay (A->_del)
#             v   v  v-- wavetable filename to be used for this rank
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
#             v-- default frequency (between 2 and 8)
#             v    v-- default amplitude (between 0 and 0.6)
/tremul       5.0  0.3

# close the division definition
/divis/end

#create more divisions
/divis/new    I        2  2
/rank         C  20  bourdon16.ae0
/rank         C  20  bourdon16.ae0
/rank         C  13  celesta.ae0
/rank         R   8  flute4.ae0
/rank         C  25  nasard.ae0
/rank         L  13  III_cymbel.ae0
/swell
/divis/end

/divis/new    P        3  3
/rank         C  20  bourdon16.ae0
/rank         L  10  P_octave_2.ae0
/rank         W  10  trombone.ae0
/divis/end

# Interface groups
# maximum number of groups is defined in global.h (default 8)
# groups allows you to :
# - assign a keyboard to play a set of stops
# - create groups of stops which can control various ranks, even from various divisions
# this will set the layout of buttons for the GUI

# /group first string is the label displayed on the GUI on the left of each group
/group/new    ManII

# add stops to be part of this group
#             v-- rank type grouping, default to 0
#             v      0  = division rank, button color is dark blue, played when coupled
#             v      >0 = keyboard rank, button color is dark green, not played when coupled
#             v            warning : keyboard rank numbers must be different for each group
#             v   v-- division number where this group is attached, starting at 1
#             v   v   v-- sequential number starting at 1
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
# tremulant button color is light_green by default
#             v-- division number where this tremulant is active, starting at 1
#             v       v-- mnemonic (8 characters max)
#             v       v        v-- label for gui (32 characters max)
/tremul       2       TR       Tremulant

# end group definition
/group/end

# add more groups
/group/new    ManI      
/stop         0   2   1
/stop         0   2   2
/stop         0   2   3
/stop         0   2   4
/stop         0   2   5
/stop         0   2   6

# /coupler add a keyboard coupler
#             v-- keyboard number to be coupled (slave)
#             v   v-- keyboard number to be played (master)
#             v   v   v-- mnemonic (8 characters max)
#             v   v   v       v-- label for the GUI button (32 characters max)
/coupler      2   1   C1+2     I+II
/group/end

/group/new    Pedal      
/stop         0   3   1
/stop         0   3   2
/stop         0   3   3
/coupler      3   2   CP+1     P+I
/coupler      3   1   CP+2     P+II
/group/end

# end instrument definition
/instr/end