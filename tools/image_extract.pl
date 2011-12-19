#!/usr/bin/perl

# needs "convert" from the ImageMagic Package

# Usage:
# image_extract.pl IMAGEFORMAT SKIPFACTOR
# e.g. image_extract.pl jpg 3.3  extract images with 10 hz from a 33 hz image stream as jpegs
#
# Typical application:
# kogmo_rtdb_play_nodb -i file.avi -n left_image -n right_image -X 'image_extract.pl jpg 3.3'
#

($format,$nskip,$dummy)=@ARGV;
$nskip=1 if $nskip < 1;
while(<STDIN>) {
 chomp;
 if (/^([^ ]+) (.) ([^ ]+) ([^ ]+) (.*)$/) {
  $time=$1; $what=$2; $size=$3; $tid=$4; $name=$5;
  $ret = read(STDIN,$data,$size);
  if ( $ret != $size ) {
    die "ERROR: Reading data: $!,";
  }
  #DEBUG: print "$time,$what,$size,$tid,$name\n";

  if ( $what eq "." && $tid == "0xA20030" ) { # todo: use int()/unpack()?
    #print "got image $name\n";
    $nr{$name}++;
    $n=$nr{$name};
    $nn=$n/$nskip;
    $nni=int($nn);
    if ( $nni < $last_n{$name} ) {
      print "skipping $n. $name ($nni/$nn)\n";
    } else {
      $last_n{$name}=$nni+1;
      $file=$name.".".($fnr{$name}++).".".$format;
      print "writing  $n. $name ($nni/$nn) as $file\n";
      open(PIPE,"|convert - $file");
      if ( $size == 307248 ) {           # HACK!!! Assumption: Image Size => Format  TODO: Extract resolution from header!
       print PIPE "P5\n640 480 255\n";
      } elsif ( $size == 921648 ) {
       print PIPE "P6\n640 480 255\n";
      } else {
       die "ERROR: unknown image format,";
      }
      print PIPE substr($data,48); # 48 == image header length
      close(PIPE);
    }
  }
  
 } else {
   die "ERROR: Unexpected input line: $_,";
 }
}
