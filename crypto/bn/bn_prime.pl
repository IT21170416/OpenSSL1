#! /usr/bin/env perl
# Copyright 1998-2018 The OpenSSL Project Authors. All Rights Reserved.
#
# Licensed under the Apache License 2.0 (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.openssl.org/source/license.html

# Output year depends on the year of the script.
my $YEAR = [localtime([stat($0)]->[9])]->[5] + 1900;
print <<"EOF";
/*
 * WARNING: do not edit!
 * Generated by crypto/bn/bn_prime.pl
 *
 * Copyright 1998-$YEAR The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

EOF


my $num = shift || 2048;
my @primes = ( 2 );
my $p = 1;
loop: while ($#primes < $num-1) {
    $p += 2;
    my $s = int(sqrt($p));

    for (my $i = 0; defined($primes[$i]) && $primes[$i] <= $s; $i++) {
        next loop if ($p % $primes[$i]) == 0;
    }
    push(@primes, $p);
}

print "typedef unsigned short prime_t;\n";
printf "# define NUMPRIMES %d\n\n", $num;

printf "static const prime_t primes[%d] = {", $num;
for (my $i = 0; $i <= $#primes; $i++) {
    printf "\n   " if ($i % 8) == 0;
    printf " %5d,", $primes[$i];
}
print "\n};\n";
