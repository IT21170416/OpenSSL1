/*
 * Copyright 2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <assert.h>
#include <string.h>
/* For strcasecmp on Windows */
#include "e_os.h"
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "internal/param_build_set.h"
#include "openssl/param_build.h"
#include "crypto/ecx.h"
#include "prov/implementations.h"
#include "prov/providercommon.h"
#include "prov/provider_ctx.h"
#ifdef S390X_EC_ASM
# include "s390x_arch.h"
# include <openssl/sha.h>   /* For SHA512_DIGEST_LENGTH */
#endif

static OSSL_FUNC_keymgmt_new_fn x25519_new_key;
static OSSL_FUNC_keymgmt_new_fn x448_new_key;
static OSSL_FUNC_keymgmt_new_fn ed25519_new_key;
static OSSL_FUNC_keymgmt_new_fn ed448_new_key;
static OSSL_FUNC_keymgmt_gen_init_fn x25519_gen_init;
static OSSL_FUNC_keymgmt_gen_init_fn x448_gen_init;
static OSSL_FUNC_keymgmt_gen_init_fn ed25519_gen_init;
static OSSL_FUNC_keymgmt_gen_init_fn ed448_gen_init;
static OSSL_FUNC_keymgmt_gen_fn x25519_gen;
static OSSL_FUNC_keymgmt_gen_fn x448_gen;
static OSSL_FUNC_keymgmt_gen_fn ed25519_gen;
static OSSL_FUNC_keymgmt_gen_fn ed448_gen;
static OSSL_FUNC_keymgmt_gen_cleanup_fn ecx_gen_cleanup;
static OSSL_FUNC_keymgmt_load_fn ecx_load;
static OSSL_FUNC_keymgmt_get_params_fn x25519_get_params;
static OSSL_FUNC_keymgmt_get_params_fn x448_get_params;
static OSSL_FUNC_keymgmt_get_params_fn ed25519_get_params;
static OSSL_FUNC_keymgmt_get_params_fn ed448_get_params;
static OSSL_FUNC_keymgmt_gettable_params_fn x25519_gettable_params;
static OSSL_FUNC_keymgmt_gettable_params_fn x448_gettable_params;
static OSSL_FUNC_keymgmt_gettable_params_fn ed25519_gettable_params;
static OSSL_FUNC_keymgmt_gettable_params_fn ed448_gettable_params;
static OSSL_FUNC_keymgmt_set_params_fn x25519_set_params;
static OSSL_FUNC_keymgmt_set_params_fn x448_set_params;
static OSSL_FUNC_keymgmt_set_params_fn ed25519_set_params;
static OSSL_FUNC_keymgmt_set_params_fn ed448_set_params;
static OSSL_FUNC_keymgmt_settable_params_fn x25519_settable_params;
static OSSL_FUNC_keymgmt_settable_params_fn x448_settable_params;
static OSSL_FUNC_keymgmt_settable_params_fn ed25519_settable_params;
static OSSL_FUNC_keymgmt_settable_params_fn ed448_settable_params;
static OSSL_FUNC_keymgmt_has_fn ecx_has;
static OSSL_FUNC_keymgmt_match_fn ecx_match;
static OSSL_FUNC_keymgmt_import_fn ecx_import;
static OSSL_FUNC_keymgmt_import_types_fn ecx_imexport_types;
static OSSL_FUNC_keymgmt_export_fn ecx_export;
static OSSL_FUNC_keymgmt_export_types_fn ecx_imexport_types;

#define ECX_POSSIBLE_SELECTIONS (OSSL_KEYMGMT_SELECT_KEYPAIR)

struct ecx_gen_ctx {
    OPENSSL_CTX *libctx;
    ECX_KEY_TYPE type;
    int selection;
};

#ifdef S390X_EC_ASM
static void *s390x_ecx_keygen25519(struct ecx_gen_ctx *gctx);
static void *s390x_ecx_keygen448(struct ecx_gen_ctx *gctx);
static void *s390x_ecd_keygen25519(struct ecx_gen_ctx *gctx);
static void *s390x_ecd_keygen448(struct ecx_gen_ctx *gctx);
#endif

static void *x25519_new_key(void *provctx)
{
    return ecx_key_new(PROV_LIBRARY_CONTEXT_OF(provctx), ECX_KEY_TYPE_X25519, 0);
}

static void *x448_new_key(void *provctx)
{
    return ecx_key_new(PROV_LIBRARY_CONTEXT_OF(provctx), ECX_KEY_TYPE_X448, 0);
}

static void *ed25519_new_key(void *provctx)
{
    return ecx_key_new(PROV_LIBRARY_CONTEXT_OF(provctx), ECX_KEY_TYPE_ED25519, 0);
}

static void *ed448_new_key(void *provctx)
{
    return ecx_key_new(PROV_LIBRARY_CONTEXT_OF(provctx), ECX_KEY_TYPE_ED448, 0);
}

static int ecx_has(void *keydata, int selection)
{
    ECX_KEY *key = keydata;
    int ok = 0;

    if (key != NULL) {
        /*
         * ECX keys always have all the parameters they need (i.e. none).
         * Therefore we always return with 1, if asked about parameters.
         */
        ok = 1;

        if ((selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY) != 0)
            ok = ok && key->haspubkey;

        if ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0)
            ok = ok && key->privkey != NULL;
    }
    return ok;
}

static int ecx_match(const void *keydata1, const void *keydata2, int selection)
{
    const ECX_KEY *key1 = keydata1;
    const ECX_KEY *key2 = keydata2;
    int ok = 1;

    if ((selection & OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS) != 0)
        ok = ok && key1->type == key2->type;
    if ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0) {
        if ((key1->privkey == NULL && key2->privkey != NULL)
                || (key1->privkey != NULL && key2->privkey == NULL)
                || key1->type != key2->type)
            ok = 0;
        else
            ok = ok && (key1->privkey == NULL /* implies key2->privkey == NULL */
                        || CRYPTO_memcmp(key1->privkey, key2->privkey,
                                         key1->keylen) == 0);
    }
    if ((selection & OSSL_KEYMGMT_SELECT_PUBLIC_KEY) != 0) {
        if (key1->haspubkey != key2->haspubkey
                || key1->type != key2->type)
            ok = 0;
        else
            ok = ok && (key1->haspubkey == 0 /* implies key2->haspubkey == 0 */
                        || CRYPTO_memcmp(key1->pubkey, key2->pubkey,
                                         key1->keylen) == 0);
    }
    return ok;
}

static int ecx_import(void *keydata, int selection, const OSSL_PARAM params[])
{
    ECX_KEY *key = keydata;
    int ok = 1;
    int include_private = 0;

    if (key == NULL)
        return 0;

    if ((selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return 0;

    include_private = ((selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) != 0);
    ok = ok && ecx_key_fromdata(key, params, include_private);

    return ok;
}

static int key_to_params(ECX_KEY *key, OSSL_PARAM_BLD *tmpl,
                         OSSL_PARAM params[])
{
    if (key == NULL)
        return 0;

    if (!ossl_param_build_set_octet_string(tmpl, params,
                                           OSSL_PKEY_PARAM_PUB_KEY,
                                           key->pubkey, key->keylen))
        return 0;

    if (key->privkey != NULL
        && !ossl_param_build_set_octet_string(tmpl, params,
                                              OSSL_PKEY_PARAM_PRIV_KEY,
                                              key->privkey, key->keylen))
        return 0;

    return 1;
}

static int ecx_export(void *keydata, int selection, OSSL_CALLBACK *param_cb,
                      void *cbarg)
{
    ECX_KEY *key = keydata;
    OSSL_PARAM_BLD *tmpl;
    OSSL_PARAM *params = NULL;
    int ret = 0;

    if (key == NULL)
        return 0;

    tmpl = OSSL_PARAM_BLD_new();
    if (tmpl == NULL)
        return 0;

    if ((selection & OSSL_KEYMGMT_SELECT_KEYPAIR) != 0
         && !key_to_params(key, tmpl, NULL))
        goto err;

    params = OSSL_PARAM_BLD_to_param(tmpl);
    if (params == NULL)
        goto err;

    ret = param_cb(params, cbarg);
    OSSL_PARAM_BLD_free_params(params);
err:
    OSSL_PARAM_BLD_free(tmpl);
    return ret;
}

#define ECX_KEY_TYPES()                                                        \
OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_PUB_KEY, NULL, 0),                     \
OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_PRIV_KEY, NULL, 0)

static const OSSL_PARAM ecx_key_types[] = {
    ECX_KEY_TYPES(),
    OSSL_PARAM_END
};
static const OSSL_PARAM *ecx_imexport_types(int selection)
{
    if ((selection & OSSL_KEYMGMT_SELECT_KEYPAIR) != 0)
        return ecx_key_types;
    return NULL;
}

static int ecx_get_params(void *key, OSSL_PARAM params[], int bits, int secbits,
                          int size)
{
    ECX_KEY *ecx = key;
    OSSL_PARAM *p;

    if ((p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_BITS)) != NULL
        && !OSSL_PARAM_set_int(p, bits))
        return 0;
    if ((p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_SECURITY_BITS)) != NULL
        && !OSSL_PARAM_set_int(p, secbits))
        return 0;
    if ((p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_MAX_SIZE)) != NULL
        && !OSSL_PARAM_set_int(p, size))
        return 0;
    if ((p = OSSL_PARAM_locate(params, OSSL_PKEY_PARAM_TLS_ENCODED_PT)) != NULL
            && (ecx->type == ECX_KEY_TYPE_X25519
                || ecx->type == ECX_KEY_TYPE_X448)) {
        if (!OSSL_PARAM_set_octet_string(p, ecx->pubkey, ecx->keylen))
            return 0;
    }

    return key_to_params(ecx, NULL, params);
}

static int ed_get_params(void *key, OSSL_PARAM params[])
{
    OSSL_PARAM *p;

    if ((p = OSSL_PARAM_locate(params,
                               OSSL_PKEY_PARAM_MANDATORY_DIGEST)) != NULL
        && !OSSL_PARAM_set_utf8_string(p, ""))
        return 0;
    return 1;
}

static int x25519_get_params(void *key, OSSL_PARAM params[])
{
    return ecx_get_params(key, params, X25519_BITS, X25519_SECURITY_BITS,
                          X25519_KEYLEN);
}

static int x448_get_params(void *key, OSSL_PARAM params[])
{
    return ecx_get_params(key, params, X448_BITS, X448_SECURITY_BITS,
                          X448_KEYLEN);
}

static int ed25519_get_params(void *key, OSSL_PARAM params[])
{
    return ecx_get_params(key, params, ED25519_BITS, ED25519_SECURITY_BITS,
                          ED25519_KEYLEN)
        && ed_get_params(key, params);
}

static int ed448_get_params(void *key, OSSL_PARAM params[])
{
    return ecx_get_params(key, params, ED448_BITS, ED448_SECURITY_BITS,
                          ED448_KEYLEN)
        && ed_get_params(key, params);
}

static const OSSL_PARAM ecx_gettable_params[] = {
    OSSL_PARAM_int(OSSL_PKEY_PARAM_BITS, NULL),
    OSSL_PARAM_int(OSSL_PKEY_PARAM_SECURITY_BITS, NULL),
    OSSL_PARAM_int(OSSL_PKEY_PARAM_MAX_SIZE, NULL),
    OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_MANDATORY_DIGEST, NULL, 0),
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_TLS_ENCODED_PT, NULL, 0),
    ECX_KEY_TYPES(),
    OSSL_PARAM_END
};

static const OSSL_PARAM ed_gettable_params[] = {
    OSSL_PARAM_int(OSSL_PKEY_PARAM_BITS, NULL),
    OSSL_PARAM_int(OSSL_PKEY_PARAM_SECURITY_BITS, NULL),
    OSSL_PARAM_int(OSSL_PKEY_PARAM_MAX_SIZE, NULL),
    ECX_KEY_TYPES(),
    OSSL_PARAM_END
};

static const OSSL_PARAM *x25519_gettable_params(void *provctx)
{
    return ecx_gettable_params;
}

static const OSSL_PARAM *x448_gettable_params(void *provctx)
{
    return ecx_gettable_params;
}

static const OSSL_PARAM *ed25519_gettable_params(void *provctx)
{
    return ed_gettable_params;
}

static const OSSL_PARAM *ed448_gettable_params(void *provctx)
{
    return ed_gettable_params;
}

static int ecx_set_params(void *key, const OSSL_PARAM params[])
{
    ECX_KEY *ecxkey = key;
    const OSSL_PARAM *p;

    p = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_TLS_ENCODED_PT);
    if (p != NULL) {
        void *buf = ecxkey->pubkey;

        if (p->data_size != ecxkey->keylen
                || !OSSL_PARAM_get_octet_string(p, &buf, sizeof(ecxkey->pubkey),
                                                NULL))
            return 0;
        OPENSSL_clear_free(ecxkey->privkey, ecxkey->keylen);
        ecxkey->privkey = NULL;
        ecxkey->haspubkey = 1;
    }

    return 1;
}

static int x25519_set_params(void *key, const OSSL_PARAM params[])
{
    return ecx_set_params(key, params);
}

static int x448_set_params(void *key, const OSSL_PARAM params[])
{
    return ecx_set_params(key, params);
}

static int ed25519_set_params(void *key, const OSSL_PARAM params[])
{
    return 1;
}

static int ed448_set_params(void *key, const OSSL_PARAM params[])
{
    return 1;
}

static const OSSL_PARAM ecx_settable_params[] = {
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_TLS_ENCODED_PT, NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM ed_settable_params[] = {
    OSSL_PARAM_END
};

static const OSSL_PARAM *x25519_settable_params(void *provctx)
{
    return ecx_settable_params;
}

static const OSSL_PARAM *x448_settable_params(void *provctx)
{
    return ecx_settable_params;
}

static const OSSL_PARAM *ed25519_settable_params(void *provctx)
{
    return ed_settable_params;
}

static const OSSL_PARAM *ed448_settable_params(void *provctx)
{
    return ed_settable_params;
}

static void *ecx_gen_init(void *provctx, int selection, ECX_KEY_TYPE type)
{
    OPENSSL_CTX *libctx = PROV_LIBRARY_CONTEXT_OF(provctx);
    struct ecx_gen_ctx *gctx = NULL;

    if ((gctx = OPENSSL_malloc(sizeof(*gctx))) != NULL) {
        gctx->libctx = libctx;
        gctx->type = type;
        gctx->selection = selection;
    }
    return gctx;
}

static void *x25519_gen_init(void *provctx, int selection)
{
    return ecx_gen_init(provctx, selection, ECX_KEY_TYPE_X25519);
}

static void *x448_gen_init(void *provctx, int selection)
{
    return ecx_gen_init(provctx, selection, ECX_KEY_TYPE_X448);
}

static void *ed25519_gen_init(void *provctx, int selection)
{
    return ecx_gen_init(provctx, selection, ECX_KEY_TYPE_ED25519);
}

static void *ed448_gen_init(void *provctx, int selection)
{
    return ecx_gen_init(provctx, selection, ECX_KEY_TYPE_ED448);
}

static int ecx_gen_set_params(void *genctx, const OSSL_PARAM params[])
{
    struct ecx_gen_ctx *gctx = genctx;
    const OSSL_PARAM *p;

    if (gctx == NULL)
        return 0;

    p = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_GROUP_NAME);
    if (p != NULL) {
        const char *groupname = NULL;

        /*
         * We optionally allow setting a group name - but each algorithm only
         * support one such name, so all we do is verify that it is the one we
         * expected.
         */
        switch (gctx->type) {
            case ECX_KEY_TYPE_X25519:
                groupname = "x25519";
                break;
            case ECX_KEY_TYPE_X448:
                groupname = "x448";
                break;
            default:
                /* We only support this for key exchange at the moment */
                break;
        }
        if (p->data_type != OSSL_PARAM_UTF8_STRING
                || groupname == NULL
                || strcasecmp(p->data, groupname) != 0) {
            ERR_raise(ERR_LIB_PROV, ERR_R_PASSED_INVALID_ARGUMENT);
            return 0;
        }
    }

    return 1;
}

static const OSSL_PARAM *ecx_gen_settable_params(void *provctx)
{
    static OSSL_PARAM settable[] = {
        OSSL_PARAM_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, NULL, 0),
        OSSL_PARAM_END
    };
    return settable;
}

static void *ecx_gen(struct ecx_gen_ctx *gctx)
{
    ECX_KEY *key;
    unsigned char *privkey;

    if (gctx == NULL)
        return NULL;
    if ((key = ecx_key_new(gctx->libctx, gctx->type, 0)) == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        return NULL;
    }

    /* If we're doing parameter generation then we just return a blank key */
    if ((gctx->selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return key;

    if ((privkey = ecx_key_allocate_privkey(key)) == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }
    if (RAND_priv_bytes_with_libctx(gctx->libctx, privkey, key->keylen) <= 0)
        goto err;
    switch (gctx->type) {
    case ECX_KEY_TYPE_X25519:
        privkey[0] &= 248;
        privkey[X25519_KEYLEN - 1] &= 127;
        privkey[X25519_KEYLEN - 1] |= 64;
        X25519_public_from_private(key->pubkey, privkey);
        break;
    case ECX_KEY_TYPE_X448:
        privkey[0] &= 252;
        privkey[X448_KEYLEN - 1] |= 128;
        X448_public_from_private(key->pubkey, privkey);
        break;
    case ECX_KEY_TYPE_ED25519:
        if (!ED25519_public_from_private(gctx->libctx, key->pubkey, privkey))
            goto err;
        break;
    case ECX_KEY_TYPE_ED448:
        if (!ED448_public_from_private(gctx->libctx, key->pubkey, privkey))
            goto err;
        break;
    }
    key->haspubkey = 1;
    return key;
err:
    ecx_key_free(key);
    return NULL;
}

static void *x25519_gen(void *genctx, OSSL_CALLBACK *osslcb, void *cbarg)
{
    struct ecx_gen_ctx *gctx = genctx;

#ifdef S390X_EC_ASM
    if (OPENSSL_s390xcap_P.pcc[1] & S390X_CAPBIT(S390X_SCALAR_MULTIPLY_X25519))
        return s390x_ecx_keygen25519(gctx);
#endif
    return ecx_gen(gctx);
}

static void *x448_gen(void *genctx, OSSL_CALLBACK *osslcb, void *cbarg)
{
    struct ecx_gen_ctx *gctx = genctx;

#ifdef S390X_EC_ASM
    if (OPENSSL_s390xcap_P.pcc[1] & S390X_CAPBIT(S390X_SCALAR_MULTIPLY_X448))
        return s390x_ecx_keygen448(gctx);
#endif
    return ecx_gen(gctx);
}

static void *ed25519_gen(void *genctx, OSSL_CALLBACK *osslcb, void *cbarg)
{
    struct ecx_gen_ctx *gctx = genctx;
#ifdef S390X_EC_ASM
    if (OPENSSL_s390xcap_P.pcc[1] & S390X_CAPBIT(S390X_SCALAR_MULTIPLY_ED25519)
        && OPENSSL_s390xcap_P.kdsa[0] & S390X_CAPBIT(S390X_EDDSA_SIGN_ED25519)
        && OPENSSL_s390xcap_P.kdsa[0]
            & S390X_CAPBIT(S390X_EDDSA_VERIFY_ED25519))
        return s390x_ecd_keygen25519(gctx);
#endif
    return ecx_gen(gctx);
}

static void *ed448_gen(void *genctx, OSSL_CALLBACK *osslcb, void *cbarg)
{
    struct ecx_gen_ctx *gctx = genctx;

#ifdef S390X_EC_ASM
    if (OPENSSL_s390xcap_P.pcc[1] & S390X_CAPBIT(S390X_SCALAR_MULTIPLY_ED448)
        && OPENSSL_s390xcap_P.kdsa[0] & S390X_CAPBIT(S390X_EDDSA_SIGN_ED448)
        && OPENSSL_s390xcap_P.kdsa[0] & S390X_CAPBIT(S390X_EDDSA_VERIFY_ED448))
        return s390x_ecd_keygen448(gctx);
#endif
    return ecx_gen(gctx);
}

static void ecx_gen_cleanup(void *genctx)
{
    struct ecx_gen_ctx *gctx = genctx;

    OPENSSL_free(gctx);
}

void *ecx_load(const void *reference, size_t reference_sz)
{
    ECX_KEY *key = NULL;

    if (reference_sz == sizeof(key)) {
        /* The contents of the reference is the address to our object */
        key = *(ECX_KEY **)reference;
        /* We grabbed, so we detach it */
        *(ECX_KEY **)reference = NULL;
        return key;
    }
    return NULL;
}

#define MAKE_KEYMGMT_FUNCTIONS(alg) \
    const OSSL_DISPATCH alg##_keymgmt_functions[] = { \
        { OSSL_FUNC_KEYMGMT_NEW, (void (*)(void))alg##_new_key }, \
        { OSSL_FUNC_KEYMGMT_FREE, (void (*)(void))ecx_key_free }, \
        { OSSL_FUNC_KEYMGMT_GET_PARAMS, (void (*) (void))alg##_get_params }, \
        { OSSL_FUNC_KEYMGMT_GETTABLE_PARAMS, (void (*) (void))alg##_gettable_params }, \
        { OSSL_FUNC_KEYMGMT_SET_PARAMS, (void (*) (void))alg##_set_params }, \
        { OSSL_FUNC_KEYMGMT_SETTABLE_PARAMS, (void (*) (void))alg##_settable_params }, \
        { OSSL_FUNC_KEYMGMT_HAS, (void (*)(void))ecx_has }, \
        { OSSL_FUNC_KEYMGMT_MATCH, (void (*)(void))ecx_match }, \
        { OSSL_FUNC_KEYMGMT_IMPORT, (void (*)(void))ecx_import }, \
        { OSSL_FUNC_KEYMGMT_IMPORT_TYPES, (void (*)(void))ecx_imexport_types }, \
        { OSSL_FUNC_KEYMGMT_EXPORT, (void (*)(void))ecx_export }, \
        { OSSL_FUNC_KEYMGMT_EXPORT_TYPES, (void (*)(void))ecx_imexport_types }, \
        { OSSL_FUNC_KEYMGMT_GEN_INIT, (void (*)(void))alg##_gen_init }, \
        { OSSL_FUNC_KEYMGMT_GEN_SET_PARAMS, (void (*)(void))ecx_gen_set_params }, \
        { OSSL_FUNC_KEYMGMT_GEN_SETTABLE_PARAMS, \
          (void (*)(void))ecx_gen_settable_params }, \
        { OSSL_FUNC_KEYMGMT_GEN, (void (*)(void))alg##_gen }, \
        { OSSL_FUNC_KEYMGMT_GEN_CLEANUP, (void (*)(void))ecx_gen_cleanup }, \
        { OSSL_FUNC_KEYMGMT_LOAD, (void (*)(void))ecx_load }, \
        { 0, NULL } \
    };

MAKE_KEYMGMT_FUNCTIONS(x25519)
MAKE_KEYMGMT_FUNCTIONS(x448)
MAKE_KEYMGMT_FUNCTIONS(ed25519)
MAKE_KEYMGMT_FUNCTIONS(ed448)

#ifdef S390X_EC_ASM
# include "s390x_arch.h"

static void *s390x_ecx_keygen25519(struct ecx_gen_ctx *gctx)
{
    static const unsigned char generator[] = {
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    ECX_KEY *key = ecx_key_new(gctx->libctx, ECX_KEY_TYPE_X25519, 1);
    unsigned char *privkey = NULL, *pubkey;

    if (key == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    /* If we're doing parameter generation then we just return a blank key */
    if ((gctx->selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return key;

    pubkey = key->pubkey;

    privkey = ecx_key_allocate_privkey(key);
    if (privkey == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    if (RAND_priv_bytes_with_libctx(gctx->libctx, privkey, X25519_KEYLEN) <= 0)
        goto err;

    privkey[0] &= 248;
    privkey[31] &= 127;
    privkey[31] |= 64;

    if (s390x_x25519_mul(pubkey, generator, privkey) != 1)
        goto err;
    key->haspubkey = 1;
    return key;
 err:
    ecx_key_free(key);
    return NULL;
}

static void *s390x_ecx_keygen448(struct ecx_gen_ctx *gctx)
{
    static const unsigned char generator[] = {
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    ECX_KEY *key = ecx_key_new(gctx->libctx, ECX_KEY_TYPE_X448, 1);
    unsigned char *privkey = NULL, *pubkey;

    if (key == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    /* If we're doing parameter generation then we just return a blank key */
    if ((gctx->selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return key;

    pubkey = key->pubkey;

    privkey = ecx_key_allocate_privkey(key);
    if (privkey == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    if (RAND_priv_bytes_with_libctx(gctx->libctx, privkey, X448_KEYLEN) <= 0)
        goto err;

    privkey[0] &= 252;
    privkey[55] |= 128;

    if (s390x_x448_mul(pubkey, generator, privkey) != 1)
        goto err;
    key->haspubkey = 1;
    return key;
 err:
    ecx_key_free(key);
    return NULL;
}

static void *s390x_ecd_keygen25519(struct ecx_gen_ctx *gctx)
{
    static const unsigned char generator_x[] = {
        0x1a, 0xd5, 0x25, 0x8f, 0x60, 0x2d, 0x56, 0xc9, 0xb2, 0xa7, 0x25, 0x95,
        0x60, 0xc7, 0x2c, 0x69, 0x5c, 0xdc, 0xd6, 0xfd, 0x31, 0xe2, 0xa4, 0xc0,
        0xfe, 0x53, 0x6e, 0xcd, 0xd3, 0x36, 0x69, 0x21
    };
    static const unsigned char generator_y[] = {
        0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
        0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
        0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
    };
    unsigned char x_dst[32], buff[SHA512_DIGEST_LENGTH];
    ECX_KEY *key = ecx_key_new(gctx->libctx, ECX_KEY_TYPE_ED25519, 1);
    unsigned char *privkey = NULL, *pubkey;
    unsigned int sz;
    EVP_MD *sha = NULL;
    int j;

    if (key == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    /* If we're doing parameter generation then we just return a blank key */
    if ((gctx->selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return key;

    pubkey = key->pubkey;

    privkey = ecx_key_allocate_privkey(key);
    if (privkey == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    if (RAND_priv_bytes_with_libctx(gctx->libctx, privkey, ED25519_KEYLEN) <= 0)
        goto err;

    sha = EVP_MD_fetch(gctx->libctx, "SHA512", NULL);
    if (sha == NULL)
        goto err;
    j = EVP_Digest(privkey, 32, buff, &sz, sha, NULL);
    EVP_MD_free(sha);
    if (!j)
        goto err;

    buff[0] &= 248;
    buff[31] &= 63;
    buff[31] |= 64;

    if (s390x_ed25519_mul(x_dst, pubkey,
                          generator_x, generator_y, buff) != 1)
        goto err;

    pubkey[31] |= ((x_dst[0] & 0x01) << 7);
    key->haspubkey = 1;
    return key;
 err:
    ecx_key_free(key);
    return NULL;
}

static void *s390x_ecd_keygen448(struct ecx_gen_ctx *gctx)
{
    static const unsigned char generator_x[] = {
        0x5e, 0xc0, 0x0c, 0xc7, 0x2b, 0xa8, 0x26, 0x26, 0x8e, 0x93, 0x00, 0x8b,
        0xe1, 0x80, 0x3b, 0x43, 0x11, 0x65, 0xb6, 0x2a, 0xf7, 0x1a, 0xae, 0x12,
        0x64, 0xa4, 0xd3, 0xa3, 0x24, 0xe3, 0x6d, 0xea, 0x67, 0x17, 0x0f, 0x47,
        0x70, 0x65, 0x14, 0x9e, 0xda, 0x36, 0xbf, 0x22, 0xa6, 0x15, 0x1d, 0x22,
        0xed, 0x0d, 0xed, 0x6b, 0xc6, 0x70, 0x19, 0x4f, 0x00
    };
    static const unsigned char generator_y[] = {
        0x14, 0xfa, 0x30, 0xf2, 0x5b, 0x79, 0x08, 0x98, 0xad, 0xc8, 0xd7, 0x4e,
        0x2c, 0x13, 0xbd, 0xfd, 0xc4, 0x39, 0x7c, 0xe6, 0x1c, 0xff, 0xd3, 0x3a,
        0xd7, 0xc2, 0xa0, 0x05, 0x1e, 0x9c, 0x78, 0x87, 0x40, 0x98, 0xa3, 0x6c,
        0x73, 0x73, 0xea, 0x4b, 0x62, 0xc7, 0xc9, 0x56, 0x37, 0x20, 0x76, 0x88,
        0x24, 0xbc, 0xb6, 0x6e, 0x71, 0x46, 0x3f, 0x69, 0x00
    };
    unsigned char x_dst[57], buff[114];
    ECX_KEY *key = ecx_key_new(gctx->libctx, ECX_KEY_TYPE_ED448, 1);
    unsigned char *privkey = NULL, *pubkey;
    EVP_MD_CTX *hashctx = NULL;
    EVP_MD *shake = NULL;

    if (key == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    /* If we're doing parameter generation then we just return a blank key */
    if ((gctx->selection & OSSL_KEYMGMT_SELECT_KEYPAIR) == 0)
        return key;

    pubkey = key->pubkey;

    privkey = ecx_key_allocate_privkey(key);
    if (privkey == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    shake = EVP_MD_fetch(gctx->libctx, "SHAKE256", NULL);
    if (shake == NULL)
        goto err;
    if (RAND_priv_bytes_with_libctx(gctx->libctx, privkey, ED448_KEYLEN) <= 0)
        goto err;

    hashctx = EVP_MD_CTX_new();
    if (hashctx == NULL)
        goto err;
    if (EVP_DigestInit_ex(hashctx, shake, NULL) != 1)
        goto err;
    if (EVP_DigestUpdate(hashctx, privkey, 57) != 1)
        goto err;
    if (EVP_DigestFinalXOF(hashctx, buff, sizeof(buff)) != 1)
        goto err;

    buff[0] &= -4;
    buff[55] |= 0x80;
    buff[56] = 0;

    if (s390x_ed448_mul(x_dst, pubkey,
                        generator_x, generator_y, buff) != 1)
        goto err;

    pubkey[56] |= ((x_dst[0] & 0x01) << 7);
    EVP_MD_CTX_free(hashctx);
    EVP_MD_free(shake);
    key->haspubkey = 1;
    return key;
 err:
    ecx_key_free(key);
    EVP_MD_CTX_free(hashctx);
    EVP_MD_free(shake);
    return NULL;
}
#endif
