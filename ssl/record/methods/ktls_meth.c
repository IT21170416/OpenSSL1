/*
 * Copyright 2018-2022 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/rand.h>
#include "../../ssl_local.h"
#include "../record_local.h"
#include "recmethod_local.h"
#include "internal/ktls.h"

#if defined(__FreeBSD__)
# include "crypto/cryptodev.h"

/*
 * TODO(RECLAYER): This is essentially a copy of ktls_int_check_supported_cipher
 * but using an SSL object instead of an OSSL_RECORD_LAYER object. Once
 * the write side has been moved to the record layer this can be deleted
 */
int ktls_check_supported_cipher(const SSL_CONNECTION *s, const EVP_CIPHER *c,
                                const EVP_MD *md, size_t taglen)
{

    switch (s->version) {
    case TLS1_VERSION:
    case TLS1_1_VERSION:
    case TLS1_2_VERSION:
    case TLS1_3_VERSION:
        break;
    default:
        return 0;
    }

    if (EVP_CIPHER_is_a(c, "AES-128-GCM")
        || EVP_CIPHER_is_a(c, "AES-256-GCM")
# ifdef OPENSSL_KTLS_CHACHA20_POLY1305
        || EVP_CIPHER_is_a(c, "CHACHA20-POLY1305")
# endif
       )
       return 1;

    if (!EVP_CIPHER_is_a(c, "AES-128-CBC")
            && !EVP_CIPHER_is_a(c, "AES-256-CBC"))
        return 0;

    if (s->ext.use_etm)
        return 0;

    if (md == NULL
            || EVP_MD_is_a(md, "SHA1")
            || EVP_MD_is_a(md, "SHA2-256")
            || EVP_MD_is_a(md, "SHA2-384"))
        return 1;

    return 0;
}

/*-
 * Check if a given cipher is supported by the KTLS interface.
 * The kernel might still fail the setsockopt() if no suitable
 * provider is found, but this checks if the socket option
 * supports the cipher suite used at all.
 */
static int ktls_int_check_supported_cipher(OSSL_RECORD_LAYER *rl,
                                           const EVP_CIPHER *c,
                                           const EVP_MD *md,
                                           size_t taglen)
{
    switch (rl->version) {
    case TLS1_VERSION:
    case TLS1_1_VERSION:
    case TLS1_2_VERSION:
    case TLS1_3_VERSION:
        break;
    default:
        return 0;
    }

    if (EVP_CIPHER_is_a(c, "AES-128-GCM")
        || EVP_CIPHER_is_a(c, "AES-256-GCM")
# ifdef OPENSSL_KTLS_CHACHA20_POLY1305
        || EVP_CIPHER_is_a(c, "CHACHA20-POLY1305")
# endif
       )
       return 1;

    if (!EVP_CIPHER_is_a(c, "AES-128-CBC")
            && !EVP_CIPHER_is_a(c, "AES-256-CBC"))
        return 0;

    if (rl->use_etm)
        return 0;

    if (md == NULL
            || EVP_MD_is_a(md, "SHA1")
            || EVP_MD_is_a(md, "SHA2-256")
            || EVP_MD_is_a(md, "SHA2-384"))
        return 1;

    return 0;
}

/* Function to configure kernel TLS structure */
int ktls_configure_crypto(OSSL_LIB_CTX *libctx, int version, const EVP_CIPHER *c,
                          EVP_MD *md, void *rl_sequence,
                          ktls_crypto_info_t *crypto_info, int is_tx,
                          unsigned char *iv, size_t ivlen,
                          unsigned char *key, size_t keylen,
                          unsigned char *mac_key, size_t mac_secret_size)
{
    memset(crypto_info, 0, sizeof(*crypto_info));
    if (EVP_CIPHER_is_a(c, "AES-128-GCM")
            || EVP_CIPHER_is_a(c, "AES-256-GCM")) {
        crypto_info->cipher_algorithm = CRYPTO_AES_NIST_GCM_16;
        crypto_info->iv_len = ivlen;
    } else
# ifdef OPENSSL_KTLS_CHACHA20_POLY1305
    if (EVP_CIPHER_is_a(c, "CHACHA20-POLY1305")) {
        crypto_info->cipher_algorithm = CRYPTO_CHACHA20_POLY1305;
        crypto_info->iv_len = ivlen;
    } else
# endif
    if (EVP_CIPHER_is_a(c, "AES-128-CBC") || EVP_CIPHER_is_a(c, "AES-256-CBC")) {
        if (EVP_MD_is_a(md, "SHA1"))
            crypto_info->auth_algorithm = CRYPTO_SHA1_HMAC;
        else if (EVP_MD_is_a(md, "SHA2-256")) {
            crypto_info->auth_algorithm = CRYPTO_SHA2_256_HMAC;
        else if (EVP_MD_is_a(md, "SHA2-384"))
            crypto_info->auth_algorithm = CRYPTO_SHA2_384_HMAC;
        else
            return 0;
        crypto_info->cipher_algorithm = CRYPTO_AES_CBC;
        crypto_info->iv_len = ivlen;
        crypto_info->auth_key = mac_key;
        crypto_info->auth_key_len = mac_secret_size;
    } else {
        return 0;
    }
    crypto_info->cipher_key = key;
    crypto_info->cipher_key_len = keylen;
    crypto_info->iv = iv;
    crypto_info->tls_vmajor = (version >> 8) & 0x000000ff;
    crypto_info->tls_vminor = (version & 0x000000ff);
# ifdef TCP_RXTLS_ENABLE
    memcpy(crypto_info->rec_seq, rl_sequence, sizeof(crypto_info->rec_seq));
# else
    if (!is_tx)
        return 0;
# endif
    return 1;
};

#endif                         /* __FreeBSD__ */

#if defined(OPENSSL_SYS_LINUX)

/*
 * TODO(RECLAYER): This is essentially a copy of ktls_int_check_supported_cipher
 * but using an SSL object instead of an OSSL_RECORD_LAYER object. Once
 * the write side has been moved to the record layer this can be deleted
 */
int ktls_check_supported_cipher(const SSL_CONNECTION *s, const EVP_CIPHER *c,
                                const EVP_MD *md, size_t taglen)
{
    switch (s->version) {
    case TLS1_2_VERSION:
    case TLS1_3_VERSION:
        break;
    default:
        return 0;
    }

    /* check that cipher is AES_GCM_128, AES_GCM_256, AES_CCM_128
     * or Chacha20-Poly1305
     */
# ifdef OPENSSL_KTLS_AES_CCM_128
    if (EVP_CIPHER_is_a(c, "AES-128-CCM")) {
        if (s->version == TLS_1_3_VERSION /* broken on 5.x kernels */
            || taglen != EVP_CCM_TLS_TAG_LEN)
            return 0;
        return 1;
    } else
# endif
    if (0
# ifdef OPENSSL_KTLS_AES_GCM_128
        || EVP_CIPHER_is_a(c, "AES-128-GCM")
# endif
# ifdef OPENSSL_KTLS_AES_GCM_256
        || EVP_CIPHER_is_a(c, "AES-256-GCM")
# endif
# ifdef OPENSSL_KTLS_CHACHA20_POLY1305
        || EVP_CIPHER_is_a(c, "ChaCha20-Poly1305")
# endif
        ) {
        return 1;
    }
    return 0;
}

/* Function to check supported ciphers in Linux */
static int ktls_int_check_supported_cipher(OSSL_RECORD_LAYER *rl,
                                           const EVP_CIPHER *c,
                                           const EVP_MD *md,
                                           size_t taglen)
{
    switch (rl->version) {
    case TLS1_2_VERSION:
    case TLS1_3_VERSION:
        break;
    default:
        return 0;
    }

    /* check that cipher is AES_GCM_128, AES_GCM_256, AES_CCM_128
     * or Chacha20-Poly1305
     */
# ifdef OPENSSL_KTLS_AES_CCM_128
    if (EVP_CIPHER_is_a(c, "AES-128-CCM")) {
        if (rl->version == TLS_1_3_VERSION /* broken on 5.x kernels */
            || taglen != EVP_CCM_TLS_TAG_LEN)
            return 0;
        return 1;
    } else
# endif
    if (0
# ifdef OPENSSL_KTLS_AES_GCM_128
        || EVP_CIPHER_is_a(c, "AES-128-GCM")
# endif
# ifdef OPENSSL_KTLS_AES_GCM_256
        || EVP_CIPHER_is_a(c, "AES-256-GCM")
# endif
# ifdef OPENSSL_KTLS_CHACHA20_POLY1305
        || EVP_CIPHER_is_a(c, "ChaCha20-Poly1305")
# endif
        ) {
        return 1;
    }
    return 0;
}

/* Function to configure kernel TLS structure */
int ktls_configure_crypto(OSSL_LIB_CTX *libctx, int version, const EVP_CIPHER *c,
                          const EVP_MD *md, void *rl_sequence,
                          ktls_crypto_info_t *crypto_info, int is_tx,
                          unsigned char *iv, size_t ivlen,
                          unsigned char *key, size_t keylen,
                          unsigned char *mac_key, size_t mac_secret_size)
{
    unsigned char geniv[EVP_GCM_TLS_EXPLICIT_IV_LEN];
    unsigned char *eiv = NULL;

# ifdef OPENSSL_NO_KTLS_RX
    if (!is_tx)
        return 0;
# endif

    if (EVP_CIPHER_get_mode(c) == EVP_CIPH_GCM_MODE
            || EVP_CIPHER_get_mode(c) == EVP_CIPH_CCM_MODE) {
        if (!ossl_assert(EVP_GCM_TLS_FIXED_IV_LEN == EVP_CCM_TLS_FIXED_IV_LEN)
                || !ossl_assert(EVP_GCM_TLS_EXPLICIT_IV_LEN
                                == EVP_CCM_TLS_EXPLICIT_IV_LEN))
            return 0;
        if (version == TLS1_2_VERSION) {
            if (!ossl_assert(ivlen == EVP_GCM_TLS_FIXED_IV_LEN))
                return 0;
            if (is_tx) {
                if (RAND_bytes_ex(libctx, geniv,
                                EVP_GCM_TLS_EXPLICIT_IV_LEN, 0) <= 0)
                    return 0;
            } else {
                memset(geniv, 0, EVP_GCM_TLS_EXPLICIT_IV_LEN);
            }
            eiv = geniv;
        } else {
            if (!ossl_assert(ivlen == EVP_GCM_TLS_FIXED_IV_LEN
                                      + EVP_GCM_TLS_EXPLICIT_IV_LEN))
                return 0;
            eiv = iv + TLS_CIPHER_AES_GCM_128_SALT_SIZE;
        }
    }

    memset(crypto_info, 0, sizeof(*crypto_info));
    switch (EVP_CIPHER_get_nid(c))
    {
# ifdef OPENSSL_KTLS_AES_GCM_128
    case NID_aes_128_gcm:
        if (!ossl_assert(TLS_CIPHER_AES_GCM_128_SALT_SIZE == EVP_GCM_TLS_FIXED_IV_LEN)
                || !ossl_assert(TLS_CIPHER_AES_GCM_128_IV_SIZE == EVP_GCM_TLS_EXPLICIT_IV_LEN))
            return 0;
        crypto_info->gcm128.info.cipher_type = TLS_CIPHER_AES_GCM_128;
        crypto_info->gcm128.info.version = version;
        crypto_info->tls_crypto_info_len = sizeof(crypto_info->gcm128);
        memcpy(crypto_info->gcm128.iv, eiv, TLS_CIPHER_AES_GCM_128_IV_SIZE);
        memcpy(crypto_info->gcm128.salt, iv, TLS_CIPHER_AES_GCM_128_SALT_SIZE);
        memcpy(crypto_info->gcm128.key, key, keylen);
        memcpy(crypto_info->gcm128.rec_seq, rl_sequence,
               TLS_CIPHER_AES_GCM_128_REC_SEQ_SIZE);
        return 1;
# endif
# ifdef OPENSSL_KTLS_AES_GCM_256
    case NID_aes_256_gcm:
        if (!ossl_assert(TLS_CIPHER_AES_GCM_256_SALT_SIZE == EVP_GCM_TLS_FIXED_IV_LEN)
                || !ossl_assert(TLS_CIPHER_AES_GCM_256_IV_SIZE == EVP_GCM_TLS_EXPLICIT_IV_LEN))
            return 0;
        crypto_info->gcm256.info.cipher_type = TLS_CIPHER_AES_GCM_256;
        crypto_info->gcm256.info.version = version;
        crypto_info->tls_crypto_info_len = sizeof(crypto_info->gcm256);
        memcpy(crypto_info->gcm256.iv, eiv, TLS_CIPHER_AES_GCM_256_IV_SIZE);
        memcpy(crypto_info->gcm256.salt, iv, TLS_CIPHER_AES_GCM_256_SALT_SIZE);
        memcpy(crypto_info->gcm256.key, key, keylen);
        memcpy(crypto_info->gcm256.rec_seq, rl_sequence,
               TLS_CIPHER_AES_GCM_256_REC_SEQ_SIZE);

        return 1;
# endif
# ifdef OPENSSL_KTLS_AES_CCM_128
    case NID_aes_128_ccm:
        if (!ossl_assert(TLS_CIPHER_AES_CCM_128_SALT_SIZE == EVP_CCM_TLS_FIXED_IV_LEN)
                || !ossl_assert(TLS_CIPHER_AES_CCM_128_IV_SIZE == EVP_CCM_TLS_EXPLICIT_IV_LEN))
            return 0;
        crypto_info->ccm128.info.cipher_type = TLS_CIPHER_AES_CCM_128;
        crypto_info->ccm128.info.version = version;
        crypto_info->tls_crypto_info_len = sizeof(crypto_info->ccm128);
        memcpy(crypto_info->ccm128.iv, eiv, TLS_CIPHER_AES_CCM_128_IV_SIZE);
        memcpy(crypto_info->ccm128.salt, iv, TLS_CIPHER_AES_CCM_128_SALT_SIZE);
        memcpy(crypto_info->ccm128.key, key, keylen);
        memcpy(crypto_info->ccm128.rec_seq, rl_sequence,
               TLS_CIPHER_AES_CCM_128_REC_SEQ_SIZE);
        return 1;
# endif
# ifdef OPENSSL_KTLS_CHACHA20_POLY1305
    case NID_chacha20_poly1305:
        if (!ossl_assert(ivlen == TLS_CIPHER_CHACHA20_POLY1305_IV_SIZE))
            return 0;
        crypto_info->chacha20poly1305.info.cipher_type = TLS_CIPHER_CHACHA20_POLY1305;
        crypto_info->chacha20poly1305.info.version = version;
        crypto_info->tls_crypto_info_len = sizeof(crypto_info->chacha20poly1305);
        memcpy(crypto_info->chacha20poly1305.iv, iv, ivlen);
        memcpy(crypto_info->chacha20poly1305.key, key, keylen);
        memcpy(crypto_info->chacha20poly1305.rec_seq, rl_sequence,
               TLS_CIPHER_CHACHA20_POLY1305_REC_SEQ_SIZE);
        return 1;
# endif
    default:
        return 0;
    }

}

#endif /* OPENSSL_SYS_LINUX */

static int ktls_set_crypto_state(OSSL_RECORD_LAYER *rl, int level,
                                 unsigned char *key, size_t keylen,
                                 unsigned char *iv, size_t ivlen,
                                 unsigned char *mackey, size_t mackeylen,
                                 const EVP_CIPHER *ciph,
                                 size_t taglen,
                                 int mactype,
                                 const EVP_MD *md,
                                 const SSL_COMP *comp)
{
    ktls_crypto_info_t crypto_info;

    /*
     * Check if we are suitable for KTLS. If not suitable we return
     * OSSL_RECORD_RETURN_NON_FATAL_ERR so that other record layers can be tried
     * instead
     */

    if (comp != NULL)
        return OSSL_RECORD_RETURN_NON_FATAL_ERR;

    /* ktls supports only the maximum fragment size */
    if (rl->max_frag_len > 0 && rl->max_frag_len != SSL3_RT_MAX_PLAIN_LENGTH)
        return OSSL_RECORD_RETURN_NON_FATAL_ERR;
#if 0
    /*
     * TODO(RECLAYER): We will need to reintroduce the check of the send
     * fragment for KTLS once we do the record write side implementation
     */
    if (ssl_get_max_send_fragment(s) != SSL3_RT_MAX_PLAIN_LENGTH)
        return OSSL_RECORD_RETURN_NON_FATAL_ERR;
#endif

    /* check that cipher is supported */
    if (!ktls_int_check_supported_cipher(rl, ciph, md, taglen))
        return OSSL_RECORD_RETURN_NON_FATAL_ERR;

    /*
     * TODO(RECLAYER): For the write side we need to add a check for
     * use of s->record_padding_cb
     */

    /* All future data will get encrypted by ktls. Flush the BIO or skip ktls */
    if (rl->direction == OSSL_RECORD_DIRECTION_WRITE) {
       if (BIO_flush(rl->bio) <= 0)
           return OSSL_RECORD_RETURN_NON_FATAL_ERR;
    }

    if (!ktls_configure_crypto(rl->libctx, rl->version, ciph, md, rl->sequence,
                               &crypto_info,
                               rl->direction == OSSL_RECORD_DIRECTION_WRITE,
                               iv, ivlen, key, keylen, mackey, mackeylen))
       return OSSL_RECORD_RETURN_NON_FATAL_ERR;

    if (!BIO_set_ktls(rl->bio, &crypto_info, rl->direction))
        return OSSL_RECORD_RETURN_NON_FATAL_ERR;

    return OSSL_RECORD_RETURN_SUCCESS;
}

static int ktls_read_n(OSSL_RECORD_LAYER *rl, size_t n, size_t max, int extend,
                       int clearold, size_t *readbytes)
{
    int ret;

    ret = tls_default_read_n(rl, n, max, extend, clearold, readbytes);

    if (ret < OSSL_RECORD_RETURN_RETRY) {
        switch (errno) {
        case EBADMSG:
            RLAYERfatal(rl, SSL_AD_BAD_RECORD_MAC,
                        SSL_R_DECRYPTION_FAILED_OR_BAD_RECORD_MAC);
            break;
        case EMSGSIZE:
            RLAYERfatal(rl, SSL_AD_RECORD_OVERFLOW,
                        SSL_R_PACKET_LENGTH_TOO_LONG);
            break;
        case EINVAL:
            RLAYERfatal(rl, SSL_AD_PROTOCOL_VERSION,
                        SSL_R_WRONG_VERSION_NUMBER);
            break;
        default:
            break;
        }
    }

    return ret;
}

static int ktls_cipher(OSSL_RECORD_LAYER *rl, SSL3_RECORD *inrecs, size_t n_recs,
                       int sending, SSL_MAC_BUF *mac, size_t macsize)
{
    return 1;
}

static int ktls_validate_record_header(OSSL_RECORD_LAYER *rl, SSL3_RECORD *rec)
{
    if (rec->rec_version != TLS1_2_VERSION) {
        RLAYERfatal(rl, SSL_AD_DECODE_ERROR, SSL_R_WRONG_VERSION_NUMBER);
        return 0;
    }

    return 1;
}

static int ktls_post_process_record(OSSL_RECORD_LAYER *rl, SSL3_RECORD *rec)
{
    if (rl->version == TLS1_3_VERSION)
        return tls13_common_post_process_record(rl, rec);

    return 1;
}

static struct record_functions_st ossl_ktls_funcs = {
    ktls_set_crypto_state,
    ktls_read_n,
    tls_get_more_records,
    ktls_cipher,
    NULL,
    tls_default_set_protocol_version,
    ktls_validate_record_header,
    ktls_post_process_record
};

static int
ktls_new_record_layer(OSSL_LIB_CTX *libctx, const char *propq, int vers,
                      int role, int direction, int level, unsigned int epoch,
                      unsigned char *key, size_t keylen, unsigned char *iv,
                      size_t ivlen, unsigned char *mackey, size_t mackeylen,
                      const EVP_CIPHER *ciph, size_t taglen,
                      int mactype,
                      const EVP_MD *md, const SSL_COMP *comp, BIO *prev,
                      BIO *transport, BIO *next, BIO_ADDR *local, BIO_ADDR *peer,
                      const OSSL_PARAM *settings, const OSSL_PARAM *options,
                      const OSSL_DISPATCH *fns, void *cbarg,
                      OSSL_RECORD_LAYER **retrl)
{
    int ret;

    ret = tls_int_new_record_layer(libctx, propq, vers, role, direction, level,
                                   key, keylen, iv, ivlen, mackey, mackeylen,
                                   ciph, taglen, mactype, md, comp, prev,
                                   transport, next, local, peer, settings,
                                   options, fns, cbarg, retrl);

    if (ret != OSSL_RECORD_RETURN_SUCCESS)
        return ret;

    (*retrl)->funcs = &ossl_ktls_funcs;

    ret = (*retrl)->funcs->set_crypto_state(*retrl, level, key, keylen, iv,
                                            ivlen, mackey, mackeylen, ciph,
                                            taglen, mactype, md, comp);

    if (ret != OSSL_RECORD_RETURN_SUCCESS) {
        OPENSSL_free(*retrl);
        *retrl = NULL;
    } else {
        /*
         * With KTLS we always try and read as much as possible and fill the
         * buffer
         */
        (*retrl)->read_ahead = 1;
    }
    return ret;
}

const OSSL_RECORD_METHOD ossl_ktls_record_method = {
    ktls_new_record_layer,
    tls_free,
    tls_reset,
    tls_unprocessed_read_pending,
    tls_processed_read_pending,
    tls_app_data_pending,
    tls_write_pending,
    tls_get_max_record_len,
    tls_get_max_records,
    tls_write_records,
    tls_retry_write_records,
    tls_read_record,
    tls_release_record,
    tls_get_alert_code,
    tls_set1_bio,
    tls_set_protocol_version,
    tls_set_plain_alerts,
    tls_set_first_handshake,
    tls_set_max_pipelines,
    NULL,
    tls_get_state,
    tls_set_options
};
