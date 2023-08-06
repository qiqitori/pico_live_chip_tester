#!/usr/bin/perl

# Usage: perl check_debug_log.pl terminal_log_file

for $i (0..65535) {
    $mem[$i] = 2;
}

while (<>) {
    if (/write (\d) to (\d+)/) {
        $val = $1;
        $add = $2;
        $mem[$add] = $val;
    } elsif (/read (\d) from (\d+)/) {
        $val = $1;
        $add = $2;
        if ($mem[$add] != 2) {
            if ($mem[$add] == $val) {
                print "Match: $add == $val\n"
            } else {
                print "Error: $add != $val\n"
            }
        }
    }
}
