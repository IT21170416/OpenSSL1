/*
 * Copyright 2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

/* Dispatch functions for Seed cipher modes ecb, cbc, ofb, cfb */

#include "cipher_seed.h"
#include "prov/implementations.h"

static Otls_OP_cipher_freectx_fn seed_freectx;
static Otls_OP_cipher_dupctx_fn seed_dupctx;

static void seed_freectx(void *vctx)
{
    PROV_SEED_CTX *ctx = (PROV_SEED_CTX *)vctx;

    OPENtls_clear_free(ctx,  sizeof(*ctx));
}

static void *seed_dupctx(void *ctx)
{
    PROV_SEED_CTX *in = (PROV_SEED_CTX *)ctx;
    PROV_SEED_CTX *ret = OPENtls_malloc(sizeof(*ret));

    if (ret == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        return NULL;
    }
    *ret = *in;

    return ret;
}

/* seed128ecb_functions */
IMPLEMENT_generic_cipher(seed, SEED, ecb, ECB, 0, 128, 128, 0, block)
/* seed128cbc_functions */
IMPLEMENT_generic_cipher(seed, SEED, cbc, CBC, 0, 128, 128, 128, block)
/* seed128ofb128_functions */
IMPLEMENT_generic_cipher(seed, SEED, ofb128, OFB, 0, 128, 8, 128, stream)
/* seed128cfb128_functions */
IMPLEMENT_generic_cipher(seed, SEED, cfb128,  CFB, 0, 128, 8, 128, stream)
