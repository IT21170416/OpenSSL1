#!/usr/bin/perl
# Written by Matt Caswell for the OpenSSL project.
# ====================================================================
# Copyright (c) 1998-2015 The OpenSSL Project.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. All advertising materials mentioning features or use of this
#    software must display the following acknowledgment:
#    "This product includes software developed by the OpenSSL Project
#    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
#
# 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
#    endorse or promote products derived from this software without
#    prior written permission. For written permission, please contact
#    openssl-core@openssl.org.
#
# 5. Products derived from this software may not be called "OpenSSL"
#    nor may "OpenSSL" appear in their names without prior written
#    permission of the OpenSSL Project.
#
# 6. Redistributions of any form whatsoever must retain the following
#    acknowledgment:
#    "This product includes software developed by the OpenSSL Project
#    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
#
# THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
# EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
# ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
# ====================================================================
#
# This product includes cryptographic software written by Eric Young
# (eay@cryptsoft.com).  This product includes software written by Tim
# Hudson (tjh@cryptsoft.com).

use strict;
use TLSProxy::Proxy;

my $proxy = TLSProxy::Proxy->new(
    \&vers_tolerance_filter,
    @ARGV
);

#Test 1: Asking for TLS1.3 should pass
my $client_version = TLSProxy::Record::VERS_TLS_1_3;
$proxy->start();
TLSProxy::Message->success or die "FAILED: Version tolerance test\n";

#Test 2: Testing something below SSLv3 should fail
$client_version = TLSProxy::Record::VERS_SSL_3_0 - 1;
$proxy->restart();
TLSProxy::Message->success and die "FAILED: Version tolerance test\n";

print "SUCCESS: Version tolerance test\n";

sub vers_tolerance_filter
{
    my $proxy = shift;

    # We're only interested in the initial ClientHello
    if ($proxy->flight != 0) {
        return;
    }

    foreach my $message (@{$proxy->message_list}) {
        if ($message->mt == TLSProxy::Message::MT_CLIENT_HELLO) {
            #Set the client version
            #Anything above the max supported version (TLS1.2) should succeed
            #Anything below SSLv3 should fail
            $message->client_version($client_version);
            $message->repack();
        }
    }
}
