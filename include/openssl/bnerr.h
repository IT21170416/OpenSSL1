/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#ifndef OPENtls_BNERR_H
# define OPENtls_BNERR_H
# pragma once

# include <opentls/macros.h>
# ifndef OPENtls_NO_DEPRECATED_3_0
#  define HEADER_BNERR_H
# endif

# include <opentls/opentlsconf.h>
# include <opentls/symhacks.h>


# ifdef  __cplusplus
extern "C"
# endif
int ERR_load_BN_strings(void);

/*
 * BN function codes.
 */
# ifndef OPENtls_NO_DEPRECATED_3_0
#  define BN_F_BNRAND                                      0
#  define BN_F_BNRAND_RANGE                                0
#  define BN_F_BN_BLINDING_CONVERT_EX                      0
#  define BN_F_BN_BLINDING_CREATE_PARAM                    0
#  define BN_F_BN_BLINDING_INVERT_EX                       0
#  define BN_F_BN_BLINDING_NEW                             0
#  define BN_F_BN_BLINDING_UPDATE                          0
#  define BN_F_BN_BN2DEC                                   0
#  define BN_F_BN_BN2HEX                                   0
#  define BN_F_BN_COMPUTE_WNAF                             0
#  define BN_F_BN_CTX_GET                                  0
#  define BN_F_BN_CTX_NEW                                  0
#  define BN_F_BN_CTX_NEW_EX                               0
#  define BN_F_BN_CTX_START                                0
#  define BN_F_BN_DIV                                      0
#  define BN_F_BN_DIV_RECP                                 0
#  define BN_F_BN_EXP                                      0
#  define BN_F_BN_EXPAND_INTERNAL                          0
#  define BN_F_BN_GENCB_NEW                                0
#  define BN_F_BN_GENERATE_DSA_NONCE                       0
#  define BN_F_BN_GENERATE_PRIME_EX                        0
#  define BN_F_BN_GF2M_MOD                                 0
#  define BN_F_BN_GF2M_MOD_EXP                             0
#  define BN_F_BN_GF2M_MOD_MUL                             0
#  define BN_F_BN_GF2M_MOD_SOLVE_QUAD                      0
#  define BN_F_BN_GF2M_MOD_SOLVE_QUAD_ARR                  0
#  define BN_F_BN_GF2M_MOD_SQR                             0
#  define BN_F_BN_GF2M_MOD_SQRT                            0
#  define BN_F_BN_LSHIFT                                   0
#  define BN_F_BN_MOD_EXP2_MONT                            0
#  define BN_F_BN_MOD_EXP_MONT                             0
#  define BN_F_BN_MOD_EXP_MONT_CONSTTIME                   0
#  define BN_F_BN_MOD_EXP_MONT_WORD                        0
#  define BN_F_BN_MOD_EXP_RECP                             0
#  define BN_F_BN_MOD_EXP_SIMPLE                           0
#  define BN_F_BN_MOD_INVERSE                              0
#  define BN_F_BN_MOD_INVERSE_NO_BRANCH                    0
#  define BN_F_BN_MOD_LSHIFT_QUICK                         0
#  define BN_F_BN_MOD_SQRT                                 0
#  define BN_F_BN_MONT_CTX_NEW                             0
#  define BN_F_BN_MPI2BN                                   0
#  define BN_F_BN_NEW                                      0
#  define BN_F_BN_POOL_GET                                 0
#  define BN_F_BN_RAND                                     0
#  define BN_F_BN_RAND_RANGE                               0
#  define BN_F_BN_RECP_CTX_NEW                             0
#  define BN_F_BN_RSHIFT                                   0
#  define BN_F_BN_SET_WORDS                                0
#  define BN_F_BN_STACK_PUSH                               0
#  define BN_F_BN_USUB                                     0
# endif

/*
 * BN reason codes.
 */
# define BN_R_ARG2_LT_ARG3                                100
# define BN_R_BAD_RECIPROCAL                              101
# define BN_R_BIGNUM_TOO_LONG                             114
# define BN_R_BITS_TOO_SMALL                              118
# define BN_R_CALLED_WITH_EVEN_MODULUS                    102
# define BN_R_DIV_BY_ZERO                                 103
# define BN_R_ENCODING_ERROR                              104
# define BN_R_EXPAND_ON_STATIC_BIGNUM_DATA                105
# define BN_R_INPUT_NOT_REDUCED                           110
# define BN_R_INVALID_LENGTH                              106
# define BN_R_INVALID_RANGE                               115
# define BN_R_INVALID_SHIFT                               119
# define BN_R_NOT_A_SQUARE                                111
# define BN_R_NOT_INITIALIZED                             107
# define BN_R_NO_INVERSE                                  108
# define BN_R_NO_SOLUTION                                 116
# define BN_R_NO_SUITABLE_DIGEST                          120
# define BN_R_PRIVATE_KEY_TOO_LARGE                       117
# define BN_R_P_IS_NOT_PRIME                              112
# define BN_R_TOO_MANY_ITERATIONS                         113
# define BN_R_TOO_MANY_TEMPORARY_VARIABLES                109

#endif
