/*
 * Copyright 2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

/* Internal tests for the Opentls library context */

#include "internal/cryptlib.h"
#include "testutil.h"

/*
 * Everything between BEGIN EXAMPLE and END EXAMPLE is copied from
 * doc/internal/man3/opentls_ctx_get_data.pod
 */

/*
 * ======================================================================
 * BEGIN EXAMPLE
 */

typedef struct foo_st {
    int i;
    void *data;
} FOO;

static void *foo_new(OPENtls_CTX *ctx)
{
    FOO *ptr = OPENtls_zalloc(sizeof(*ptr));
    if (ptr != NULL)
        ptr->i = 42;
    return ptr;
}
static void foo_free(void *ptr)
{
    OPENtls_free(ptr);
}
static const OPENtls_CTX_METHOD foo_method = {
    foo_new,
    foo_free
};

/*
 * END EXAMPLE
 * ======================================================================
 */

static int test_context(OPENtls_CTX *ctx)
{
    FOO *data = NULL;

    return TEST_ptr(data = opentls_ctx_get_data(ctx, 0, &foo_method))
        /* OPENtls_zalloc in foo_new() initialized it to zero */
        && TEST_int_eq(data->i, 42);
}

static int test_app_context(void)
{
    OPENtls_CTX *ctx = NULL;
    int result =
        TEST_ptr(ctx = OPENtls_CTX_new())
        && test_context(ctx);

    OPENtls_CTX_free(ctx);
    return result;
}

static int test_def_context(void)
{
    return test_context(NULL);
}

int setup_tests(void)
{
    ADD_TEST(test_app_context);
    ADD_TEST(test_def_context);
    return 1;
}
