/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/err.h>
#include <openssl/comperr.h>

#ifndef OPENSSL_NO_ERR

static const ERR_STRING_DATA COMP_str_reasons[] = {
    {ERR_PACK(ERR_LIB_COMP, 0, COMP_R_ZLIB_DEFLATE_ERROR),
    "zlib deflate error"},
    {ERR_PACK(ERR_LIB_COMP, 0, COMP_R_ZLIB_INFLATE_ERROR),
    "zlib inflate error"},
    {ERR_PACK(ERR_LIB_COMP, 0, COMP_R_ZLIB_NOT_SUPPORTED),
    "zlib not supported"},
    {0, NULL}
};

#endif

int ERR_load_COMP_strings(void)
{
#ifndef OPENSSL_NO_ERR
    if (ERR_reason_error_string(COMP_str_reasons[0].error) == NULL)
        ERR_load_strings_const(COMP_str_reasons);
#endif
    return 1;
}
