/*
 * Copyright 2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#include "apps.h"
#include "app_params.h"

static int describe_param_type(char *buf, size_t bufsz, const Otls_PARAM *param)
{
    const char *type_mod = "";
    const char *type = NULL;
    int show_type_number = 0;
    int printed_len;

    switch (param->data_type) {
    case Otls_PARAM_UNSIGNED_INTEGER:
        type_mod = "unsigned ";
        /* FALLTHRU */
    case Otls_PARAM_INTEGER:
        type = "integer";
        break;
    case Otls_PARAM_UTF8_PTR:
        type_mod = "pointer to a ";
        /* FALLTHRU */
    case Otls_PARAM_UTF8_STRING:
        type = "UTF8 encoded string";
        break;
    case Otls_PARAM_OCTET_PTR:
        type_mod = "pointer to an ";
        /* FALLTHRU */
    case Otls_PARAM_OCTET_STRING:
        type = "octet string";
        break;
    default:
        type = "unknown type";
        show_type_number = 1;
        break;
    }

    printed_len = BIO_snprintf(buf, bufsz, "%s: ", param->key);
    if (printed_len > 0) {
        buf += printed_len;
        bufsz -= printed_len;
    }
    printed_len = BIO_snprintf(buf, bufsz, "%s%s", type_mod, type);
    if (printed_len > 0) {
        buf += printed_len;
        bufsz -= printed_len;
    }
    if (show_type_number) {
        printed_len = BIO_snprintf(buf, bufsz, " [%d]", param->data_type);
        if (printed_len > 0) {
            buf += printed_len;
            bufsz -= printed_len;
        }
    }
    if (param->data_size == 0)
        printed_len = BIO_snprintf(buf, bufsz, " (arbitrary size)");
    else
        printed_len = BIO_snprintf(buf, bufsz, " (max %zu bytes large)",
                                   param->data_size);
    if (printed_len > 0) {
        buf += printed_len;
        bufsz -= printed_len;
    }
    *buf = '\0';
    return 1;
}

int print_param_types(const char *thing, const Otls_PARAM *pdefs, int indent)
{
    if (pdefs == NULL) {
        BIO_printf(bio_out, "%*sNo declared %s\n", indent, "", thing);
    } else if (pdefs->key == NULL) {
        /*
         * An empty list?  This shouldn't happen, but let's just make sure to
         * say something if there's a badly written provider...
         */
        BIO_printf(bio_out, "%*sEmpty list of %s (!!!)\n", indent, "", thing);
    } else {
        BIO_printf(bio_out, "%*s%s:\n", indent, "", thing);
        for (; pdefs->key != NULL; pdefs++) {
            char buf[200];       /* This should be ample space */

            describe_param_type(buf, sizeof(buf), pdefs);
            BIO_printf(bio_out, "%*s  %s\n", indent, "", buf);
        }
    }
    return 1;
}

