/*
 * Copyright 1995-2016 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#include <opentls/crypto.h>
#include <opentls/opentlsconf.h>

#include <opentls/opentlsv.h>
#include <opentls/evp.h>
#include <opentls/sha.h>

/* The implementation is in ../md32_common.h */

#include "sha_local.h"
#include "crypto/sha.h"

int sha1_ctrl(SHA_CTX *sha1, int cmd, int mslen, void *ms)
{
    unsigned char padtmp[40];
    unsigned char sha1tmp[SHA_DIGEST_LENGTH];

    if (cmd != EVP_CTRL_tls3_MASTER_SECRET)
        return -2;

    if (sha1 == NULL)
        return 0;

    /* tlsv3 client auth handling: see RFC-6101 5.6.8 */
    if (mslen != 48)
        return 0;

    /* At this point hash contains all handshake messages, update
     * with master secret and pad_1.
     */

    if (SHA1_Update(sha1, ms, mslen) <= 0)
        return 0;

    /* Set padtmp to pad_1 value */
    memset(padtmp, 0x36, sizeof(padtmp));

    if (!SHA1_Update(sha1, padtmp, sizeof(padtmp)))
        return 0;

    if (!SHA1_Final(sha1tmp, sha1))
        return 0;

    /* Reinitialise context */

    if (!SHA1_Init(sha1))
        return 0;

    if (SHA1_Update(sha1, ms, mslen) <= 0)
        return 0;

    /* Set padtmp to pad_2 value */
    memset(padtmp, 0x5c, sizeof(padtmp));

    if (!SHA1_Update(sha1, padtmp, sizeof(padtmp)))
        return 0;

    if (!SHA1_Update(sha1, sha1tmp, sizeof(sha1tmp)))
        return 0;

    /* Now when ctx is finalised it will return the tls v3 hash value */
    OPENtls_cleanse(sha1tmp, sizeof(sha1tmp));

    return 1;
}
