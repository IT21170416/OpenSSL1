/*
 * Copyright 2023 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OPENSSL_HSS_H
# define OPENSSL_HSS_H
# pragma once

# ifdef  __cplusplus
extern "C" {
# endif

/*
 * HSS minimum and maximum number of LMS trees
 * A tree of height 1 can be used to represent a LMS tree.
 */
#define OSSL_HSS_MIN_L 1
#define OSSL_HSS_MAX_L 8

#define OSSL_LMS_TYPE_SHA256_N32_H5   0x00000005
#define OSSL_LMS_TYPE_SHA256_N32_H10  0x00000006
#define OSSL_LMS_TYPE_SHA256_N32_H15  0x00000007
#define OSSL_LMS_TYPE_SHA256_N32_H20  0x00000008
#define OSSL_LMS_TYPE_SHA256_N32_H25  0x00000009
#define OSSL_LMS_TYPE_SHA256_N24_H5   0x0000000A
#define OSSL_LMS_TYPE_SHA256_N24_H10  0x0000000B
#define OSSL_LMS_TYPE_SHA256_N24_H15  0x0000000C
#define OSSL_LMS_TYPE_SHA256_N24_H20  0x0000000D
#define OSSL_LMS_TYPE_SHA256_N24_H25  0x0000000E
#define OSSL_LMS_TYPE_SHAKE_N32_H5    0x0000000F
#define OSSL_LMS_TYPE_SHAKE_N32_H10   0x00000010
#define OSSL_LMS_TYPE_SHAKE_N32_H15   0x00000011
#define OSSL_LMS_TYPE_SHAKE_N32_H20   0x00000012
#define OSSL_LMS_TYPE_SHAKE_N32_H25   0x00000013
#define OSSL_LMS_TYPE_SHAKE_N24_H5    0x00000014
#define OSSL_LMS_TYPE_SHAKE_N24_H10   0x00000015
#define OSSL_LMS_TYPE_SHAKE_N24_H15   0x00000016
#define OSSL_LMS_TYPE_SHAKE_N24_H20   0x00000017
#define OSSL_LMS_TYPE_SHAKE_N24_H25   0x00000018

#define OSSL_LM_OTS_TYPE_SHA256_N32_W1 0x00000001
#define OSSL_LM_OTS_TYPE_SHA256_N32_W2 0x00000002
#define OSSL_LM_OTS_TYPE_SHA256_N32_W4 0x00000003
#define OSSL_LM_OTS_TYPE_SHA256_N32_W8 0x00000004
#define OSSL_LM_OTS_TYPE_SHA256_N24_W1 0x00000005
#define OSSL_LM_OTS_TYPE_SHA256_N24_W2 0x00000006
#define OSSL_LM_OTS_TYPE_SHA256_N24_W4 0x00000007
#define OSSL_LM_OTS_TYPE_SHA256_N24_W8 0x00000008
#define OSSL_LM_OTS_TYPE_SHAKE_N32_W1  0x00000009
#define OSSL_LM_OTS_TYPE_SHAKE_N32_W2  0x0000000A
#define OSSL_LM_OTS_TYPE_SHAKE_N32_W4  0x0000000B
#define OSSL_LM_OTS_TYPE_SHAKE_N32_W8  0x0000000C
#define OSSL_LM_OTS_TYPE_SHAKE_N24_W1  0x0000000D
#define OSSL_LM_OTS_TYPE_SHAKE_N24_W2  0x0000000E
#define OSSL_LM_OTS_TYPE_SHAKE_N24_W4  0x0000000F
#define OSSL_LM_OTS_TYPE_SHAKE_N24_W8  0x00000010

# ifdef  __cplusplus
}
# endif
#endif
