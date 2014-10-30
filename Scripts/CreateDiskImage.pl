#!/usr/bin/perl
#
# SD-Card Disk Image Creator
# ===========================================================================
# (c)2014 by Lucky Resistor. http://luckyresistor.me
# Licensed under the MIT license. See file LICENSE for details.
#

use v5.16.0;
use strict;
use warnings;
use IO::Seekable;
use IO::Dir;
use IO::File;
use Getopt::Long;
use File::stat;
use File::Spec;

# This script creates a disk image from a number of files in a directory. 
# The files in this directory should have short names and ideally no space
# or other special characters in the name. Actually the script doesn't care
# and just use the UTF-8 encoded file name.
#
# All files in this directory are aligned in the disk image, and a directory
# block is written in front of it. The block size is fixed to 512bytes, which
# is the default for modern SD-Cards.
#
# The directory block must never exceed 512 bytes. If it does, the script 
# will fail with an error message. 512 bytes is a huge amount of data to hold
# in RAM for a microcontroller.
#
# An example directory could look like this:
#
# example/
#        /a1.txt
#        /s1.snd
#        /test.bin
#
# You would start the script like this:
#
#   CreateDiskImage.pl -i example -o example_image.bin
#
# It will convert the directory with all the files into the disk image 
# called "example_image.bin". 
#
# To white this disk image to an SD Card, use the "dd" command.
# WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
# Be ABSOLUTELY SURE you write to the right device. Otherwise you will
# destroy any data on your harddisk or any other disk you accidentally 
# write to.
# WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
#
# To find the device, use for example "diskutil list" on Mac, or have a 
# look in the message log on a Linux system.
#
# To write the data on the device use this dd command:
#
#   sudo dd if=example_image.bin of=/dev/diskXX bs=512
#
# Use the correct device instead of "/dev/diskXX" and CHECK EVERY TIME
# if you are writing to the right device. 
#
# Format
# ---------------------------------------------------------------------------
# 
# The format of the disk image is the following one:
#
# Block 0 - File Directory:
#   4 Bytes Identifier: 0x48, 0x43, 0x44, 0x49 = "HCDI"
#   For each file: 
#     4 Bytes start block Little-Endian. 0 = End of Directory
#     4 Bytes file size in bytes Little-Endian.
#     1 Byte file name length in bytes.
#     n Bytes file name in ASCII format.
#   Rest of block filled with 0x00 bytes.
# Block 1... - Files. Last Block always filled with 0x00 bytes.
#

# Configuration
# ---------------------------------------------------------------------------
my $confMagic = "HCDI";
my $confBlockSize = 512;

# Options
# ---------------------------------------------------------------------------
my $optInputDirectory;
my $optOutput;

# Main
# ---------------------------------------------------------------------------
GetOptions( "input|i=s" => \$optInputDirectory,
			"output|o=s" => \$optOutput,)
	or die( "Error reading commands line parameters.");

print "  Create Disk Image\n";
print "-" x 78 . "\n";

# Open the input directory
my $inputDirectory = IO::Dir->new($optInputDirectory)
	or die("Could not open input directory.");
	
# Read all files from it.
my $file;
my @files = ();
my $nextBlock = 1;
while (defined($file = $inputDirectory->read)) {
	next if $file =~ /^\.{1,2}$/; # Skip . and ..
	my $filePath = File::Spec->catfile($optInputDirectory, $file);
	my $st = stat($filePath)
		or die("Could not check attributes of file: $filePath");
	my $fileSize = $st->size();
	if ($fileSize < 1) {
		die("Found file with size < 1 byte.");
	}
	my $fileStartBlock = $nextBlock;
	print "File \"$file\" size=$fileSize bytes startBlock=$fileStartBlock\n";
	push(@files, {"name"=>$file, "size"=>$fileSize, "startBlock"=>$fileStartBlock});
	# Search for the next block.
	$nextBlock += ($fileSize / 512) + 1;
}
undef $inputDirectory;

# A quick check if we have at least one file.
if (@files < 1) {
	die("There are no files to write into the image.");
}

# Build the image
print "Writing disk image...\n";
my $outFile = IO::File->new($optOutput, ">:raw") or 
	die("Could not open output file \"$optOutput\" for write.");

# Start with the magic.
$outFile->print($confMagic);
# write all file entries.
foreach my $fileEntry (@files) {
	my $fileName = $fileEntry->{"name"};
	my $fileSize = $fileEntry->{"size"};
	my $startBlock = $fileEntry->{"startBlock"};
	$outFile->print(pack("VVCA*", $startBlock, $fileSize, length($fileName), $fileName));
}
# Add at least 4 zero bytes.
$outFile->print(pack("V", 0));

# Check the size of the header.
if ($outFile->tell > $confBlockSize) {
	die("Header is too large. Header is " . $outFile->getpos . ", but block size is " . $confBlockSize);
}

# Fill with zero
while ($outFile->tell < $confBlockSize) {
	$outFile->print(pack("x"));
}

# Copy all files.
foreach my $fileEntry (@files) {
	my $fileName = $fileEntry->{"name"};
	my $filePath = File::Spec->catfile($optInputDirectory, $fileName);
	print "Writing file $fileName ...\n";
	my $inFile = IO::File->new($filePath, "<:raw")
		or die("Could not open input file $fileName for reading.");
	my $buffer;
	while ($inFile->read($buffer, 4096) > 0) {
		$outFile->write($buffer);
	};
	# fill with padding bytes
	while (($outFile->tell % $confBlockSize) != 0) {
		$outFile->print(pack("x"));
	}
	$inFile->close();	
}

$outFile->close();

print "\nDisk image was successfully written.\n\n";

# ===========================================================================
# END
#




