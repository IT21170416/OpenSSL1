/*
 * Copyright 2022 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <errno.h>
#include <openssl/err.h>
#include "internal/time.h"

OSSL_TIME ossl_time_now(void)
{
#if defined(_WIN32)
    SYSTEMTIME st;
    union {
        unsigned __int64 ul;
        FILETIME ft;
    } now;

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &now.ft);
    /* re-bias to 1/1/1970 */
# ifdef  __MINGW32__
    now.ul -= 116444736000000000ULL;
# else
    now.ul -= 116444736000000000UI64;
# endif
    return ((uint64_t)now.ul) * (OSSL_TIME_SECOND / 10000000);
#else
    struct timeval t;

    if (gettimeofday(&t, NULL) < 0) {
        ERR_raise_data(ERR_LIB_SYS, get_last_sys_error(),
                       "calling gettimeofday()");
        return 0;
    }
    if (t.tv_sec <= 0)
        return t.tv_usec <= 0 ? 0 : t.tv_usec * (OSSL_TIME_SECOND / 1000000);
    return ((uint64_t)t.tv_sec * 1000000 + t.tv_usec)
           * (OSSL_TIME_SECOND / 1000000);
#endif
}

