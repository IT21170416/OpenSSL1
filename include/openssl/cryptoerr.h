/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_CRYPTOERR_H
# define HEADER_CRYPTOERR_H

# ifdef  __cplusplus
extern "C"
# endif
int ERR_load_CRYPTO_strings(void);

/*
 * CRYPTO function codes.
 */
# define CRYPTO_F_CMAC_CTX_NEW                            120
# define CRYPTO_F_CRYPTO_DUP_EX_DATA                      110
# define CRYPTO_F_CRYPTO_FREE_EX_DATA                     111
# define CRYPTO_F_CRYPTO_GET_EX_NEW_INDEX                 100
# define CRYPTO_F_CRYPTO_MEMDUP                           115
# define CRYPTO_F_CRYPTO_NEW_EX_DATA                      112
# define CRYPTO_F_CRYPTO_OCB128_COPY_CTX                  121
# define CRYPTO_F_CRYPTO_OCB128_INIT                      122
# define CRYPTO_F_CRYPTO_SET_EX_DATA                      102
# define CRYPTO_F_FIPS_MODE_SET                           109
# define CRYPTO_F_GET_AND_LOCK                            113
# define CRYPTO_F_OPENSSL_ATEXIT                          114
# define CRYPTO_F_OPENSSL_BUF2HEXSTR                      117
# define CRYPTO_F_OPENSSL_FOPEN                           119
# define CRYPTO_F_OPENSSL_HEXSTR2BUF                      118
# define CRYPTO_F_OPENSSL_INIT_CRYPTO                     116
# define CRYPTO_F_OPENSSL_LH_NEW                          126
# define CRYPTO_F_OPENSSL_SK_DEEP_COPY                    127
# define CRYPTO_F_OPENSSL_SK_DUP                          128
# define CRYPTO_F_OSSL_PARAM_GET_BN                       130
# define CRYPTO_F_OSSL_PARAM_GET_DOUBLE                   131
# define CRYPTO_F_OSSL_PARAM_GET_INT_COMMON               132
# define CRYPTO_F_OSSL_PARAM_LOCATE                       133
# define CRYPTO_F_OSSL_PARAM_SET_BN                       134
# define CRYPTO_F_OSSL_PARAM_SET_DOUBLE                   135
# define CRYPTO_F_OSSL_PARAM_SET_INT_COMMON               136
# define CRYPTO_F_PKEY_HMAC_INIT                          123
# define CRYPTO_F_PKEY_POLY1305_INIT                      124
# define CRYPTO_F_PKEY_SIPHASH_INIT                       125
# define CRYPTO_F_SK_RESERVE                              129

/*
 * CRYPTO reason codes.
 */
# define CRYPTO_R_FIPS_MODE_NOT_SUPPORTED                 101
# define CRYPTO_R_ILLEGAL_HEX_DIGIT                       102
# define CRYPTO_R_ODD_NUMBER_OF_DIGITS                    103
# define CRYPTO_R_PARAM_NOT_FOUND                         104
# define CRYPTO_R_TYPE_MISMATCH                           105

#endif
