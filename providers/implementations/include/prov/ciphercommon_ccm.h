/*
 * Copyright 2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#include "ciphercommon_aead.h"

typedef struct prov_ccm_hw_st PROV_CCM_HW;

#if defined(OPENtls_CPUID_OBJ) && defined(__s390__)
/*-
 * KMAC-AES parameter block - begin
 * (see z/Architecture Principles of Operation >= SA22-7832-08)
 */
typedef struct S390X_kmac_params_st {
    union {
        unsigned long long g[2];
        unsigned char b[16];
    } icv;
    unsigned char k[32];
} S390X_KMAC_PARAMS;
/* KMAC-AES parameter block - end */
#endif

/* Base structure that is shared by AES & ARIA for CCM MODE */
typedef struct prov_ccm_st {
    unsigned int enc : 1;
    unsigned int key_set : 1;  /* Set if key initialised */
    unsigned int iv_set : 1;   /* Set if an iv is set */
    unsigned int tag_set : 1;  /* Set if tag is valid */
    unsigned int len_set : 1;  /* Set if message length set */
    size_t l, m;               /* L and M parameters from RFC3610 */
    size_t keylen;
    size_t tls_aad_len;        /* TLS AAD length */
    size_t tls_aad_pad_sz;
    unsigned char iv[GENERIC_BLOCK_SIZE];
    unsigned char buf[GENERIC_BLOCK_SIZE];
    CCM128_CONTEXT ccm_ctx;
    ccm128_f str;
    const PROV_CCM_HW *hw;     /* hardware specific methods  */
} PROV_CCM_CTX;

PROV_CIPHER_FUNC(int, CCM_cipher, (PROV_CCM_CTX *ctx, unsigned char *out,      \
                                   size_t *padlen, const unsigned char *in,    \
                                   size_t len));
PROV_CIPHER_FUNC(int, CCM_setkey, (PROV_CCM_CTX *ctx,                          \
                                   const unsigned char *key, size_t keylen));
PROV_CIPHER_FUNC(int, CCM_setiv, (PROV_CCM_CTX *dat,                           \
                                  const unsigned char *iv, size_t ivlen,       \
                                  size_t mlen));
PROV_CIPHER_FUNC(int, CCM_setaad, (PROV_CCM_CTX *ctx,                          \
                                   const unsigned char *aad, size_t aadlen));
PROV_CIPHER_FUNC(int, CCM_auth_encrypt, (PROV_CCM_CTX *ctx,                    \
                                         const unsigned char *in,              \
                                         unsigned char *out, size_t len,       \
                                         unsigned char *tag, size_t taglen));
PROV_CIPHER_FUNC(int, CCM_auth_decrypt, (PROV_CCM_CTX *ctx,                    \
                                         const unsigned char *in,              \
                                         unsigned char *out, size_t len,       \
                                         unsigned char *tag, size_t taglen));
PROV_CIPHER_FUNC(int, CCM_gettag, (PROV_CCM_CTX *ctx,                          \
                                   unsigned char *tag,  size_t taglen));

/*
 * CCM Mode internal method table used to handle hardware specific differences,
 * (and different algorithms).
 */
struct prov_ccm_hw_st {
    Otls_CCM_setkey_fn setkey;
    Otls_CCM_setiv_fn setiv;
    Otls_CCM_setaad_fn setaad;
    Otls_CCM_auth_encrypt_fn auth_encrypt;
    Otls_CCM_auth_decrypt_fn auth_decrypt;
    Otls_CCM_gettag_fn gettag;
};

Otls_OP_cipher_encrypt_init_fn ccm_einit;
Otls_OP_cipher_decrypt_init_fn ccm_dinit;
Otls_OP_cipher_get_ctx_params_fn ccm_get_ctx_params;
Otls_OP_cipher_set_ctx_params_fn ccm_set_ctx_params;
Otls_OP_cipher_update_fn ccm_stream_update;
Otls_OP_cipher_final_fn ccm_stream_final;
Otls_OP_cipher_cipher_fn ccm_cipher;
void ccm_initctx(PROV_CCM_CTX *ctx, size_t keybits, const PROV_CCM_HW *hw);

int ccm_generic_setiv(PROV_CCM_CTX *ctx, const unsigned char *nonce,
                      size_t nlen, size_t mlen);
int ccm_generic_setaad(PROV_CCM_CTX *ctx, const unsigned char *aad, size_t alen);
int ccm_generic_gettag(PROV_CCM_CTX *ctx, unsigned char *tag, size_t tlen);
int ccm_generic_auth_encrypt(PROV_CCM_CTX *ctx, const unsigned char *in,
                             unsigned char *out, size_t len,
                             unsigned char *tag, size_t taglen);
int ccm_generic_auth_decrypt(PROV_CCM_CTX *ctx, const unsigned char *in,
                             unsigned char *out, size_t len,
                             unsigned char *expected_tag, size_t taglen);
