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

#
# Read in the maps
#
my @codepoints = ();

opendir (MAPS, "maps") || die "Cannot open maps/ subdirectory: $!\n";
while (defined (my $map = readdir (MAPS))) {
    next if ($map =~ /^\./);
    next if ($map =~ /~$/);
    next if ($map =~ /^CVS|README$/);

    open (MAP, "maps/$map") || die "Cannot open map '$map:!\n";

    $encoding = convert($map);
    while (<MAP>) {
	s/\s*#.*//;
	s/\s*$//;
	next if /^$/;
	if (!/^\s*(0x[A-Fa-f0-9]+)\s+(0x[A-Fa-f0-9]+)$/) {
	    die "Cannot parse line '%s' in map '$map'\n";
	}
	push @codepoints, [hex($2), $encoding];
    }
    close (MAP);
}

# 
# And sort them
#
@codepoints = sort { $a->[0] <=> $b->[0] } @codepoints;

print "const guint32 char_mask_map[] = {\n  0,\n";

$encodings = "";

for $cp (@codepoints) {
    $u = $cp->[0]; $e = $cp->[1];

    if (!defined $old_u) {
	$old_u = $u;
	$encodings = $e;
    } elsif ($old_u ne $u) {
	add($encodings);
	$old_u = $u;
	$encodings = $e;
    } else {
	$encodings .= "|".$e;
    }
}

if (defined $old_u) {
    add($encodings);
} 

print <<EOF;
};

const guchar char_masks[] = {
EOF

$encodings = "";

undef $old_u;
$start = 0;
for $cp (@codepoints) {
    $u = $cp->[0]; $e = $cp->[1];

    if (!defined $old_u) {
	$old_u = $u;
	$encodings = $e;
    } elsif ($old_u ne $u) {
	output($start, $old_u, $combos{$encodings});
	$start = $old_u + 1;
	$old_u = $u;
	$encodings = $e;
    } else {
	$encodings .= "|".$e;
    }
}

if (defined $old_u) {
    output($start, $old_u, $combos{$encodings});
} 

print "\n};\n";
