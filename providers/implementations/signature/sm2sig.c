/*
 * Copyright 2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * ECDSA low level APIs are deprecated for public use, but still ok for
 * internal use - SM2 implemetation uses ECDSA_size() function.
 */
#include "internal/deprecated.h"

#include <string.h> /* memcpy */
#include <openssl/crypto.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/dsa.h>
#include <openssl/params.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/proverr.h>
#include "internal/nelem.h"
#include "internal/sizes.h"
#include "internal/cryptlib.h"
#include "prov/implementations.h"
#include "prov/provider_ctx.h"
#include "crypto/ec.h"
#include "crypto/sm2.h"
#include "prov/der_sm2.h"

static OSSL_FUNC_signature_newctx_fn sm2sig_newctx;
static OSSL_FUNC_signature_sign_init_fn sm2sig_signature_init;
static OSSL_FUNC_signature_verify_init_fn sm2sig_signature_init;
static OSSL_FUNC_signature_sign_fn sm2sig_sign;
static OSSL_FUNC_signature_verify_fn sm2sig_verify;
static OSSL_FUNC_signature_digest_sign_init_fn sm2sig_digest_signverify_init;
static OSSL_FUNC_signature_digest_sign_update_fn sm2sig_digest_signverify_update;
static OSSL_FUNC_signature_digest_sign_final_fn sm2sig_digest_sign_final;
static OSSL_FUNC_signature_digest_verify_init_fn sm2sig_digest_signverify_init;
static OSSL_FUNC_signature_digest_verify_update_fn sm2sig_digest_signverify_update;
static OSSL_FUNC_signature_digest_verify_final_fn sm2sig_digest_verify_final;
static OSSL_FUNC_signature_freectx_fn sm2sig_freectx;
static OSSL_FUNC_signature_dupctx_fn sm2sig_dupctx;
static OSSL_FUNC_signature_get_ctx_params_fn sm2sig_get_ctx_params;
static OSSL_FUNC_signature_gettable_ctx_params_fn sm2sig_gettable_ctx_params;
static OSSL_FUNC_signature_set_ctx_params_fn sm2sig_set_ctx_params;
static OSSL_FUNC_signature_settable_ctx_params_fn sm2sig_settable_ctx_params;
static OSSL_FUNC_signature_get_ctx_md_params_fn sm2sig_get_ctx_md_params;
static OSSL_FUNC_signature_gettable_ctx_md_params_fn sm2sig_gettable_ctx_md_params;
static OSSL_FUNC_signature_set_ctx_md_params_fn sm2sig_set_ctx_md_params;
static OSSL_FUNC_signature_settable_ctx_md_params_fn sm2sig_settable_ctx_md_params;

/*
 * What's passed as an actual key is defined by the KEYMGMT interface.
 * We happen to know that our KEYMGMT simply passes EC structures, so
 * we use that here too.
 */
typedef struct {
    OSSL_LIB_CTX *libctx;
    char *propq;
    EC_KEY *ec;

    /*
     * Flag to determine if the hash function can be changed (1) or not (0)
     * Because it's dangerous to change during a DigestSign or DigestVerify
     * operation, this flag is cleared by their Init function, and set again
     * by their Final function.
     */
    unsigned int flag_allow_md : 1;
    /*
     * Flag to termine if the 'z' digest needs to be computed and fed to the
     * hash function.
     * This flag should be set on initialization and the compuation should
     * be performed only once, on first update.
     */
    unsigned int flag_compute_z_digest : 1;

    char mdname[OSSL_MAX_NAME_SIZE];

    /* The Algorithm Identifier of the combined signature algorithm */
    unsigned char aid_buf[OSSL_MAX_ALGORITHM_ID_SIZE];
    unsigned char *aid;
    size_t  aid_len;

    /* main digest */
    EVP_MD *md;
    EVP_MD_CTX *mdctx;
    size_t mdsize;

    /* SM2 ID used for calculating the Z value */
    unsigned char *id;
    size_t id_len;
} PROV_SM2_CTX;

static void *sm2sig_newctx(void *provctx, const char *propq)
{
    PROV_SM2_CTX *ctx = OPENSSL_zalloc(sizeof(PROV_SM2_CTX));

    if (ctx == NULL)
        return NULL;

    ctx->libctx = PROV_LIBCTX_OF(provctx);
    if (propq != NULL && (ctx->propq = OPENSSL_strdup(propq)) == NULL) {
        OPENSSL_free(ctx);
        ctx = NULL;
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
    }
    /* don't allow to change MD, and in fact there is no such need */
    ctx->flag_allow_md = 0;
    return ctx;
}

static int sm2sig_signature_init(void *vpsm2ctx, void *ec)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;

    if (psm2ctx == NULL || ec == NULL || !EC_KEY_up_ref(ec))
        return 0;
    EC_KEY_free(psm2ctx->ec);
    psm2ctx->ec = ec;
    return 1;
}

static int sm2sig_sign(void *vpsm2ctx, unsigned char *sig, size_t *siglen,
                       size_t sigsize, const unsigned char *tbs, size_t tbslen)
{
    PROV_SM2_CTX *ctx = (PROV_SM2_CTX *)vpsm2ctx;
    int ret;
    unsigned int sltmp;
    /* SM2 uses ECDSA_size as well */
    size_t ecsize = ECDSA_size(ctx->ec);

    if (sig == NULL) {
        *siglen = ecsize;
        return 1;
    }

    if (sigsize < (size_t)ecsize)
        return 0;

    if (ctx->mdsize != 0 && tbslen != ctx->mdsize)
        return 0;

    ret = sm2_internal_sign(tbs, tbslen, sig, &sltmp, ctx->ec);
    if (ret <= 0)
        return 0;

    *siglen = sltmp;
    return 1;
}

static int sm2sig_verify(void *vpsm2ctx, const unsigned char *sig, size_t siglen,
                         const unsigned char *tbs, size_t tbslen)
{
    PROV_SM2_CTX *ctx = (PROV_SM2_CTX *)vpsm2ctx;

    if (ctx->mdsize != 0 && tbslen != ctx->mdsize)
        return 0;

    return sm2_internal_verify(tbs, tbslen, sig, siglen, ctx->ec);
}

static void free_md(PROV_SM2_CTX *ctx)
{
    EVP_MD_CTX_free(ctx->mdctx);
    EVP_MD_free(ctx->md);
    ctx->mdctx = NULL;
    ctx->md = NULL;
    ctx->mdsize = 0;
}

static int sm2sig_digest_signverify_init(void *vpsm2ctx, const char *mdname,
                                         void *ec)
{
    PROV_SM2_CTX *ctx = (PROV_SM2_CTX *)vpsm2ctx;
    int md_nid = NID_sm3;
    WPACKET pkt;
    int ret = 0;

    free_md(ctx);

    if (!sm2sig_signature_init(vpsm2ctx, ec))
        return ret;

    ctx->md = EVP_MD_fetch(ctx->libctx, mdname, ctx->propq);
    ctx->mdsize = EVP_MD_size(ctx->md);
    ctx->mdctx = EVP_MD_CTX_new();
    if (ctx->mdctx == NULL)
        goto error;

    /*
     * TODO(3.0) Should we care about DER writing errors?
     * All it really means is that for some reason, there's no
     * AlgorithmIdentifier to be had, but the operation itself is
     * still valid, just as long as it's not used to construct
     * anything that needs an AlgorithmIdentifier.
     */
    ctx->aid_len = 0;
    if (WPACKET_init_der(&pkt, ctx->aid_buf, sizeof(ctx->aid_buf))
        && DER_w_algorithmIdentifier_SM2_with_MD(&pkt, -1, ctx->ec, md_nid)
        && WPACKET_finish(&pkt)) {
        WPACKET_get_total_written(&pkt, &ctx->aid_len);
        ctx->aid = WPACKET_get_curr(&pkt);
    }
    WPACKET_cleanup(&pkt);

    if (!EVP_DigestInit_ex(ctx->mdctx, ctx->md, NULL))
        goto error;

    ctx->flag_compute_z_digest = 1;

    ret = 1;

 error:
    if (!ret)
        free_md(ctx);
    return ret;
}

static int sm2sig_compute_z_digest(PROV_SM2_CTX *ctx)
{
    uint8_t *z = NULL;
    int ret = 1;

    if (ctx->flag_compute_z_digest) {
        /* Only do this once */
        ctx->flag_compute_z_digest = 0;

        if ((z = OPENSSL_zalloc(ctx->mdsize)) == NULL
            /* get hashed prefix 'z' of tbs message */
            || !sm2_compute_z_digest(z, ctx->md, ctx->id, ctx->id_len, ctx->ec)
            || !EVP_DigestUpdate(ctx->mdctx, z, ctx->mdsize))
            ret = 0;
        OPENSSL_free(z);
    }

    return ret;
}

int sm2sig_digest_signverify_update(void *vpsm2ctx, const unsigned char *data,
                                    size_t datalen)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;

    if (psm2ctx == NULL || psm2ctx->mdctx == NULL)
        return 0;

    return sm2sig_compute_z_digest(psm2ctx)
        && EVP_DigestUpdate(psm2ctx->mdctx, data, datalen);
}

int sm2sig_digest_sign_final(void *vpsm2ctx, unsigned char *sig, size_t *siglen,
                             size_t sigsize)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen = 0;

    if (psm2ctx == NULL || psm2ctx->mdctx == NULL)
        return 0;

    /*
     * If sig is NULL then we're just finding out the sig size. Other fields
     * are ignored. Defer to sm2sig_sign.
     */
    if (sig != NULL) {
        if (!(sm2sig_compute_z_digest(psm2ctx)
              && EVP_DigestFinal_ex(psm2ctx->mdctx, digest, &dlen)))
            return 0;
    }

    return sm2sig_sign(vpsm2ctx, sig, siglen, sigsize, digest, (size_t)dlen);
}


int sm2sig_digest_verify_final(void *vpsm2ctx, const unsigned char *sig,
                               size_t siglen)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen = 0;

    if (psm2ctx == NULL || psm2ctx->mdctx == NULL)
        return 0;

    /* SM2 always use SM3 so it's not possible to exceed the limit */
    if (!(sm2sig_compute_z_digest(psm2ctx)
          && EVP_DigestFinal_ex(psm2ctx->mdctx, digest, &dlen)))
        return 0;

    return sm2sig_verify(vpsm2ctx, sig, siglen, digest, (size_t)dlen);
}

static void sm2sig_freectx(void *vpsm2ctx)
{
    PROV_SM2_CTX *ctx = (PROV_SM2_CTX *)vpsm2ctx;

    free_md(ctx);
    EC_KEY_free(ctx->ec);
    OPENSSL_free(ctx->id);
    OPENSSL_free(ctx);
}

static void *sm2sig_dupctx(void *vpsm2ctx)
{
    PROV_SM2_CTX *srcctx = (PROV_SM2_CTX *)vpsm2ctx;
    PROV_SM2_CTX *dstctx;

    dstctx = OPENSSL_zalloc(sizeof(*srcctx));
    if (dstctx == NULL)
        return NULL;

    *dstctx = *srcctx;
    dstctx->ec = NULL;
    dstctx->md = NULL;
    dstctx->mdctx = NULL;

    if (srcctx->ec != NULL && !EC_KEY_up_ref(srcctx->ec))
        goto err;
    dstctx->ec = srcctx->ec;

    if (srcctx->md != NULL && !EVP_MD_up_ref(srcctx->md))
        goto err;
    dstctx->md = srcctx->md;

    if (srcctx->mdctx != NULL) {
        dstctx->mdctx = EVP_MD_CTX_new();
        if (dstctx->mdctx == NULL
                || !EVP_MD_CTX_copy_ex(dstctx->mdctx, srcctx->mdctx))
            goto err;
    }

    if (srcctx->id != NULL) {
        dstctx->id = OPENSSL_malloc(srcctx->id_len);
        if (dstctx->id == NULL)
            goto err;
        dstctx->id_len = srcctx->id_len;
        memcpy(dstctx->id, srcctx->id, srcctx->id_len);
    }

    return dstctx;
 err:
    sm2sig_freectx(dstctx);
    return NULL;
}

static int sm2sig_get_ctx_params(void *vpsm2ctx, OSSL_PARAM *params)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;
    OSSL_PARAM *p;

    if (psm2ctx == NULL || params == NULL)
        return 0;

    p = OSSL_PARAM_locate(params, OSSL_SIGNATURE_PARAM_ALGORITHM_ID);
    if (p != NULL
        && !OSSL_PARAM_set_octet_string(p, psm2ctx->aid, psm2ctx->aid_len))
        return 0;

    p = OSSL_PARAM_locate(params, OSSL_SIGNATURE_PARAM_DIGEST_SIZE);
    if (p != NULL && !OSSL_PARAM_set_size_t(p, psm2ctx->mdsize))
        return 0;

    p = OSSL_PARAM_locate(params, OSSL_SIGNATURE_PARAM_DIGEST);
    if (p != NULL && !OSSL_PARAM_set_utf8_string(p, psm2ctx->md == NULL
                                                    ? psm2ctx->mdname
                                                    : EVP_MD_name(psm2ctx->md)))
        return 0;

    return 1;
}

static const OSSL_PARAM known_gettable_ctx_params[] = {
    OSSL_PARAM_octet_string(OSSL_SIGNATURE_PARAM_ALGORITHM_ID, NULL, 0),
    OSSL_PARAM_size_t(OSSL_SIGNATURE_PARAM_DIGEST_SIZE, NULL),
    OSSL_PARAM_utf8_string(OSSL_SIGNATURE_PARAM_DIGEST, NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM *sm2sig_gettable_ctx_params(ossl_unused void *provctx)
{
    return known_gettable_ctx_params;
}

static int sm2sig_set_ctx_params(void *vpsm2ctx, const OSSL_PARAM params[])
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;
    const OSSL_PARAM *p;
    char *mdname;

    if (psm2ctx == NULL || params == NULL)
        return 0;

    p = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_DIST_ID);
    if (p != NULL) {
        void *tmp_id = NULL;
        size_t tmp_idlen;

        /*
         * If the 'z' digest has already been computed, the ID is set too late
         */
        if (!psm2ctx->flag_compute_z_digest)
            return 0;

        if (!OSSL_PARAM_get_octet_string(p, &tmp_id, 0, &tmp_idlen))
            return 0;
        OPENSSL_free(psm2ctx->id);
        psm2ctx->id = tmp_id;
        psm2ctx->id_len = tmp_idlen;
    }

    if (psm2ctx->md != NULL) {
        /*
         * You cannot set the digest name/size when doing a DigestSign or
         * DigestVerify.
         */
        return 1;
    }

    p = OSSL_PARAM_locate_const(params, OSSL_SIGNATURE_PARAM_DIGEST_SIZE);
    if (p != NULL && !OSSL_PARAM_get_size_t(p, &psm2ctx->mdsize))
        return 0;

    /*
     * We never actually use the mdname, but we do support getting it later.
     * This can be useful for applications that want to know the MD that they
     * previously set.
     */
    p = OSSL_PARAM_locate_const(params, OSSL_SIGNATURE_PARAM_DIGEST);
    mdname = psm2ctx->mdname;
    if (p != NULL
            && !OSSL_PARAM_get_utf8_string(p, &mdname, sizeof(psm2ctx->mdname)))
        return 0;

    return 1;
}

static const OSSL_PARAM known_settable_ctx_params[] = {
    OSSL_PARAM_size_t(OSSL_SIGNATURE_PARAM_DIGEST_SIZE, NULL),
    OSSL_PARAM_utf8_string(OSSL_SIGNATURE_PARAM_DIGEST, NULL, 0),
    OSSL_PARAM_octet_string(OSSL_PKEY_PARAM_DIST_ID, NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM *sm2sig_settable_ctx_params(ossl_unused void *provctx)
{
    /*
     * TODO(3.0): Should this function return a different set of settable ctx
     * params if the ctx is being used for a DigestSign/DigestVerify? In that
     * case it is not allowed to set the digest size/digest name because the
     * digest is explicitly set as part of the init.
     */
    return known_settable_ctx_params;
}

static int sm2sig_get_ctx_md_params(void *vpsm2ctx, OSSL_PARAM *params)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;

    if (psm2ctx->mdctx == NULL)
        return 0;

    return EVP_MD_CTX_get_params(psm2ctx->mdctx, params);
}

static const OSSL_PARAM *sm2sig_gettable_ctx_md_params(void *vpsm2ctx)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;

    if (psm2ctx->md == NULL)
        return 0;

    return EVP_MD_gettable_ctx_params(psm2ctx->md);
}

static int sm2sig_set_ctx_md_params(void *vpsm2ctx, const OSSL_PARAM params[])
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;

    if (psm2ctx->mdctx == NULL)
        return 0;

    return EVP_MD_CTX_set_params(psm2ctx->mdctx, params);
}

static const OSSL_PARAM *sm2sig_settable_ctx_md_params(void *vpsm2ctx)
{
    PROV_SM2_CTX *psm2ctx = (PROV_SM2_CTX *)vpsm2ctx;

    if (psm2ctx->md == NULL)
        return 0;

    return EVP_MD_settable_ctx_params(psm2ctx->md);
}

const OSSL_DISPATCH ossl_sm2_signature_functions[] = {
    { OSSL_FUNC_SIGNATURE_NEWCTX, (void (*)(void))sm2sig_newctx },
    { OSSL_FUNC_SIGNATURE_SIGN_INIT, (void (*)(void))sm2sig_signature_init },
    { OSSL_FUNC_SIGNATURE_SIGN, (void (*)(void))sm2sig_sign },
    { OSSL_FUNC_SIGNATURE_VERIFY_INIT, (void (*)(void))sm2sig_signature_init },
    { OSSL_FUNC_SIGNATURE_VERIFY, (void (*)(void))sm2sig_verify },
    { OSSL_FUNC_SIGNATURE_DIGEST_SIGN_INIT,
      (void (*)(void))sm2sig_digest_signverify_init },
    { OSSL_FUNC_SIGNATURE_DIGEST_SIGN_UPDATE,
      (void (*)(void))sm2sig_digest_signverify_update },
    { OSSL_FUNC_SIGNATURE_DIGEST_SIGN_FINAL,
      (void (*)(void))sm2sig_digest_sign_final },
    { OSSL_FUNC_SIGNATURE_DIGEST_VERIFY_INIT,
      (void (*)(void))sm2sig_digest_signverify_init },
    { OSSL_FUNC_SIGNATURE_DIGEST_VERIFY_UPDATE,
      (void (*)(void))sm2sig_digest_signverify_update },
    { OSSL_FUNC_SIGNATURE_DIGEST_VERIFY_FINAL,
      (void (*)(void))sm2sig_digest_verify_final },
    { OSSL_FUNC_SIGNATURE_FREECTX, (void (*)(void))sm2sig_freectx },
    { OSSL_FUNC_SIGNATURE_DUPCTX, (void (*)(void))sm2sig_dupctx },
    { OSSL_FUNC_SIGNATURE_GET_CTX_PARAMS, (void (*)(void))sm2sig_get_ctx_params },
    { OSSL_FUNC_SIGNATURE_GETTABLE_CTX_PARAMS,
      (void (*)(void))sm2sig_gettable_ctx_params },
    { OSSL_FUNC_SIGNATURE_SET_CTX_PARAMS, (void (*)(void))sm2sig_set_ctx_params },
    { OSSL_FUNC_SIGNATURE_SETTABLE_CTX_PARAMS,
      (void (*)(void))sm2sig_settable_ctx_params },
    { OSSL_FUNC_SIGNATURE_GET_CTX_MD_PARAMS,
      (void (*)(void))sm2sig_get_ctx_md_params },
    { OSSL_FUNC_SIGNATURE_GETTABLE_CTX_MD_PARAMS,
      (void (*)(void))sm2sig_gettable_ctx_md_params },
    { OSSL_FUNC_SIGNATURE_SET_CTX_MD_PARAMS,
      (void (*)(void))sm2sig_set_ctx_md_params },
    { OSSL_FUNC_SIGNATURE_SETTABLE_CTX_MD_PARAMS,
      (void (*)(void))sm2sig_settable_ctx_md_params },
    { 0, NULL }
};
