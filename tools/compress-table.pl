#!/usr/bin/perl -w

sub convert {
    $s = shift;
    $s = "ENC_".uc($s);
    $s =~ s/-/_/g;
    return $s;
}

$combo_index = 1;

sub add {
    my $combo = shift;
    if (!exists($combos{$combo})) {
	$combos{$combo} = $combo_index++;
	printf "  $combo,\n", $combos{$combo};
    }
}

my $col = 0;

sub output {
    my ($start,$u,$index) = @_;
    
    for (my $i = $start; $i < $u; $i++) {
	print "  0,";
	$col = ($col + 1) % 16;
	if ($col == 0) {
	    print "\n";
	}
    }
    printf " %2d,", $index;
    $col = ($col + 1) % 16;
    if ($col == 0) {
	print "\n";
    }
}

print "const guint32 char_mask_map[] = {\n  0,\n";

open TABLE, "table";

$encodings = "";

while (<TABLE>) {
    if (/^(0x[0-9a-fA-F]+)\s+([^:]*):(0x[0-9a-fA-F]+)/) {
	($u, $e) = ($1, $2);

	$u = oct($u);

	if (!defined $old_u) {
	    $old_u = $u;
	    $encodings = convert($e);
	} elsif ($old_u ne $u) {
	    add($encodings);
	    $old_u = $u;
	    $encodings = convert($e);
	} else {
	    $encodings .= "|".convert($e);
	}
    }
}

if (defined $old_u) {
    add($encodings);
} 

close TABLE;

print <<EOF;
};

const guchar char_masks[] = {
EOF

open TABLE, "table";

$encodings = "";

undef $old_u;
$start = 0;
while (<TABLE>) {
    if (/^(0x[0-9a-fA-F]+)\s+([^:]*):(0x[0-9a-fA-F]+)/) {
	($u, $e) = ($1, $2);

	$u = oct($u);

	if (!defined $old_u) {
	    $old_u = $u;
	    $encodings = convert($e);
	} elsif ($old_u ne $u) {
	    output($start, $old_u, $combos{$encodings});
	    $start = $old_u + 1;
	    $old_u = $u;
	    $encodings = convert($e);
	} else {
	    $encodings .= "|".convert($e);
	}
    }
}

if (defined $old_u) {
    output($start, $old_u, $combos{$encodings});
} 

close TABLE;

print "\n};\n";
