/*
 * Copyright 2019-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * This implemments a dummy key manager for legacy KDFs that still support the
 * old way of performing a KDF via EVP_PKEY_derive(). New KDFs should not be
 * implemented this way. In reality there is no key data for such KDFs, so this
 * key manager does very little.
 */

#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include "prov/implementations.h"
#include "prov/providercommon.h"
#include "prov/provider_ctx.h"
#include "prov/kdfexchange.h"

static OSSL_FUNC_keymgmt_new_fn kdf_newdata;
static OSSL_FUNC_keymgmt_free_fn kdf_freedata;
static OSSL_FUNC_keymgmt_has_fn kdf_has;

KDF_DATA *kdf_data_new(void *provctx)
{
    KDF_DATA *kdfdata;

    if (!ossl_prov_is_running())
        return NULL;

    kdfdata = OPENSSL_zalloc(sizeof(*kdfdata));
    if (kdfdata == NULL)
        return NULL;

    kdfdata->lock = CRYPTO_THREAD_lock_new();
    if (kdfdata->lock == NULL) {
        OPENSSL_free(kdfdata);
        return NULL;
    }
    kdfdata->libctx = PROV_LIBRARY_CONTEXT_OF(provctx);
    kdfdata->refcnt = 1;

    return kdfdata;
}

void kdf_data_free(KDF_DATA *kdfdata)
{
    int ref = 0;

    if (kdfdata == NULL)
        return;

    CRYPTO_DOWN_REF(&kdfdata->refcnt, &ref, kdfdata->lock);
    if (ref > 0)
        return;

    CRYPTO_THREAD_lock_free(kdfdata->lock);
    OPENSSL_free(kdfdata);
}

int kdf_data_up_ref(KDF_DATA *kdfdata)
{
    int ref = 0;

    if (!ossl_prov_is_running())
        return 0;

    CRYPTO_UP_REF(&kdfdata->refcnt, &ref, kdfdata->lock);
    return 1;
}

static void *kdf_newdata(void *provctx)
{
    return kdf_data_new(provctx);
}

static void kdf_freedata(void *kdfdata)
{
    kdf_data_free(kdfdata);
}

static int kdf_has(void *keydata, int selection)
{
    return 0;
}

const OSSL_DISPATCH kdf_keymgmt_functions[] = {
    { OSSL_FUNC_KEYMGMT_NEW, (void (*)(void))kdf_newdata },
    { OSSL_FUNC_KEYMGMT_FREE, (void (*)(void))kdf_freedata },
    { OSSL_FUNC_KEYMGMT_HAS, (void (*)(void))kdf_has },
    { 0, NULL }
};
