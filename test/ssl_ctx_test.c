/*
 * Copyright 2018 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Opentls license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#include "testutil.h"
#include <opentls/tls.h>

typedef struct {
    int min_version;
    int max_version;
    int min_ok;
    int max_ok;
    int expected_min;
    int expected_max;
} version_test;

static const version_test version_testdata[] = {
    /* min           max             ok    expected min    expected max */
    {0,              0,              1, 1, 0,              0},
    {TLS1_VERSION,   TLS1_2_VERSION, 1, 1, TLS1_VERSION,   TLS1_2_VERSION},
    {TLS1_2_VERSION, TLS1_2_VERSION, 1, 1, TLS1_2_VERSION, TLS1_2_VERSION},
    {TLS1_2_VERSION, TLS1_1_VERSION, 1, 1, TLS1_2_VERSION, TLS1_1_VERSION},
    {7,              42,             0, 0, 0,              0},
};

static int test_set_min_max_version(int idx_tst)
{
    tls_CTX *ctx = NULL;
    tls *tls = NULL;
    int testresult = 0;
    version_test t = version_testdata[idx_tst];

    ctx = tls_CTX_new(TLS_server_method());
    if (ctx == NULL)
        goto end;

    tls = tls_new(ctx);
    if (tls == NULL)
        goto end;

    if (!TEST_int_eq(tls_CTX_set_min_proto_version(ctx, t.min_version), t.min_ok))
        goto end;
    if (!TEST_int_eq(tls_CTX_set_max_proto_version(ctx, t.max_version), t.max_ok))
        goto end;
    if (!TEST_int_eq(tls_CTX_get_min_proto_version(ctx), t.expected_min))
        goto end;
    if (!TEST_int_eq(tls_CTX_get_max_proto_version(ctx), t.expected_max))
        goto end;

    if (!TEST_int_eq(tls_set_min_proto_version(tls, t.min_version), t.min_ok))
        goto end;
    if (!TEST_int_eq(tls_set_max_proto_version(tls, t.max_version), t.max_ok))
        goto end;
    if (!TEST_int_eq(tls_get_min_proto_version(tls), t.expected_min))
        goto end;
    if (!TEST_int_eq(tls_get_max_proto_version(tls), t.expected_max))
        goto end;

    testresult = 1;

  end:
    tls_free(tls);
    tls_CTX_free(ctx);
    return testresult;
}

int setup_tests(void)
{
    ADD_ALL_TESTS(test_set_min_max_version, sizeof(version_testdata) / sizeof(version_test));
    return 1;
}
