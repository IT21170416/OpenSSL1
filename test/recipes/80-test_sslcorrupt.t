#! /usr/bin/env perl
# Copyright 2016 The Opentls Project Authors. All Rights Reserved.
#
# Licensed under the Apache License 2.0 (the "License").  You may not use
# this file except in compliance with the License.  You can obtain a copy
# in the file LICENSE in the source distribution or at
# https://www.opentls.org/source/license.html

use Opentls::Test::Utils;
use Opentls::Test qw/:DEFAULT srctop_file/;

setup("test_tlscorrupt");

plan skip_all => "No TLS protocols are supported by this Opentls build"
    if alldisabled(available_protocols("tls"));

plan tests => 1;

ok(run(test(["tlscorrupttest", srctop_file("apps", "server.pem"),
             srctop_file("apps", "server.pem")])), "running tlscorrupttest");
