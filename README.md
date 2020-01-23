# tonegen

A flexible ALSA tone generator for Raspberry Pi and other 
embedded Linux applications.

v1.0a

## What is this?

`tonegen` is a simple utility to create musical tones and noises. It may,
perhaps, be useful for testing audio equipment, but I mostly wrote it as
a way of producing audible warning and feedback tones in 
embedded Linux robotics projects. 
It's completely self-contained -- `tonegen` outputs directly to an ALSA
device, so you don't need any other player. It's written entirely in
ANSI-standard C, and only uses about 100kB of memory when
running. It builds and runs on a Raspberry Pi, among other systems.
`tonegen` uses fewer system resources than decoding and playing an
MP3 file, and you have the option to create hundreds of different
sounds, if necessary. For example, you could play tones of gradually 
increasing pitch as the robot gets closer to some goal.

With a bit of ingenuity, `tonegen` can make a whole raft of irritating
chirps, sqwarks, and warbles from the days of 8-bit computing ;)

## Examples

    $ tonegen --list "tone 100,500 quiet 100 tone 100,600"
Two-tone warning beep

    $ tonegen  --list "tone 20,1000 quiet 15 tone \
      20,1000 quiet 15 tone 20,1000 quiet 15 \
      tone 20,1000 quiet 15 tone 20,1000"

A short, warbling tone

    $ tonegen --list "tone 100,500 quiet 100 tone 100,600 \
      random 400,50,500,2000 sweep 500,300,2000 buzz 300,2000

R2D2 ;)

    $ tonegen  --wave 1 --volume 100 --tone 160,50

A low, harsh buzzing noise, of the kind you might hear if you
press a switch that doesn't do anything. Play it at full volume

## Command line

### -b,--buzz D,F

Play an irritating squawk of frequency approximately F Hz for
duration D milliseconds. It's really more of a "squawk" than a 
buzz, but `-s` was already taken, for 'sweep'. At frequencies
around 1000 Hz, the sound is like that which you might hear on 70s
sci-fi movies, to indicate that a computer is doing something.

### -d,--device={device}

Sets the ALSA device. The default is "default". Use, for example,
`aplay -L` to get a list of ALSA devices.

### -l,--list "..."

The list command can play a list of all the same sounds as the single
operations. The format is

    sound args... sound_type args...

The number of args depends on the `sound_type`, and is the same as when 
playing the individual sounds. For example, instead of doing

    --tone 1000,440

you could do

    --list "tone 1000,440"

instead. Indivudual arguments can be separated with whitespace (including
end-of-line) or
commas, and these can be mixed to make the command line more comprehensible.
If using spaces, you need to be a bit careful to avoid the shell splitting
up the command.

If the `--list` argument is give as `--list -`, then the list of 
sounds is read from standard input. See note below about
script input.

### -n,--noise D

Play while noise for D milliseconds

### --q,quiet D

Play silence for D milliseconds

### --r,random D,D2,F1,F2

Play a sequence of random pitches between F1 and F2 Hz, each for D2
msec, to a total of D msec. See
`--wave` for setting the waveform.

### --t,--tone D,F

Play a constant tone for D milliseconds, of pitch F Hz. See
`--wave` for setting the waveform.

Play silence for D milliseconds

### --v,--volume={0..100}

Set the volume level, from 0-100%. See the note on 'volume' below.

### --w,--wave={0,1}

Set the waveform -- 0 for sine (the default), 1 for square. This applies
to sounds that are actually tones -- you can't change the waveform
of noise.

### --w,--sweep D,F1,F2

Play a frequency sweep over D msec, from F1 to F2 Hz

## RC file

Any of the command line arguments can be specified in the file
`$HOME/.tonegen.rc`. Probably the only argument that is useful to
set is the ALSA device:

    device=pulse

for example.

## Notes

### Volume

The `--volume` switch sets the volume, from 0-100%. However, it only 
sets it relative to the maximum volume produced by this program --
it doesn't change the system volume at the sound device.

### Audio format

`tonegen` outputs at 48kHz, 16-bits per sample, stereo. Both stereo
channels output the same samples.

### Efficiency

On a Raspberry Pi or similar, `tonegen` uses about 5% CPU when it is
running. I've made very little effort to improve the efficiency
-- there's a lot of floating-point math, and the sine tone uses 
the floating-point `sin()` function. There are many ways that 
efficiency could be improved -- a lot of the math could be done
with integers, for example. A sine lookup table could be pre-computed.
And so on.

### Using the tone generator

All the interesting, and useful, material in this utility is in the
file `tonegen.c`. The other twenty or so C files are mostly concerned
with parsing the command line and input data. It would be easy enough
to extract `tonegen.c` -- which has few dependencies -- and use
it in other applications. 

### Script input

An interesting and lightweight way to extend the capabilities of
`tonegen` is to generate input to it using a shell script. Any program
could be used, in fact, but even the most cut-down embedded system
probably has a shell. If you give the switch `--list -`, then you
can feed a list of sound specifications into `tonegen` from standard
input. So you could do

    $ my_script.sh | tonegen

See the `sample.sh` file for an example of a simple script.

You can waste any about of time, experimenting to find the most
interesting combinations of sounds.

There's no limit on the amount of data that can be fed into
`tonegen` this way, except the memory limit of the host system.
However, bear in mind that `tonegen` won't start to produce
output until the script sending data has completed. The method can't
reliably be used, for example, to make sounds as arbitrary times.

## Legal and copyright

`tonegen` is copyright (c)2015-202 Kevin Boone, and distributed under
the terms of the GNU Public Licence, v3.0. Essentiially,
you may do whatever you
like with this software, provided the original author continues
to be acknowledged. There is no warranty of any kind. 


