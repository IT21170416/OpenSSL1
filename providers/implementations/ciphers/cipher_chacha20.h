/*
 * Copyright 2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#include "include/crypto/chacha.h"
#include "prov/ciphercommon.h"

typedef struct {
    PROV_CIPHER_CTX base;     /* must be first */
    union {
        Otls_UNION_ALIGN;
        unsigned int d[CHACHA_KEY_SIZE / 4];
    } key;
    unsigned int  counter[CHACHA_CTR_SIZE / 4];
    unsigned char buf[CHACHA_BLK_SIZE];
    unsigned int  partial_len;
} PROV_CHACHA20_CTX;

typedef struct prov_cipher_hw_chacha20_st {
    PROV_CIPHER_HW base; /* must be first */
    int (*initiv)(PROV_CIPHER_CTX *ctx);

} PROV_CIPHER_HW_CHACHA20;

const PROV_CIPHER_HW *PROV_CIPHER_HW_chacha20(size_t keybits);

Otls_OP_cipher_encrypt_init_fn chacha20_einit;
Otls_OP_cipher_decrypt_init_fn chacha20_dinit;
void chacha20_initctx(PROV_CHACHA20_CTX *ctx);
