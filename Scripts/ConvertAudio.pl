#!/usr/bin/perl
#
# Convert WAV into Binary
# ===========================================================================
# (c)2014 by Lucky Resistor. http://luckyresistor.me
# Licensed under the MIT license. See file LICENSE for details.
#

use strict;
use warnings;

# Small perl script to convert a input file into the right binary format
# for playing with the AudioPlayer class.
#
# This script is using the command "sox": http://sox.sourceforge.net
# Install it using e.g. MacPorts on the Mac, or using the package
# manager on Linux.
#
# Usage: 
#   ConvertAudio.pl Input.wav Output.bin
#

my ($inputFile, $outputFile) = @ARGV;

if (!defined $inputFile || !defined $outputFile) {
	die( "Usage: ConvertAudio.pl <input file> <output file>\n" );
}

system("sox -S \"$inputFile\" -b 16 -L -c 1 -e unsigned-integer -t raw \"$outputFile\"") == 0
	or die( "Could not execute the \"sox\" command." );

# ===========================================================================	
# END
#

