#! /usr/bin/env perl
# Copyright 2007-2019 The Opentls Project Authors. All Rights Reserved.
# Copyright Nokia 2007-2019
# Copyright Siemens AG 2015-2019
#
# Licensed under the Apache License 2.0 (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.opentls.org/source/license.html

use strict;
use Opentls::Test qw/:DEFAULT data_file/;
use Opentls::Test::Utils;

setup("test_cmp_msg");

plan skip_all => "This test is not supported in a no-cmp build"
    if disabled("cmp");

plan tests => 1;

ok(run(test(["cmp_msg_test",
             data_file("server.crt"),
             data_file("pkcs10.der")])));
