/*
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* We need to use some engine deprecated APIs */
#define OPENSSL_SUPPRESS_DEPRECATED

#include <stdio.h>
#include "internal/cryptlib.h"
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/engine.h>
#include <openssl/x509.h>
#include <openssl/asn1.h>
#include "crypto/asn1.h"
#include "crypto/evp.h"

DEFINE_STACK_OF(ASN1_TYPE)
EVP_PKEY *d2i_PrivateKey_ex(int type, EVP_PKEY **a, const unsigned char **pp,
                            long length, OSSL_CTX *libctx, const char *propq)
{
    EVP_PKEY *ret;
    const unsigned char *p = *pp;

    if ((a == NULL) || (*a == NULL)) {
        if ((ret = EVP_PKEY_new()) == NULL) {
            ASN1err(0, ERR_R_EVP_LIB);
            return NULL;
        }
    } else {
        ret = *a;
#ifndef OPENSSL_NO_ENGINE
        ENGINE_finish(ret->engine);
        ret->engine = NULL;
#endif
    }

    if (!EVP_PKEY_set_type(ret, type)) {
        ASN1err(0, ASN1_R_UNKNOWN_PUBLIC_KEY_TYPE);
        goto err;
    }

    if (!ret->ameth->old_priv_decode ||
        !ret->ameth->old_priv_decode(ret, &p, length)) {
        if (ret->ameth->priv_decode != NULL
                || ret->ameth->priv_decode_with_libctx != NULL) {
            EVP_PKEY *tmp;
            PKCS8_PRIV_KEY_INFO *p8 = NULL;
            p8 = d2i_PKCS8_PRIV_KEY_INFO(NULL, &p, length);
            if (p8 == NULL)
                goto err;
            tmp = evp_pkcs82pkey_int(p8, libctx, propq);
            PKCS8_PRIV_KEY_INFO_free(p8);
            if (tmp == NULL)
                goto err;
            EVP_PKEY_free(ret);
            ret = tmp;
            if (EVP_PKEY_type(type) != EVP_PKEY_base_id(ret))
                goto err;
        } else {
            ASN1err(0, ERR_R_ASN1_LIB);
            goto err;
        }
    }
    *pp = p;
    if (a != NULL)
        (*a) = ret;
    return ret;
 err:
    if (a == NULL || *a != ret)
        EVP_PKEY_free(ret);
    return NULL;
}

EVP_PKEY *d2i_PrivateKey(int type, EVP_PKEY **a, const unsigned char **pp,
                         long length)
{
    return d2i_PrivateKey_ex(type, a, pp, length, NULL, NULL);
}

/*
 * This works like d2i_PrivateKey() except it automatically works out the
 * type
 */

EVP_PKEY *d2i_AutoPrivateKey_ex(EVP_PKEY **a, const unsigned char **pp,
                                long length, OSSL_CTX *libctx,
                                const char *propq)
{
    STACK_OF(ASN1_TYPE) *inkey;
    const unsigned char *p;
    int keytype;
    p = *pp;
    /*
     * Dirty trick: read in the ASN1 data into a STACK_OF(ASN1_TYPE): by
     * analyzing it we can determine the passed structure: this assumes the
     * input is surrounded by an ASN1 SEQUENCE.
     */
    inkey = d2i_ASN1_SEQUENCE_ANY(NULL, &p, length);
    p = *pp;
    /*
     * Since we only need to discern "traditional format" RSA and DSA keys we
     * can just count the elements.
     */
    if (sk_ASN1_TYPE_num(inkey) == 6) {
        keytype = EVP_PKEY_DSA;
    } else if (sk_ASN1_TYPE_num(inkey) == 4) {
        keytype = EVP_PKEY_EC;
    } else if (sk_ASN1_TYPE_num(inkey) == 3) { /* This seems to be PKCS8, not
                                              * traditional format */
        PKCS8_PRIV_KEY_INFO *p8 = d2i_PKCS8_PRIV_KEY_INFO(NULL, &p, length);
        EVP_PKEY *ret;

        sk_ASN1_TYPE_pop_free(inkey, ASN1_TYPE_free);
        if (p8 == NULL) {
            ASN1err(0, ASN1_R_UNSUPPORTED_PUBLIC_KEY_TYPE);
            return NULL;
        }
        ret = evp_pkcs82pkey_int(p8, libctx, propq);
        PKCS8_PRIV_KEY_INFO_free(p8);
        if (ret == NULL)
            return NULL;
        *pp = p;
        if (a) {
            *a = ret;
        }
        return ret;
    } else {
        keytype = EVP_PKEY_RSA;
    }
    sk_ASN1_TYPE_pop_free(inkey, ASN1_TYPE_free);
    return d2i_PrivateKey_ex(keytype, a, pp, length, libctx, propq);
}

EVP_PKEY *d2i_AutoPrivateKey(EVP_PKEY **a, const unsigned char **pp,
                             long length)
{
    return d2i_AutoPrivateKey_ex(a, pp, length, NULL, NULL);
}
