/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#include <opentls/err.h>
#include <opentls/objectserr.h>

#ifndef OPENtls_NO_ERR

static const ERR_STRING_DATA OBJ_str_reasons[] = {
    {ERR_PACK(ERR_LIB_OBJ, 0, OBJ_R_OID_EXISTS), "oid exists"},
    {ERR_PACK(ERR_LIB_OBJ, 0, OBJ_R_UNKNOWN_NID), "unknown nid"},
    {0, NULL}
};

#endif

int ERR_load_OBJ_strings(void)
{
#ifndef OPENtls_NO_ERR
    if (ERR_reason_error_string(OBJ_str_reasons[0].error) == NULL)
        ERR_load_strings_const(OBJ_str_reasons);
#endif
    return 1;
}
