/*
 * Copyright 2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <string.h>
#include <stdio.h>
#include <openssl/params.h>
#include <prov/providercommon.h>
#include <prov/names.h>
#include "pkcs11_ctx.h"
#include <dlfcn.h>
#include <openssl/err.h>
#include <openssl/proverr.h>

extern const OSSL_DISPATCH rsa_keymgmt_dp_tbl[];

/* provider entry point (fixed name, exported) */
OSSL_provider_init_fn OSSL_provider_init;

static OSSL_ALGORITHM rsa_keymgmt_alg_tbl[] = {
    { PROV_NAMES_RSA, "provider=pkcs11,fips=no", rsa_keymgmt_dp_tbl},
    { NULL, NULL, NULL }
};

/************************************************************************
 * Parameters we provide to the core.
 * The parameters in this list can be used with this provider.
 * Implementation for retrieving those parameters are implemented in 
 * my_get_params.
 */
static const OSSL_PARAM pkcs11_get_param_types[] = {
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_NAME,       OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_VERSION,    OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_BUILDINFO,  OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_STATUS,     OSSL_PARAM_INTEGER,  NULL, 0),
    OSSL_PARAM_END
};

static const OSSL_PARAM pkcs11_set_param_types[] = {
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_PKCS11_SLOT,     OSSL_PARAM_INTEGER,  NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_PKCS11_TOKEN,     OSSL_PARAM_INTEGER,  NULL, 0),
    OSSL_PARAM_DEFN(OSSL_PROV_PARAM_PKCS11_MODULE, OSSL_PARAM_UTF8_PTR, NULL, 0),
    OSSL_PARAM_END
};

/************************************************************************/

/************************************************************************
 * Static private functions definition
 */
void pkcs11_unload_module(PKCS11_CTX *ctx);
int pkcs11_load_module(PKCS11_CTX *ctx, const char* libname);
void pkcs11_generate_dispatch_tables(PKCS11_CTX *ctx);
int pkcs11_generate_mechanism_tables(PKCS11_CTX *ctx);
/************************************************************************/

/************************************************************************
 * Defining the dispatch table.
 * Forward declarations to ensure that interface functions are correctly
 * defined. Those interface functions are provided to the core using
 * my_dispatch_table.
 */
static OSSL_FUNC_provider_gettable_params_fn pkcs11_gettable_params;
static OSSL_FUNC_provider_get_params_fn pkcs11_get_params;
static OSSL_FUNC_provider_settable_params_fn pkcs11_settable_params;
static OSSL_FUNC_provider_set_params_fn pkcs11_set_params;
static OSSL_FUNC_provider_query_operation_fn pkcs11_query;
static OSSL_FUNC_provider_get_reason_strings_fn pkcs11_get_reason_strings;
static OSSL_FUNC_provider_teardown_fn pkcs11_teardown;

#define SET_PKCS11_PROV_ERR(ctx, reasonidx) \
    pkcs11_set_error(ctx, reasonidx, OPENSSL_FILE, OPENSSL_LINE, OPENSSL_FUNC, NULL)
static void pkcs11_set_error(PKCS11_CTX *ctx, int reason, const char *file, int line,
                        const char *func, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ctx->core_new_error(ctx->handle);
    ctx->core_set_error_debug(ctx->handle, file, line, func);
    ctx->core_vset_error(ctx->handle, ERR_PACK(ERR_LIB_PROV, 0, reason), fmt, ap);
    va_end(ap);
}

/* Define the reason string table */
#define ERR_PKCS11_NO_USERPIN_SET (CKR_TOKEN_RESOURCE_EXCEEDED + 1)
static const OSSL_ITEM reason_strings[] = {
#define REASON_STRING(ckr) {ckr, #ckr}
    REASON_STRING(CKR_CANCEL),
    REASON_STRING(CKR_HOST_MEMORY),
    REASON_STRING(CKR_SLOT_ID_INVALID),
    REASON_STRING(CKR_GENERAL_ERROR),
    REASON_STRING(CKR_FUNCTION_FAILED),
    REASON_STRING(CKR_ARGUMENTS_BAD),
    REASON_STRING(CKR_NO_EVENT),
    REASON_STRING(CKR_NEED_TO_CREATE_THREADS),
    REASON_STRING(CKR_CANT_LOCK),
    REASON_STRING(CKR_ATTRIBUTE_READ_ONLY),
    REASON_STRING(CKR_ATTRIBUTE_SENSITIVE),
    REASON_STRING(CKR_ATTRIBUTE_TYPE_INVALID),
    REASON_STRING(CKR_ATTRIBUTE_VALUE_INVALID),
    REASON_STRING(CKR_ACTION_PROHIBITED),
    REASON_STRING(CKR_DATA_INVALID),
    REASON_STRING(CKR_DATA_LEN_RANGE),
    REASON_STRING(CKR_DEVICE_ERROR),
    REASON_STRING(CKR_DEVICE_MEMORY),
    REASON_STRING(CKR_DEVICE_REMOVED),
    REASON_STRING(CKR_ENCRYPTED_DATA_INVALID),
    REASON_STRING(CKR_ENCRYPTED_DATA_LEN_RANGE),
    REASON_STRING(CKR_AEAD_DECRYPT_FAILED),
    REASON_STRING(CKR_FUNCTION_CANCELED),
    REASON_STRING(CKR_FUNCTION_NOT_PARALLEL),
    REASON_STRING(CKR_FUNCTION_NOT_SUPPORTED),
    REASON_STRING(CKR_KEY_HANDLE_INVALID),
    REASON_STRING(CKR_KEY_SIZE_RANGE),
    REASON_STRING(CKR_KEY_TYPE_INCONSISTENT),
    REASON_STRING(CKR_KEY_NOT_NEEDED),
    REASON_STRING(CKR_KEY_CHANGED),
    REASON_STRING(CKR_KEY_NEEDED),
    REASON_STRING(CKR_KEY_INDIGESTIBLE),
    REASON_STRING(CKR_KEY_FUNCTION_NOT_PERMITTED),
    REASON_STRING(CKR_KEY_NOT_WRAPPABLE),
    REASON_STRING(CKR_KEY_UNEXTRACTABLE),
    REASON_STRING(CKR_MECHANISM_INVALID),
    REASON_STRING(CKR_MECHANISM_PARAM_INVALID),
    REASON_STRING(CKR_OBJECT_HANDLE_INVALID),
    REASON_STRING(CKR_OPERATION_ACTIVE),
    REASON_STRING(CKR_OPERATION_NOT_INITIALIZED),
    REASON_STRING(CKR_PIN_INCORRECT),
    REASON_STRING(CKR_PIN_INVALID),
    REASON_STRING(CKR_PIN_LEN_RANGE),
    REASON_STRING(CKR_PIN_EXPIRED),
    REASON_STRING(CKR_PIN_LOCKED),
    REASON_STRING(CKR_SESSION_CLOSED),
    REASON_STRING(CKR_SESSION_COUNT),
    REASON_STRING(CKR_SESSION_HANDLE_INVALID),
    REASON_STRING(CKR_SESSION_PARALLEL_NOT_SUPPORTED),
    REASON_STRING(CKR_SESSION_READ_ONLY),
    REASON_STRING(CKR_SESSION_EXISTS),
    REASON_STRING(CKR_SESSION_READ_ONLY_EXISTS),
    REASON_STRING(CKR_SESSION_READ_WRITE_SO_EXISTS),
    REASON_STRING(CKR_SIGNATURE_INVALID),
    REASON_STRING(CKR_SIGNATURE_LEN_RANGE),
    REASON_STRING(CKR_TEMPLATE_INCOMPLETE),
    REASON_STRING(CKR_TEMPLATE_INCONSISTENT),
    REASON_STRING(CKR_TOKEN_NOT_PRESENT),
    REASON_STRING(CKR_TOKEN_NOT_RECOGNIZED),
    REASON_STRING(CKR_TOKEN_WRITE_PROTECTED),
    REASON_STRING(CKR_UNWRAPPING_KEY_HANDLE_INVALID),
    REASON_STRING(CKR_UNWRAPPING_KEY_SIZE_RANGE),
    REASON_STRING(CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT),
    REASON_STRING(CKR_USER_ALREADY_LOGGED_IN),
    REASON_STRING(CKR_USER_NOT_LOGGED_IN),
    REASON_STRING(CKR_USER_PIN_NOT_INITIALIZED),
    REASON_STRING(CKR_USER_TYPE_INVALID),
    REASON_STRING(CKR_USER_ANOTHER_ALREADY_LOGGED_IN),
    REASON_STRING(CKR_USER_TOO_MANY_TYPES),
    REASON_STRING(CKR_WRAPPED_KEY_INVALID),
    REASON_STRING(CKR_WRAPPED_KEY_LEN_RANGE),
    REASON_STRING(CKR_WRAPPING_KEY_HANDLE_INVALID),
    REASON_STRING(CKR_WRAPPING_KEY_SIZE_RANGE),
    REASON_STRING(CKR_WRAPPING_KEY_TYPE_INCONSISTENT),
    REASON_STRING(CKR_RANDOM_SEED_NOT_SUPPORTED),
    REASON_STRING(CKR_RANDOM_NO_RNG),
    REASON_STRING(CKR_DOMAIN_PARAMS_INVALID),
    REASON_STRING(CKR_CURVE_NOT_SUPPORTED),
    REASON_STRING(CKR_BUFFER_TOO_SMALL),
    REASON_STRING(CKR_SAVED_STATE_INVALID),
    REASON_STRING(CKR_INFORMATION_SENSITIVE),
    REASON_STRING(CKR_STATE_UNSAVEABLE),
    REASON_STRING(CKR_CRYPTOKI_NOT_INITIALIZED),
    REASON_STRING(CKR_CRYPTOKI_ALREADY_INITIALIZED),
    REASON_STRING(CKR_MUTEX_BAD),
    REASON_STRING(CKR_MUTEX_NOT_LOCKED),
    REASON_STRING(CKR_NEW_PIN_MODE),
    REASON_STRING(CKR_NEXT_OTP),
    REASON_STRING(CKR_EXCEEDED_MAX_ITERATIONS),
    REASON_STRING(CKR_FIPS_SELF_TEST_FAILED),
    REASON_STRING(CKR_LIBRARY_LOAD_FAILED),
    REASON_STRING(CKR_PIN_TOO_WEAK),
    REASON_STRING(CKR_PUBLIC_KEY_INVALID),
    REASON_STRING(CKR_FUNCTION_REJECTED),
    REASON_STRING(CKR_TOKEN_RESOURCE_EXCEEDED),
    REASON_STRING(ERR_PKCS11_NO_USERPIN_SET),
#undef REASON_STRING
    {0, NULL}
};


/* Functions we provide to the core */
static const OSSL_DISPATCH my_dispatch_table[] = {
    { OSSL_FUNC_PROVIDER_SETTABLE_PARAMS, (void (*)(void))pkcs11_settable_params },
    { OSSL_FUNC_PROVIDER_SET_PARAMS, (void (*)(void))pkcs11_set_params },
    { OSSL_FUNC_PROVIDER_GETTABLE_PARAMS, (void (*)(void))pkcs11_gettable_params },
    { OSSL_FUNC_PROVIDER_GET_PARAMS, (void (*)(void))pkcs11_get_params },
    { OSSL_FUNC_PROVIDER_QUERY_OPERATION, (void (*)(void))pkcs11_query },
    { OSSL_FUNC_PROVIDER_GET_REASON_STRINGS, (void (*)(void))pkcs11_get_reason_strings },
    { OSSL_FUNC_PROVIDER_TEARDOWN, (void (*)(void))pkcs11_teardown },
    { 0, NULL }
};

/************************************************************************/

#define SEARCH_PARAM "provider=my_provider"

/************************************************************************/

/************************************************************************/

/* Implementation for the OSSL_FUNC_PROVIDER_GETTABLE_PARAMS function */
static const OSSL_PARAM *pkcs11_gettable_params(void *provctx)
{
    printf("- my_provider: %s (%d)\n", __FUNCTION__, __LINE__);
    fflush(stdout);
    return pkcs11_get_param_types;
}

/* Implementation for the OSSL_FUNC_PROVIDER_GET_PARAMS function */
static int pkcs11_get_params(void *provctx, OSSL_PARAM params[])
{
    OSSL_PARAM *p;

    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_NAME);
    if (p != NULL && !OSSL_PARAM_set_utf8_ptr(p, "PKCS11 Provider"))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_VERSION);
    if (p != NULL && !OSSL_PARAM_set_utf8_ptr(p, OPENSSL_VERSION_STR))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_BUILDINFO);
    if (p != NULL && !OSSL_PARAM_set_utf8_ptr(p, OPENSSL_FULL_VERSION_STR))
        return 0;
    p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_STATUS);
    if (p != NULL && !OSSL_PARAM_set_int(p, ossl_prov_is_running()))
        return 0;
            
    return 1;
}

/* Implementation for the OSSL_FUNC_PROVIDER_GETTABLE_PARAMS function */
static const OSSL_PARAM *pkcs11_settable_params(void *provctx)
{
    return pkcs11_set_param_types;
}

/* Implementation for the OSSL_FUNC_PROVIDER_GET_PARAMS function */
static int pkcs11_set_params(void *provctx, const OSSL_PARAM params[])
{
    OSSL_PARAM *p;
    PKCS11_CTX *ctx = (PKCS11_CTX*)provctx;
    int ival = 0;
    const char* strval = NULL;

    p = OSSL_PARAM_locate((OSSL_PARAM*)params, OSSL_PROV_PARAM_PKCS11_SLOT);
    if (p != NULL && !OSSL_PARAM_get_int(p, &ival))
        return 0;
    else {
        ctx->slot = ival;
    }
    p = OSSL_PARAM_locate((OSSL_PARAM*)params, OSSL_PROV_PARAM_PKCS11_TOKEN);
    if (p != NULL && !OSSL_PARAM_get_int(p, &ival))
        return 0;
    else {
        ctx->token = ival;
    }
    p = OSSL_PARAM_locate((OSSL_PARAM*)params, OSSL_PROV_PARAM_PKCS11_USERPIN);
    if (p != NULL && !OSSL_PARAM_get_utf8_ptr(p, &strval))
        return 0;
    else
    {
        ctx->userpin = OPENSSL_zalloc(strlen(strval) + 1);
        strncpy((char*)ctx->userpin, strval, strlen(strval));
    }
    p = OSSL_PARAM_locate((OSSL_PARAM*)params, OSSL_PROV_PARAM_PKCS11_MODULE);
    if (p != NULL && !OSSL_PARAM_get_utf8_ptr(p, &strval))
        return 0;
    else {
        if (!pkcs11_load_module(ctx, strval))
            return 0;
    }

    return 1;
}

/* Implementation of the OSSL_FUNC_PROVIDER_QUERY_OPERATION function */
static const OSSL_ALGORITHM *pkcs11_query(void *provctx,
                                          int operation_id,
                                          int *no_cache)
{
    PKCS11_CTX* ctx = (PKCS11_CTX*)provctx;

    *no_cache = 1;
    switch (operation_id) {
    case OSSL_OP_DIGEST:
        return NULL;
    case OSSL_OP_CIPHER:
        return NULL;
    case OSSL_OP_KEYMGMT:
        return ctx->keymgmt.algolist;
    }
    return NULL;
}

static const OSSL_ITEM *pkcs11_get_reason_strings(void *provctx)
{
    if (provctx == NULL)
        return NULL;

    return reason_strings;
}

/* Implementation for the OSSL_FUNC_PROVIDER_TEARDOWN function */
static void pkcs11_teardown(void *provctx)
{
    PKCS11_CTX *ctx = (PKCS11_CTX*)provctx;
    pkcs11_unload_module(ctx);
    OSSL_LIB_CTX_free(PROV_LIBCTX_OF(provctx));
    OPENSSL_free(ctx);
}

/* Implementation of the OSSL_provider_init function
 * This method is basically the entry point of the provider library.
 */
int OSSL_provider_init(const OSSL_CORE_HANDLE *handle,
                            const OSSL_DISPATCH *in,
                            const OSSL_DISPATCH **out,
                            void **provctx)
{
    OSSL_LIB_CTX *libctx = NULL;
    PKCS11_CTX* ctx = NULL;
    int ret = 0;

    if (handle == NULL || in == NULL || out == NULL || provctx == NULL)
    {
        ERR_raise(ERR_LIB_PROV, ERR_R_PASSED_INVALID_ARGUMENT);
        goto end;
    }

    ctx = (PKCS11_CTX*)OPENSSL_zalloc(sizeof(PKCS11_CTX));
    if (ctx == NULL
        || (libctx = OSSL_LIB_CTX_new()) == NULL) {
        OSSL_LIB_CTX_free(libctx);
        pkcs11_teardown(ctx);
        *provctx = NULL;
        return 0;
    }

    /* Asign the core function to the context object */
    for (; in->function_id != 0; in++)
    {
        switch(in->function_id)
        {
        case OSSL_FUNC_CORE_NEW_ERROR:
            ctx->core_new_error = OSSL_FUNC_core_new_error(in);
            break;
        case OSSL_FUNC_CORE_SET_ERROR_DEBUG:
            ctx->core_set_error_debug = OSSL_FUNC_core_set_error_debug(in);
            break;
        case OSSL_FUNC_CORE_VSET_ERROR:
            ctx->core_vset_error = OSSL_FUNC_core_vset_error(in);
            break;
        }
    }

    *provctx = ctx;

    ossl_prov_ctx_set0_libctx(*provctx, libctx);
    ossl_prov_ctx_set0_handle(*provctx, handle);

    *out = my_dispatch_table;
    ret = 1;
end:
    return ret;
}

/************************************************************************
 * Helper Functions
 */

int pkcs11_do_GetFunctionList(PKCS11_CTX *ctx, char* libname)
{
    CK_RV(*pfunc) ();
    int ret = 0;

    ctx->lib_handle = dlopen(libname, RTLD_NOW);
    if (ctx->lib_handle == NULL)
        goto ret;

    *(void **)(&pfunc) = dlsym(ctx->lib_handle, "C_GetFunctionList");
    if (pfunc == NULL)
        goto ret;

    if (pfunc(&ctx->lib_functions) != CKR_OK)
        goto ret;

    ret = 1;
ret:
    if (!ret)
        pkcs11_unload_module(ctx);

    return ret;
}

int pkcs11_load_module(PKCS11_CTX *ctx, const char* libname)
{
    int ret = 0;
    CK_RV rv = CKR_CANCEL;
    CK_C_INITIALIZE_ARGS cinit_args = {0};
    CK_FLAGS flags;

    if (ctx == NULL || libname == NULL || strlen(libname) <= 0)
    {
        ERR_raise(ERR_LIB_PROV, ERR_R_PASSED_INVALID_ARGUMENT);
        goto end;
    }

    pkcs11_unload_module(ctx);

    if (!pkcs11_do_GetFunctionList(ctx, (char*)libname))
        goto end;

    // Initialize
    memset(&cinit_args, 0x0, sizeof(cinit_args));
    cinit_args.flags = CKF_OS_LOCKING_OK;

    rv = ctx->lib_functions->C_Initialize(&cinit_args);
    if (rv != CKR_OK) {
        SET_PKCS11_PROV_ERR(ctx, rv);
        goto end;
    }

    if (!pkcs11_generate_mechanism_tables(ctx))
        goto end;

    pkcs11_generate_dispatch_tables(ctx);

    /* Open a user R/W session: all future sessions will be user sessions. */
    flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;
    rv = ctx->lib_functions->C_OpenSession(ctx->slot, flags, NULL, NULL, &ctx->session);
    if (rv != CKR_OK)
        goto end;

    if (!ctx->userpin)
    {
        SET_PKCS11_PROV_ERR(ctx, ERR_PKCS11_NO_USERPIN_SET);
        goto end;
    }

    rv = ctx->lib_functions->C_Login(ctx->session, CKU_USER,
                          ctx->userpin,
                          strlen((const char*)ctx->userpin));
    if (rv != CKR_OK && rv != CKR_USER_ALREADY_LOGGED_IN) {
        SET_PKCS11_PROV_ERR(ctx, rv);
        goto end;
    }

    ctx->module_filename = OPENSSL_zalloc(strlen(libname) + 1);
    strncpy((char*)ctx->module_filename, libname, strlen(libname));
    ret = 1;
end:
    if (!ret)
        pkcs11_unload_module(ctx);

    return ret;
}

int pkcs11_generate_mechanism_tables(PKCS11_CTX *ctx)
{
    int ret = 0;
    CK_ULONG i = 0;
    CK_RV rv;
    CK_ULONG mechcount = 0;
    CK_MECHANISM_TYPE* mechlist = NULL;
    CK_MECHANISM_TYPE* pmechlist = NULL;
    CK_MECHANISM_INFO* mechinfo = NULL;
    CK_MECHANISM_INFO* pmechinfo = NULL;
    PKCS11_TYPE_DATA_ITEM* pkeymgm = NULL;
    int keymgmtlen = 0;

    /* Cache the slot's mechanism list. */
    rv = ctx->lib_functions->C_GetMechanismList(ctx->slot, NULL, &mechcount);
    if (rv != CKR_OK)
        goto end;
    mechlist = (CK_MECHANISM_TYPE*)OPENSSL_zalloc(mechcount * sizeof(CK_MECHANISM_TYPE));
    if (mechlist == NULL)
        goto end;
    rv = ctx->lib_functions->C_GetMechanismList(ctx->slot, mechlist, &mechcount);
    if (rv != CKR_OK)
        goto end;

    mechinfo = calloc(mechcount, sizeof(CK_MECHANISM_INFO));
    /* Cache the slot's mechanism info structure for each mechanism. */
    for (i = 0; i < mechcount; i++) {
        rv = ctx->lib_functions->C_GetMechanismInfo(ctx->slot,
                                         mechlist[i], &mechinfo[i]);
        if (rv != CKR_OK)
            goto end;
        if (mechinfo[i].flags & CKF_GENERATE_KEY_PAIR)
            keymgmtlen++;
    }
    if (keymgmtlen > 0) {
        ctx->keymgmt.items = OPENSSL_zalloc(keymgmtlen * sizeof(PKCS11_TYPE_DATA_ITEM));
        ctx->keymgmt.len = keymgmtlen;
        pkeymgm = ctx->keymgmt.items;
    }

    pmechinfo = mechinfo;
    pmechlist = mechlist;
    for (i = 0; i < mechcount; i++, pmechinfo++, pmechlist++) {
        if (pmechinfo->flags & CKF_GENERATE_KEY_PAIR)
        {
            pkeymgm->type = *pmechlist;
            memcpy(&pkeymgm->info, pmechinfo, sizeof(CK_MECHANISM_INFO));
            pkeymgm++;
        }
    }
    ret = 1;
end:
    if (mechlist)
        OPENSSL_free(mechlist);
    if (mechinfo)
        OPENSSL_free(mechinfo);

    return ret;
}

void pkcs11_unload_module(PKCS11_CTX *ctx)
{
    if (ctx->lib_handle)
    {
        ctx->lib_functions->C_Logout(ctx->session);
        ctx->lib_functions->C_Finalize(NULL);
        dlclose(ctx->lib_handle);
        ctx->lib_handle = NULL;
        if (ctx->module_filename)
            OPENSSL_free(ctx->module_filename);
        ctx->module_filename = NULL;
        if (ctx->userpin)
            OPENSSL_free(ctx->userpin);
        ctx->userpin = NULL;
        if (ctx->keymgmt.items)
            OPENSSL_free(ctx->keymgmt.items);
        ctx->keymgmt.items = NULL;
    }
}

void pkcs11_generate_dispatch_tables(PKCS11_CTX *ctx)
{
    ctx->keymgmt.algolist = rsa_keymgmt_alg_tbl;
}
/************************************************************************/

