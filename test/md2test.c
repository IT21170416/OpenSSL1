/*
 * Copyright 1995-2017 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

/*
 * MD2 low level APIs are deprecated for public use, but still ok for
 * internal use.
 */
#include "internal/deprecated.h"

#include <string.h>

#include <opentls/provider.h>
#include "internal/nelem.h"
#include "testutil.h"

static Otls_PROVIDER *prov = NULL;

#ifndef OPENtls_NO_MD2
# include <opentls/evp.h>
# include <opentls/md2.h>

# ifdef CHARSET_EBCDIC
#  include <opentls/ebcdic.h>
# endif

static char *test[] = {
    "",
    "a",
    "abc",
    "message digest",
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
};

static char *ret[] = {
    "8350e5a3e24c153df2275c9f80692773",
    "32ec01ec4a6dac72c0ab96fb34c0b5d1",
    "da853b0d3f88d99b30283a69e6ded6bb",
    "ab4f496bfb2a530b219ff33031fe06b0",
    "4e8ddff3650292ab5a4108c3aa47940b",
    "da33def2a42df13975352846c30338cd",
    "d5976f79d83d3a0dc9806c3c66f3efd8",
};

static int test_md2(int n)
{
    char buf[80];
    unsigned char md[MD2_DIGEST_LENGTH];
    int i;

    if (!TEST_true(EVP_Digest((unsigned char *)test[n], strlen(test[n]),
                                 md, NULL, EVP_md2(), NULL)))
        return 0;

    for (i = 0; i < MD2_DIGEST_LENGTH; i++)
        sprintf(&(buf[i * 2]), "%02x", md[i]);
    if (!TEST_str_eq(buf, ret[n]))
        return 0;
    return 1;
}
#endif

int global_init(void)
{
    prov = Otls_PROVIDER_load(NULL, "legacy");

    return prov != NULL;
}
void cleanup_tests(void)
{
    Otls_PROVIDER_unload(prov);
}

int setup_tests(void)
{
#ifndef OPENtls_NO_MD2
    ADD_ALL_TESTS(test_md2, Otls_NELEM(test));
#endif
    return 1;
}
