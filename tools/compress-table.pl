#!/usr/bin/perl -w

sub convert {
    $s = shift;
    $s = "ENC_".uc($s);
    $s =~ s/-/_/g;
    return $s;
}

open TABLE, "table";

$encodings = "";

while (<TABLE>) {
    if (/^(0x[0-9a-fA-F]+)\s+([^:]*):(0x[0-9a-fA-F]+)/) {
	($u, $e) = ($1, $2);

	$u = oct($u);
	
	if (!defined $start) {
	    $start = $u;
	    $old_u = $u;
	    $encodings = convert($e);
	    $end = $u;
	} elsif ($old_u ne $u) {
	    if (!defined $old_encodings) {
		$old_encodings = $encodings;
	    } elsif ($old_encodings ne $encodings || $old_u != $end + 1) {
		     
		printf "{ %#x, %#x, $old_encodings },\n", $start, $end;
		$start = $old_u;
		$old_encodings = $encodings;
	    }
	    $end = $old_u;
	    $encodings = convert($e);
	    $old_u = $u;
	} else {
	    $encodings .= "|".convert($e);
	}
    }
}

close TABLE;
