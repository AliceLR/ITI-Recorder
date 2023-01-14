#!/bin/sh

if [ "$#" != "1" ] && [ "$#" != "2" ]; then
	echo "usage: ./ju06adump.sh PORT [OUTFILE]"
	echo
	echo "Sends SysEx data request commands to a Roland Boutique JU-06A"
	echo "device on MIDI port 'PORT' and outputs the resulting dump to"
	echo "'OUTFILE' (or dumps hex to stdout if no filename). The USB MIDI"
	echo "interface should be preferred over the 5-pin DIN interface."
	echo
	echo "This is a thin wrapper around amidi(1) from the ALSA utils."
	echo
	exit 0
fi

HEAD="f041100000006211"

LFO="$HEAD 03000600 00000008 6ff7 "
DCO="$HEAD 03000700 00000012 64f7 "
VCF="$HEAD 03000800 0000000e 67f7 "
VCA="$HEAD 03000900 00000004 70f7 "
ENV="$HEAD 03000a00 00000008 6bf7 "
FX1="$HEAD 03001000 0000000c 61f7 "
FX2="$HEAD 03001100 0000000c 60f7 "
NAM="$HEAD 03001300 00000010 5af7 "

REQUEST="$LFO""$DCO""$VCF""$VCA""$ENV""$FX1""$FX2""$NAM"

if [ "$#" = "2" ]; then
	echo "amidi -t1 -p'$1'" "-S'$REQUEST'" "-r'$2'"
	amidi -t1 -p"$1" -S"$REQUEST" -r"$2"
else
	echo "amidi -t1 -p'$1'" "-S'$REQUEST'" "-d"
	amidi -t1 -p"$1" -S"$REQUEST" -d
fi
