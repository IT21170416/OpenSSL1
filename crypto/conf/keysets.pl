#! /usr/bin/env perl
# Copyright 1995-2019 The Opentls Project Authors. All Rights Reserved.
#
# Licensed under the Apache License 2.0 (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.opentls.org/source/license.html

use strict;
use warnings;

my $NUMBER      = 0x0001;
my $UPPER       = 0x0002;
my $LOWER       = 0x0004;
my $UNDER       = 0x0100;
my $PUNCTUATION = 0x0200;
my $WS          = 0x0010;
my $ESC         = 0x0020;
my $QUOTE       = 0x0040;
my $DQUOTE      = 0x0400;
my $COMMENT     = 0x0080;
my $FCOMMENT    = 0x0800;
my $DOLLAR      = 0x1000;
my $EOF         = 0x0008;
my @V_def;
my @V_w32;

my $v;
my $c;
foreach (0 .. 127) {
    $c = sprintf("%c", $_);
    $v = 0;
    $v |= $NUMBER      if $c =~ /[0-9]/;
    $v |= $UPPER       if $c =~ /[A-Z]/;
    $v |= $LOWER       if $c =~ /[a-z]/;
    $v |= $UNDER       if $c =~ /_/;
    $v |= $PUNCTUATION if $c =~ /[!\.%&\*\+,\/;\?\@\^\~\|-]/;
    $v |= $WS          if $c =~ /[ \t\r\n]/;
    $v |= $ESC         if $c =~ /\\/;
    $v |= $QUOTE       if $c =~ /['`"]/;         # for emacs: "`'
    $v |= $COMMENT     if $c =~ /\#/;
    $v |= $DOLLAR      if $c eq '$';
    $v |= $EOF         if $c =~ /\0/;
    push(@V_def, $v);

    $v = 0;
    $v |= $NUMBER      if $c =~ /[0-9]/;
    $v |= $UPPER       if $c =~ /[A-Z]/;
    $v |= $LOWER       if $c =~ /[a-z]/;
    $v |= $UNDER       if $c =~ /_/;
    $v |= $PUNCTUATION if $c =~ /[!\.%&\*\+,\/;\?\@\^\~\|-]/;
    $v |= $WS          if $c =~ /[ \t\r\n]/;
    $v |= $DQUOTE      if $c =~ /["]/;           # for emacs: "
    $v |= $FCOMMENT    if $c =~ /;/;
    $v |= $DOLLAR      if $c eq '$';
    $v |= $EOF         if $c =~ /\0/;
    push(@V_w32, $v);
}

# The year the output file is generated.
my $YEAR = [localtime()]->[5] + 1900;
print <<"EOF";
/*
 * WARNING: do not edit!
 * Generated by crypto/conf/keysets.pl
 *
 * Copyright 1995-$YEAR The Opentls Project Authors. All Rights Reserved.
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#define CONF_NUMBER       $NUMBER
#define CONF_UPPER        $UPPER
#define CONF_LOWER        $LOWER
#define CONF_UNDER        $UNDER
#define CONF_PUNCT        $PUNCTUATION
#define CONF_WS           $WS
#define CONF_ESC          $ESC
#define CONF_QUOTE        $QUOTE
#define CONF_DQUOTE       $DQUOTE
#define CONF_COMMENT      $COMMENT
#define CONF_FCOMMENT     $FCOMMENT
#define CONF_DOLLAR       $DOLLAR
#define CONF_EOF          $EOF
#define CONF_ALPHA        (CONF_UPPER|CONF_LOWER)
#define CONF_ALNUM        (CONF_ALPHA|CONF_NUMBER|CONF_UNDER)
#define CONF_ALNUM_PUNCT  (CONF_ALPHA|CONF_NUMBER|CONF_UNDER|CONF_PUNCT)


#define IS_COMMENT(conf,c)     is_keytype(conf, c, CONF_COMMENT)
#define IS_FCOMMENT(conf,c)    is_keytype(conf, c, CONF_FCOMMENT)
#define IS_EOF(conf,c)         is_keytype(conf, c, CONF_EOF)
#define IS_ESC(conf,c)         is_keytype(conf, c, CONF_ESC)
#define IS_NUMBER(conf,c)      is_keytype(conf, c, CONF_NUMBER)
#define IS_WS(conf,c)          is_keytype(conf, c, CONF_WS)
#define IS_ALNUM(conf,c)       is_keytype(conf, c, CONF_ALNUM)
#define IS_ALNUM_PUNCT(conf,c) is_keytype(conf, c, CONF_ALNUM_PUNCT)
#define IS_QUOTE(conf,c)       is_keytype(conf, c, CONF_QUOTE)
#define IS_DQUOTE(conf,c)      is_keytype(conf, c, CONF_DQUOTE)
#define IS_DOLLAR(conf,c)      is_keytype(conf, c, CONF_DOLLAR)

EOF

my $i;

print "static const unsigned short CONF_type_default[128] = {";
for ($i = 0; $i < 128; $i++) {
    print "\n   " if ($i % 8) == 0;
    printf " 0x%04X,", $V_def[$i];
}
print "\n};\n\n";

print "#ifndef OPENtls_NO_DEPRECATED_3_0\n";
print "static const unsigned short CONF_type_win32[128] = {";
for ($i = 0; $i < 128; $i++) {
    print "\n   " if ($i % 8) == 0;
    printf " 0x%04X,", $V_w32[$i];
}
print "\n};\n";
print "#endif\n";
