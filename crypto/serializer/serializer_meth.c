/*
 * Copyright 2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#include <opentls/core.h>
#include <opentls/core_numbers.h>
#include <opentls/serializer.h>
#include <opentls/ui.h>
#include "internal/core.h"
#include "internal/namemap.h"
#include "internal/property.h"
#include "internal/provider.h"
#include "crypto/serializer.h"
#include "serializer_local.h"

/*
 * Serializer can have multiple names, separated with colons in a name string
 */
#define NAME_SEPARATOR ':'

/* Simple method structure constructor and destructor */
static Otls_SERIALIZER *otls_serializer_new(void)
{
    Otls_SERIALIZER *ser = NULL;

    if ((ser = OPENtls_zalloc(sizeof(*ser))) == NULL
        || (ser->lock = CRYPTO_THREAD_lock_new()) == NULL) {
        Otls_SERIALIZER_free(ser);
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_MALLOC_FAILURE);
        return NULL;
    }

    ser->refcnt = 1;

    return ser;
}

int Otls_SERIALIZER_up_ref(Otls_SERIALIZER *ser)
{
    int ref = 0;

    CRYPTO_UP_REF(&ser->refcnt, &ref, ser->lock);
    return 1;
}

void Otls_SERIALIZER_free(Otls_SERIALIZER *ser)
{
    int ref = 0;

    if (ser == NULL)
        return;

    CRYPTO_DOWN_REF(&ser->refcnt, &ref, ser->lock);
    if (ref > 0)
        return;
    otls_provider_free(ser->prov);
    CRYPTO_THREAD_lock_free(ser->lock);
    OPENtls_free(ser);
}

/* Permanent serializer method store, constructor and destructor */
static void serializer_store_free(void *vstore)
{
    otls_method_store_free(vstore);
}

static void *serializer_store_new(OPENtls_CTX *ctx)
{
    return otls_method_store_new(ctx);
}


static const OPENtls_CTX_METHOD serializer_store_method = {
    serializer_store_new,
    serializer_store_free,
};

/* Data to be passed through otls_method_construct() */
struct serializer_data_st {
    OPENtls_CTX *libctx;
    Otls_METHOD_CONSTRUCT_METHOD *mcm;
    int id;                      /* For get_serializer_from_store() */
    const char *names;           /* For get_serializer_from_store() */
    const char *propquery;       /* For get_serializer_from_store() */
};

/*
 * Generic routines to fetch / create SERIALIZER methods with
 * otls_method_construct()
 */

/* Temporary serializer method store, constructor and destructor */
static void *alloc_tmp_serializer_store(OPENtls_CTX *ctx)
{
    return otls_method_store_new(ctx);
}

 static void dealloc_tmp_serializer_store(void *store)
{
    if (store != NULL)
        otls_method_store_free(store);
}

/* Get the permanent serializer store */
static Otls_METHOD_STORE *get_serializer_store(OPENtls_CTX *libctx)
{
    return opentls_ctx_get_data(libctx, OPENtls_CTX_SERIALIZER_STORE_INDEX,
                                &serializer_store_method);
}

/* Get serializer methods from a store, or put one in */
static void *get_serializer_from_store(OPENtls_CTX *libctx, void *store,
                                       void *data)
{
    struct serializer_data_st *methdata = data;
    void *method = NULL;
    int id;

    if ((id = methdata->id) == 0) {
        Otls_NAMEMAP *namemap = otls_namemap_stored(libctx);

        id = otls_namemap_name2num(namemap, methdata->names);
    }

    if (store == NULL
        && (store = get_serializer_store(libctx)) == NULL)
        return NULL;

    if (!otls_method_store_fetch(store, id, methdata->propquery, &method))
        return NULL;
    return method;
}

static int put_serializer_in_store(OPENtls_CTX *libctx, void *store,
                                   void *method, const Otls_PROVIDER *prov,
                                   int operation_id, const char *names,
                                   const char *propdef, void *unused)
{
    Otls_NAMEMAP *namemap;
    int id;

    if ((namemap = otls_namemap_stored(libctx)) == NULL
        || (id = otls_namemap_name2num(namemap, names)) == 0)
        return 0;

    if (store == NULL && (store = get_serializer_store(libctx)) == NULL)
        return 0;

    return otls_method_store_add(store, prov, id, propdef, method,
                                 (int (*)(void *))Otls_SERIALIZER_up_ref,
                                 (void (*)(void *))Otls_SERIALIZER_free);
}

/* Create and populate a serializer method */
static void *serializer_from_dispatch(int id, const Otls_ALGORITHM *algodef,
                                      Otls_PROVIDER *prov)
{
    Otls_SERIALIZER *ser = NULL;
    const Otls_DISPATCH *fns = algodef->implementation;

    if ((ser = otls_serializer_new()) == NULL)
        return NULL;
    ser->id = id;
    ser->propdef = algodef->property_definition;

    for (; fns->function_id != 0; fns++) {
        switch (fns->function_id) {
        case Otls_FUNC_SERIALIZER_NEWCTX:
            if (ser->newctx == NULL)
                ser->newctx =
                    Otls_get_OP_serializer_newctx(fns);
            break;
        case Otls_FUNC_SERIALIZER_FREECTX:
            if (ser->freectx == NULL)
                ser->freectx =
                    Otls_get_OP_serializer_freectx(fns);
            break;
        case Otls_FUNC_SERIALIZER_SET_CTX_PARAMS:
            if (ser->set_ctx_params == NULL)
                ser->set_ctx_params =
                    Otls_get_OP_serializer_set_ctx_params(fns);
            break;
        case Otls_FUNC_SERIALIZER_SETTABLE_CTX_PARAMS:
            if (ser->settable_ctx_params == NULL)
                ser->settable_ctx_params =
                    Otls_get_OP_serializer_settable_ctx_params(fns);
            break;
        case Otls_FUNC_SERIALIZER_SERIALIZE_DATA:
            if (ser->serialize_data == NULL)
                ser->serialize_data =
                    Otls_get_OP_serializer_serialize_data(fns);
            break;
        case Otls_FUNC_SERIALIZER_SERIALIZE_OBJECT:
            if (ser->serialize_object == NULL)
                ser->serialize_object =
                    Otls_get_OP_serializer_serialize_object(fns);
            break;
        }
    }
    /*
     * Try to check that the method is sensible.
     * If you have a constructor, you must have a destructor and vice versa.
     * You must have at least one of the serializing driver functions.
     */
    if (!((ser->newctx == NULL && ser->freectx == NULL)
          || (ser->newctx != NULL && ser->freectx != NULL))
        || (ser->serialize_data == NULL && ser->serialize_object == NULL)) {
        Otls_SERIALIZER_free(ser);
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_INVALID_PROVIDER_FUNCTIONS);
        return NULL;
    }

    if (prov != NULL && !otls_provider_up_ref(prov)) {
        Otls_SERIALIZER_free(ser);
        return NULL;
    }

    ser->prov = prov;
    return ser;
}


/*
 * The core fetching functionality passes the names of the implementation.
 * This function is responsible to getting an identity number for them,
 * then call serializer_from_dispatch() with that identity number.
 */
static void *construct_serializer(const Otls_ALGORITHM *algodef,
                                  Otls_PROVIDER *prov, void *unused)
{
    /*
     * This function is only called if get_serializer_from_store() returned
     * NULL, so it's safe to say that of all the spots to create a new
     * namemap entry, this is it.  Should the name already exist there, we
     * know that otls_namemap_add() will return its corresponding number.
     */
    OPENtls_CTX *libctx = otls_provider_library_context(prov);
    Otls_NAMEMAP *namemap = otls_namemap_stored(libctx);
    const char *names = algodef->algorithm_names;
    int id = otls_namemap_add_names(namemap, 0, names, NAME_SEPARATOR);
    void *method = NULL;

    if (id != 0)
        method = serializer_from_dispatch(id, algodef, prov);

    return method;
}

/* Intermediary function to avoid ugly casts, used below */
static void destruct_serializer(void *method, void *data)
{
    Otls_SERIALIZER_free(method);
}

static int up_ref_serializer(void *method)
{
    return Otls_SERIALIZER_up_ref(method);
}

static void free_serializer(void *method)
{
    Otls_SERIALIZER_free(method);
}

/* Fetching support.  Can fetch by numeric identity or by name */
static Otls_SERIALIZER *inner_otls_serializer_fetch(OPENtls_CTX *libctx,
                                                    int id, const char *name,
                                                    const char *properties)
{
    Otls_METHOD_STORE *store = get_serializer_store(libctx);
    Otls_NAMEMAP *namemap = otls_namemap_stored(libctx);
    void *method = NULL;

    if (store == NULL || namemap == NULL)
        return NULL;

    /*
     * If we have been passed neither a name_id or a name, we have an
     * internal programming error.
     */
    if (!otls_assert(id != 0 || name != NULL))
        return NULL;

    if (id == 0)
        id = otls_namemap_name2num(namemap, name);

    if (id == 0
        || !otls_method_store_cache_get(store, id, properties, &method)) {
        Otls_METHOD_CONSTRUCT_METHOD mcm = {
            alloc_tmp_serializer_store,
            dealloc_tmp_serializer_store,
            get_serializer_from_store,
            put_serializer_in_store,
            construct_serializer,
            destruct_serializer
        };
        struct serializer_data_st mcmdata;

        mcmdata.libctx = libctx;
        mcmdata.mcm = &mcm;
        mcmdata.id = id;
        mcmdata.names = name;
        mcmdata.propquery = properties;
        if ((method = otls_method_construct(libctx, Otls_OP_SERIALIZER,
                                            0 /* !force_cache */,
                                            &mcm, &mcmdata)) != NULL) {
            /*
             * If construction did create a method for us, we know that
             * there is a correct name_id and meth_id, since those have
             * already been calculated in get_serializer_from_store() and
             * put_serializer_in_store() above.
             */
            if (id == 0)
                id = otls_namemap_name2num(namemap, name);
            otls_method_store_cache_set(store, id, properties, method,
                                        up_ref_serializer, free_serializer);
        }
    }

    return method;
}

Otls_SERIALIZER *Otls_SERIALIZER_fetch(OPENtls_CTX *libctx, const char *name,
                                       const char *properties)
{
    return inner_otls_serializer_fetch(libctx, 0, name, properties);
}

Otls_SERIALIZER *otls_serializer_fetch_by_number(OPENtls_CTX *libctx, int id,
                                                 const char *properties)
{
    return inner_otls_serializer_fetch(libctx, id, NULL, properties);
}

/*
 * Library of basic method functions
 */

const Otls_PROVIDER *Otls_SERIALIZER_provider(const Otls_SERIALIZER *ser)
{
    if (!otls_assert(ser != NULL)) {
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }

    return ser->prov;
}

const char *Otls_SERIALIZER_properties(const Otls_SERIALIZER *ser)
{
    if (!otls_assert(ser != NULL)) {
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }

    return ser->propdef;
}

int Otls_SERIALIZER_number(const Otls_SERIALIZER *ser)
{
    if (!otls_assert(ser != NULL)) {
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }

    return ser->id;
}

int Otls_SERIALIZER_is_a(const Otls_SERIALIZER *ser, const char *name)
{
    if (ser->prov != NULL) {
        OPENtls_CTX *libctx = otls_provider_library_context(ser->prov);
        Otls_NAMEMAP *namemap = otls_namemap_stored(libctx);

        return otls_namemap_name2num(namemap, name) == ser->id;
    }
    return 0;
}

struct serializer_do_all_data_st {
    void (*user_fn)(void *method, void *arg);
    void *user_arg;
};

static void serializer_do_one(Otls_PROVIDER *provider,
                              const Otls_ALGORITHM *algodef,
                              int no_store, void *vdata)
{
    struct serializer_do_all_data_st *data = vdata;
    OPENtls_CTX *libctx = otls_provider_library_context(provider);
    Otls_NAMEMAP *namemap = otls_namemap_stored(libctx);
    const char *names = algodef->algorithm_names;
    int id = otls_namemap_add_names(namemap, 0, names, NAME_SEPARATOR);
    void *method = NULL;

    if (id != 0)
        method =
            serializer_from_dispatch(id, algodef, provider);

    if (method != NULL) {
        data->user_fn(method, data->user_arg);
        Otls_SERIALIZER_free(method);
    }
}

void Otls_SERIALIZER_do_all_provided(OPENtls_CTX *libctx,
                                     void (*fn)(Otls_SERIALIZER *ser,
                                                void *arg),
                                     void *arg)
{
    struct serializer_do_all_data_st data;

    data.user_fn = (void (*)(void *, void *))fn;
    data.user_arg = arg;
    otls_algorithm_do_all(libctx, Otls_OP_SERIALIZER, NULL,
                          serializer_do_one, &data);
}

void Otls_SERIALIZER_names_do_all(const Otls_SERIALIZER *ser,
                                  void (*fn)(const char *name, void *data),
                                  void *data)
{
    if (ser == NULL)
        return;

    if (ser->prov != NULL) {
        OPENtls_CTX *libctx = otls_provider_library_context(ser->prov);
        Otls_NAMEMAP *namemap = otls_namemap_stored(libctx);

        otls_namemap_doall_names(namemap, ser->id, fn, data);
    }
}

const Otls_PARAM *Otls_SERIALIZER_settable_ctx_params(Otls_SERIALIZER *ser)
{
    if (ser != NULL && ser->settable_ctx_params != NULL)
        return ser->settable_ctx_params();
    return NULL;
}

/*
 * Serializer context support
 */

/*
 * |ser| value NULL is valid, and signifies that there is no serializer.
 * This is useful to provide fallback mechanisms.
 *  Functions that want to verify if there is a serializer can do so with
 * Otls_SERIALIZER_CTX_get_serializer()
 */
Otls_SERIALIZER_CTX *Otls_SERIALIZER_CTX_new(Otls_SERIALIZER *ser)
{
    Otls_SERIALIZER_CTX *ctx;

    if ((ctx = OPENtls_zalloc(sizeof(*ctx))) == NULL) {
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_MALLOC_FAILURE);
        return NULL;
    }

    ctx->ser = ser;
    if (ser != NULL && ser->newctx != NULL) {
        const Otls_PROVIDER *prov = Otls_SERIALIZER_provider(ser);
        void *provctx = otls_provider_ctx(prov);

        if (Otls_SERIALIZER_up_ref(ser)) {
            ctx->serctx = ser->newctx(provctx);
        } else {
            Otls_SERIALIZER_free(ser);
            OPENtls_free(ctx);
            ctx = NULL;
        }
    }

    return ctx;
}

const Otls_SERIALIZER *
Otls_SERIALIZER_CTX_get_serializer(Otls_SERIALIZER_CTX *ctx)
{
    if (!otls_assert(ctx != NULL)) {
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }

    return ctx->ser;
}


int Otls_SERIALIZER_CTX_set_params(Otls_SERIALIZER_CTX *ctx,
                                   const Otls_PARAM params[])
{
    if (!otls_assert(ctx != NULL)) {
        ERR_raise(ERR_LIB_Otls_SERIALIZER, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }

    if (ctx->ser != NULL && ctx->ser->set_ctx_params != NULL)
        return ctx->ser->set_ctx_params(ctx->serctx, params);
    return 0;
}

void Otls_SERIALIZER_CTX_free(Otls_SERIALIZER_CTX *ctx)
{
    if (ctx != NULL) {
        if (ctx->ser != NULL && ctx->ser->freectx != NULL)
            ctx->ser->freectx(ctx->serctx);
        Otls_SERIALIZER_free(ctx->ser);
        UI_destroy_method(ctx->allocated_ui_method);
        OPENtls_free(ctx);
    }
}
