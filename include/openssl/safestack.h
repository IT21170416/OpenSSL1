/* automatically generated by util/mkstack.pl */
/* ====================================================================
 * Copyright (c) 1999-2015 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#ifndef HEADER_SAFESTACK_H
# define HEADER_SAFESTACK_H

# include <openssl/stack.h>
# include <openssl/e_os2.h>

#ifdef __cplusplus
extern "C" {
#endif

# ifndef CHECKED_PTR_OF
#  define CHECKED_PTR_OF(type, p) ((void*) (1 ? p : (type*)0))
# endif

/*
 * In C++ we get problems because an explicit cast is needed from (void *) we
 * use CHECKED_STACK_OF to ensure the correct type is passed in the macros
 * below.
 */

# define CHECKED_STACK_OF(type, p) \
    ((_STACK*) (1 ? p : (STACK_OF(type)*)0))

# define CHECKED_SK_COPY_FUNC(type, p) \
    ((void *(*)(void *)) ((1 ? p : (type *(*)(const type *))0)))

# define CHECKED_SK_FREE_FUNC(type, p) \
    ((void (*)(void *)) ((1 ? p : (void (*)(type *))0)))

# define CHECKED_SK_CMP_FUNC(type, p) \
    ((int (*)(const void *, const void *)) \
        ((1 ? p : (int (*)(const type * const *, const type * const *))0)))

# define STACK_OF(type) struct stack_st_##type
# define PREDECLARE_STACK_OF(type) STACK_OF(type);

# define SKM_DECLARE_STACK_OF(t1, t2, t3) \
    STACK_OF(t1); \
    static ossl_inline int sk_##t1##_num(const STACK_OF(t1) *sk) \
    { \
        return sk_num((_STACK *)sk); \
    } \
    static ossl_inline t2 *sk_##t1##_value(const STACK_OF(t1) *sk, int idx) \
    { \
        return sk_value((_STACK *)sk, idx); \
    } \
    static ossl_inline STACK_OF(t1) *sk_##t1##_new(int (*cmpf)(const t3 * const *a, const t3 * const *b)) \
    { \
        return (STACK_OF(t1) *)sk_new((int (*)(const void *a, const void *b))cmpf); \
    } \
    static ossl_inline STACK_OF(t1) *sk_##t1##_new_null(void) \
    { \
        return (STACK_OF(t1) *)sk_new_null(); \
    } \
    static ossl_inline void sk_##t1##_free(const STACK_OF(t1) *sk) \
    { \
        sk_free((_STACK *)sk); \
    } \
    static ossl_inline void sk_##t1##_zero(const STACK_OF(t1) *sk) \
    { \
        sk_zero((_STACK *)sk); \
    } \
    static ossl_inline t2 *sk_##t1##_delete(STACK_OF(t1) *sk, int i) \
    { \
        return sk_delete((_STACK *)sk, i); \
    } \
    static ossl_inline t2 *sk_##t1##_delete_ptr(STACK_OF(t1) *sk, t2 *ptr) \
    { \
        return sk_delete_ptr((_STACK *)sk, (void *)ptr); \
    } \
    static ossl_inline int sk_##t1##_push(STACK_OF(t1) *sk, t2 *ptr) \
    { \
        return sk_push((_STACK *)sk, (void *)ptr); \
    } \
    static ossl_inline int sk_##t1##_unshift(STACK_OF(t1) *sk, t2 *ptr) \
    { \
        return sk_unshift((_STACK *)sk, (void *)ptr); \
    } \
    static ossl_inline t2 *sk_##t1##_pop(STACK_OF(t1) *sk) \
    { \
        return (t2 *)sk_pop((_STACK *)sk); \
    } \
    static ossl_inline t2 *sk_##t1##_shift(STACK_OF(t1) *sk) \
    { \
        return (t2 *)sk_shift((_STACK *)sk); \
    } \
    static ossl_inline void sk_##t1##_pop_free(STACK_OF(t1) *sk, void (*func)(t3 *a)) \
    { \
        sk_pop_free((_STACK *)sk, (void (*)(void *))func); \
    } \
    static ossl_inline int sk_##t1##_insert(STACK_OF(t1) *sk, t2 *ptr, int idx) \
    { \
        return sk_insert((_STACK *)sk, (void *)ptr, idx); \
    } \
    static ossl_inline t2 *sk_##t1##_set(STACK_OF(t1) *sk, int idx, t2 *ptr) \
    { \
        return sk_set((_STACK *)sk, idx, (void *)ptr); \
    } \
    static ossl_inline int sk_##t1##_find(STACK_OF(t1) *sk, t2 *ptr) \
    { \
        return sk_find((_STACK *)sk, (void *)ptr); \
    } \
    static ossl_inline int sk_##t1##_find_ex(STACK_OF(t1) *sk, t2 *ptr) \
    { \
        return sk_find_ex((_STACK *)sk, (void *)ptr); \
    } \
    static ossl_inline void sk_##t1##_sort(const STACK_OF(t1) *sk) \
    { \
        sk_sort((_STACK *)sk); \
    } \
    static ossl_inline int sk_##t1##_is_sorted(const STACK_OF(t1) *sk) \
    { \
        return sk_is_sorted((_STACK *)sk); \
    } \
    static ossl_inline STACK_OF(t1) * sk_##t1##_dup(STACK_OF(t1) *sk) \
    { \
        return (STACK_OF(t1) *)sk_dup((_STACK *)sk); \
    } \
    static ossl_inline STACK_OF(t1) *sk_##t1##_deep_copy(STACK_OF(t1) *sk, \
                                                    t3 *(*copyfn)(const t3 *), \
                                                    void (*freefn)(t3 *)) \
    { \
        return (STACK_OF(t1) *)sk_deep_copy((_STACK *)sk, \
                                            (void * (*)(void *a))copyfn, \
                                            (void (*)(void *a))freefn); \
    } \
    static ossl_inline int (*sk_##t1##_set_cmp_func(STACK_OF(t1) *sk, int (*cmpf)(const t3 * const *a, const t3 * const *b)))(const t3 * const *, const t3 * const *) \
    { \
        return (int (*)(const t3 * const *,const t3 * const *))sk_set_cmp_func((_STACK *)sk, (int (*)(const void *a, const void *b))cmpf); \
    }

# define DECLARE_SPECIAL_STACK_OF(t1, t2) SKM_DECLARE_STACK_OF(t1, t2, t2)
# define DECLARE_STACK_OF(t) SKM_DECLARE_STACK_OF(t, t, t)
# define DECLARE_CONST_STACK_OF(t) SKM_DECLARE_STACK_OF(t, const t, t)

/*-
 * Strings are special: normally an lhash entry will point to a single
 * (somewhat) mutable object. In the case of strings:
 *
 * a) Instead of a single char, there is an array of chars, NUL-terminated.
 * b) The string may have be immutable.
 *
 * So, they need their own declarations. Especially important for
 * type-checking tools, such as Deputy.
 *
 * In practice, however, it appears to be hard to have a const
 * string. For now, I'm settling for dealing with the fact it is a
 * string at all.
 */
typedef char *OPENSSL_STRING;
typedef const char *OPENSSL_CSTRING;

/*-
 * Confusingly, LHASH_OF(STRING) deals with char ** throughout, but
 * STACK_OF(STRING) is really more like STACK_OF(char), only, as mentioned
 * above, instead of a single char each entry is a NUL-terminated array of
 * chars. So, we have to implement STRING specially for STACK_OF. This is
 * dealt with in the autogenerated macros below.
 */
DECLARE_SPECIAL_STACK_OF(OPENSSL_STRING, char)

/*
 * Similarly, we sometimes use a block of characters, NOT nul-terminated.
 * These should also be distinguished from "normal" stacks.
 */
typedef void *OPENSSL_BLOCK;
DECLARE_SPECIAL_STACK_OF(OPENSSL_BLOCK, void)

/*
 * This file is automatically generated by util/mkstack.pl
 * Do not edit!
 */

/*
 * SKM_sk_... stack macros are internal to safestack.h: never use them
 * directly, use sk_<type>_... instead
 */


# define lh_ADDED_OBJ_new() LHM_lh_new(ADDED_OBJ,added_obj)
# define lh_ADDED_OBJ_insert(lh,inst) LHM_lh_insert(ADDED_OBJ,lh,inst)
# define lh_ADDED_OBJ_retrieve(lh,inst) LHM_lh_retrieve(ADDED_OBJ,lh,inst)
# define lh_ADDED_OBJ_delete(lh,inst) LHM_lh_delete(ADDED_OBJ,lh,inst)
# define lh_ADDED_OBJ_doall(lh,fn) LHM_lh_doall(ADDED_OBJ,lh,fn)
# define lh_ADDED_OBJ_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(ADDED_OBJ,lh,fn,arg_type,arg)
# define lh_ADDED_OBJ_error(lh) LHM_lh_error(ADDED_OBJ,lh)
# define lh_ADDED_OBJ_num_items(lh) LHM_lh_num_items(ADDED_OBJ,lh)
# define lh_ADDED_OBJ_down_load(lh) LHM_lh_down_load(ADDED_OBJ,lh)
# define lh_ADDED_OBJ_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(ADDED_OBJ,lh,out)
# define lh_ADDED_OBJ_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(ADDED_OBJ,lh,out)
# define lh_ADDED_OBJ_stats_bio(lh,out) \
  LHM_lh_stats_bio(ADDED_OBJ,lh,out)
# define lh_ADDED_OBJ_free(lh) LHM_lh_free(ADDED_OBJ,lh)

# define lh_APP_INFO_new() LHM_lh_new(APP_INFO,app_info)
# define lh_APP_INFO_insert(lh,inst) LHM_lh_insert(APP_INFO,lh,inst)
# define lh_APP_INFO_retrieve(lh,inst) LHM_lh_retrieve(APP_INFO,lh,inst)
# define lh_APP_INFO_delete(lh,inst) LHM_lh_delete(APP_INFO,lh,inst)
# define lh_APP_INFO_doall(lh,fn) LHM_lh_doall(APP_INFO,lh,fn)
# define lh_APP_INFO_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(APP_INFO,lh,fn,arg_type,arg)
# define lh_APP_INFO_error(lh) LHM_lh_error(APP_INFO,lh)
# define lh_APP_INFO_num_items(lh) LHM_lh_num_items(APP_INFO,lh)
# define lh_APP_INFO_down_load(lh) LHM_lh_down_load(APP_INFO,lh)
# define lh_APP_INFO_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(APP_INFO,lh,out)
# define lh_APP_INFO_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(APP_INFO,lh,out)
# define lh_APP_INFO_stats_bio(lh,out) \
  LHM_lh_stats_bio(APP_INFO,lh,out)
# define lh_APP_INFO_free(lh) LHM_lh_free(APP_INFO,lh)

# define lh_CONF_VALUE_new() LHM_lh_new(CONF_VALUE,conf_value)
# define lh_CONF_VALUE_insert(lh,inst) LHM_lh_insert(CONF_VALUE,lh,inst)
# define lh_CONF_VALUE_retrieve(lh,inst) LHM_lh_retrieve(CONF_VALUE,lh,inst)
# define lh_CONF_VALUE_delete(lh,inst) LHM_lh_delete(CONF_VALUE,lh,inst)
# define lh_CONF_VALUE_doall(lh,fn) LHM_lh_doall(CONF_VALUE,lh,fn)
# define lh_CONF_VALUE_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(CONF_VALUE,lh,fn,arg_type,arg)
# define lh_CONF_VALUE_error(lh) LHM_lh_error(CONF_VALUE,lh)
# define lh_CONF_VALUE_num_items(lh) LHM_lh_num_items(CONF_VALUE,lh)
# define lh_CONF_VALUE_down_load(lh) LHM_lh_down_load(CONF_VALUE,lh)
# define lh_CONF_VALUE_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(CONF_VALUE,lh,out)
# define lh_CONF_VALUE_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(CONF_VALUE,lh,out)
# define lh_CONF_VALUE_stats_bio(lh,out) \
  LHM_lh_stats_bio(CONF_VALUE,lh,out)
# define lh_CONF_VALUE_free(lh) LHM_lh_free(CONF_VALUE,lh)

# define lh_ENGINE_PILE_new() LHM_lh_new(ENGINE_PILE,engine_pile)
# define lh_ENGINE_PILE_insert(lh,inst) LHM_lh_insert(ENGINE_PILE,lh,inst)
# define lh_ENGINE_PILE_retrieve(lh,inst) LHM_lh_retrieve(ENGINE_PILE,lh,inst)
# define lh_ENGINE_PILE_delete(lh,inst) LHM_lh_delete(ENGINE_PILE,lh,inst)
# define lh_ENGINE_PILE_doall(lh,fn) LHM_lh_doall(ENGINE_PILE,lh,fn)
# define lh_ENGINE_PILE_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(ENGINE_PILE,lh,fn,arg_type,arg)
# define lh_ENGINE_PILE_error(lh) LHM_lh_error(ENGINE_PILE,lh)
# define lh_ENGINE_PILE_num_items(lh) LHM_lh_num_items(ENGINE_PILE,lh)
# define lh_ENGINE_PILE_down_load(lh) LHM_lh_down_load(ENGINE_PILE,lh)
# define lh_ENGINE_PILE_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(ENGINE_PILE,lh,out)
# define lh_ENGINE_PILE_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(ENGINE_PILE,lh,out)
# define lh_ENGINE_PILE_stats_bio(lh,out) \
  LHM_lh_stats_bio(ENGINE_PILE,lh,out)
# define lh_ENGINE_PILE_free(lh) LHM_lh_free(ENGINE_PILE,lh)

# define lh_ERR_STATE_new() LHM_lh_new(ERR_STATE,err_state)
# define lh_ERR_STATE_insert(lh,inst) LHM_lh_insert(ERR_STATE,lh,inst)
# define lh_ERR_STATE_retrieve(lh,inst) LHM_lh_retrieve(ERR_STATE,lh,inst)
# define lh_ERR_STATE_delete(lh,inst) LHM_lh_delete(ERR_STATE,lh,inst)
# define lh_ERR_STATE_doall(lh,fn) LHM_lh_doall(ERR_STATE,lh,fn)
# define lh_ERR_STATE_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(ERR_STATE,lh,fn,arg_type,arg)
# define lh_ERR_STATE_error(lh) LHM_lh_error(ERR_STATE,lh)
# define lh_ERR_STATE_num_items(lh) LHM_lh_num_items(ERR_STATE,lh)
# define lh_ERR_STATE_down_load(lh) LHM_lh_down_load(ERR_STATE,lh)
# define lh_ERR_STATE_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(ERR_STATE,lh,out)
# define lh_ERR_STATE_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(ERR_STATE,lh,out)
# define lh_ERR_STATE_stats_bio(lh,out) \
  LHM_lh_stats_bio(ERR_STATE,lh,out)
# define lh_ERR_STATE_free(lh) LHM_lh_free(ERR_STATE,lh)

# define lh_ERR_STRING_DATA_new() LHM_lh_new(ERR_STRING_DATA,err_string_data)
# define lh_ERR_STRING_DATA_insert(lh,inst) LHM_lh_insert(ERR_STRING_DATA,lh,inst)
# define lh_ERR_STRING_DATA_retrieve(lh,inst) LHM_lh_retrieve(ERR_STRING_DATA,lh,inst)
# define lh_ERR_STRING_DATA_delete(lh,inst) LHM_lh_delete(ERR_STRING_DATA,lh,inst)
# define lh_ERR_STRING_DATA_doall(lh,fn) LHM_lh_doall(ERR_STRING_DATA,lh,fn)
# define lh_ERR_STRING_DATA_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(ERR_STRING_DATA,lh,fn,arg_type,arg)
# define lh_ERR_STRING_DATA_error(lh) LHM_lh_error(ERR_STRING_DATA,lh)
# define lh_ERR_STRING_DATA_num_items(lh) LHM_lh_num_items(ERR_STRING_DATA,lh)
# define lh_ERR_STRING_DATA_down_load(lh) LHM_lh_down_load(ERR_STRING_DATA,lh)
# define lh_ERR_STRING_DATA_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(ERR_STRING_DATA,lh,out)
# define lh_ERR_STRING_DATA_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(ERR_STRING_DATA,lh,out)
# define lh_ERR_STRING_DATA_stats_bio(lh,out) \
  LHM_lh_stats_bio(ERR_STRING_DATA,lh,out)
# define lh_ERR_STRING_DATA_free(lh) LHM_lh_free(ERR_STRING_DATA,lh)

# define lh_FUNCTION_new() LHM_lh_new(FUNCTION,function)
# define lh_FUNCTION_insert(lh,inst) LHM_lh_insert(FUNCTION,lh,inst)
# define lh_FUNCTION_retrieve(lh,inst) LHM_lh_retrieve(FUNCTION,lh,inst)
# define lh_FUNCTION_delete(lh,inst) LHM_lh_delete(FUNCTION,lh,inst)
# define lh_FUNCTION_doall(lh,fn) LHM_lh_doall(FUNCTION,lh,fn)
# define lh_FUNCTION_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(FUNCTION,lh,fn,arg_type,arg)
# define lh_FUNCTION_error(lh) LHM_lh_error(FUNCTION,lh)
# define lh_FUNCTION_num_items(lh) LHM_lh_num_items(FUNCTION,lh)
# define lh_FUNCTION_down_load(lh) LHM_lh_down_load(FUNCTION,lh)
# define lh_FUNCTION_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(FUNCTION,lh,out)
# define lh_FUNCTION_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(FUNCTION,lh,out)
# define lh_FUNCTION_stats_bio(lh,out) \
  LHM_lh_stats_bio(FUNCTION,lh,out)
# define lh_FUNCTION_free(lh) LHM_lh_free(FUNCTION,lh)

# define lh_MEM_new() LHM_lh_new(MEM,mem)
# define lh_MEM_insert(lh,inst) LHM_lh_insert(MEM,lh,inst)
# define lh_MEM_retrieve(lh,inst) LHM_lh_retrieve(MEM,lh,inst)
# define lh_MEM_delete(lh,inst) LHM_lh_delete(MEM,lh,inst)
# define lh_MEM_doall(lh,fn) LHM_lh_doall(MEM,lh,fn)
# define lh_MEM_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(MEM,lh,fn,arg_type,arg)
# define lh_MEM_error(lh) LHM_lh_error(MEM,lh)
# define lh_MEM_num_items(lh) LHM_lh_num_items(MEM,lh)
# define lh_MEM_down_load(lh) LHM_lh_down_load(MEM,lh)
# define lh_MEM_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(MEM,lh,out)
# define lh_MEM_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(MEM,lh,out)
# define lh_MEM_stats_bio(lh,out) \
  LHM_lh_stats_bio(MEM,lh,out)
# define lh_MEM_free(lh) LHM_lh_free(MEM,lh)

# define lh_OBJ_NAME_new() LHM_lh_new(OBJ_NAME,obj_name)
# define lh_OBJ_NAME_insert(lh,inst) LHM_lh_insert(OBJ_NAME,lh,inst)
# define lh_OBJ_NAME_retrieve(lh,inst) LHM_lh_retrieve(OBJ_NAME,lh,inst)
# define lh_OBJ_NAME_delete(lh,inst) LHM_lh_delete(OBJ_NAME,lh,inst)
# define lh_OBJ_NAME_doall(lh,fn) LHM_lh_doall(OBJ_NAME,lh,fn)
# define lh_OBJ_NAME_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(OBJ_NAME,lh,fn,arg_type,arg)
# define lh_OBJ_NAME_error(lh) LHM_lh_error(OBJ_NAME,lh)
# define lh_OBJ_NAME_num_items(lh) LHM_lh_num_items(OBJ_NAME,lh)
# define lh_OBJ_NAME_down_load(lh) LHM_lh_down_load(OBJ_NAME,lh)
# define lh_OBJ_NAME_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(OBJ_NAME,lh,out)
# define lh_OBJ_NAME_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(OBJ_NAME,lh,out)
# define lh_OBJ_NAME_stats_bio(lh,out) \
  LHM_lh_stats_bio(OBJ_NAME,lh,out)
# define lh_OBJ_NAME_free(lh) LHM_lh_free(OBJ_NAME,lh)

# define lh_OPENSSL_CSTRING_new() LHM_lh_new(OPENSSL_CSTRING,openssl_cstring)
# define lh_OPENSSL_CSTRING_insert(lh,inst) LHM_lh_insert(OPENSSL_CSTRING,lh,inst)
# define lh_OPENSSL_CSTRING_retrieve(lh,inst) LHM_lh_retrieve(OPENSSL_CSTRING,lh,inst)
# define lh_OPENSSL_CSTRING_delete(lh,inst) LHM_lh_delete(OPENSSL_CSTRING,lh,inst)
# define lh_OPENSSL_CSTRING_doall(lh,fn) LHM_lh_doall(OPENSSL_CSTRING,lh,fn)
# define lh_OPENSSL_CSTRING_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(OPENSSL_CSTRING,lh,fn,arg_type,arg)
# define lh_OPENSSL_CSTRING_error(lh) LHM_lh_error(OPENSSL_CSTRING,lh)
# define lh_OPENSSL_CSTRING_num_items(lh) LHM_lh_num_items(OPENSSL_CSTRING,lh)
# define lh_OPENSSL_CSTRING_down_load(lh) LHM_lh_down_load(OPENSSL_CSTRING,lh)
# define lh_OPENSSL_CSTRING_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(OPENSSL_CSTRING,lh,out)
# define lh_OPENSSL_CSTRING_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(OPENSSL_CSTRING,lh,out)
# define lh_OPENSSL_CSTRING_stats_bio(lh,out) \
  LHM_lh_stats_bio(OPENSSL_CSTRING,lh,out)
# define lh_OPENSSL_CSTRING_free(lh) LHM_lh_free(OPENSSL_CSTRING,lh)

# define lh_OPENSSL_STRING_new() LHM_lh_new(OPENSSL_STRING,openssl_string)
# define lh_OPENSSL_STRING_insert(lh,inst) LHM_lh_insert(OPENSSL_STRING,lh,inst)
# define lh_OPENSSL_STRING_retrieve(lh,inst) LHM_lh_retrieve(OPENSSL_STRING,lh,inst)
# define lh_OPENSSL_STRING_delete(lh,inst) LHM_lh_delete(OPENSSL_STRING,lh,inst)
# define lh_OPENSSL_STRING_doall(lh,fn) LHM_lh_doall(OPENSSL_STRING,lh,fn)
# define lh_OPENSSL_STRING_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(OPENSSL_STRING,lh,fn,arg_type,arg)
# define lh_OPENSSL_STRING_error(lh) LHM_lh_error(OPENSSL_STRING,lh)
# define lh_OPENSSL_STRING_num_items(lh) LHM_lh_num_items(OPENSSL_STRING,lh)
# define lh_OPENSSL_STRING_down_load(lh) LHM_lh_down_load(OPENSSL_STRING,lh)
# define lh_OPENSSL_STRING_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(OPENSSL_STRING,lh,out)
# define lh_OPENSSL_STRING_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(OPENSSL_STRING,lh,out)
# define lh_OPENSSL_STRING_stats_bio(lh,out) \
  LHM_lh_stats_bio(OPENSSL_STRING,lh,out)
# define lh_OPENSSL_STRING_free(lh) LHM_lh_free(OPENSSL_STRING,lh)

# define lh_SSL_SESSION_new() LHM_lh_new(SSL_SESSION,ssl_session)
# define lh_SSL_SESSION_insert(lh,inst) LHM_lh_insert(SSL_SESSION,lh,inst)
# define lh_SSL_SESSION_retrieve(lh,inst) LHM_lh_retrieve(SSL_SESSION,lh,inst)
# define lh_SSL_SESSION_delete(lh,inst) LHM_lh_delete(SSL_SESSION,lh,inst)
# define lh_SSL_SESSION_doall(lh,fn) LHM_lh_doall(SSL_SESSION,lh,fn)
# define lh_SSL_SESSION_doall_arg(lh,fn,arg_type,arg) \
  LHM_lh_doall_arg(SSL_SESSION,lh,fn,arg_type,arg)
# define lh_SSL_SESSION_error(lh) LHM_lh_error(SSL_SESSION,lh)
# define lh_SSL_SESSION_num_items(lh) LHM_lh_num_items(SSL_SESSION,lh)
# define lh_SSL_SESSION_down_load(lh) LHM_lh_down_load(SSL_SESSION,lh)
# define lh_SSL_SESSION_node_stats_bio(lh,out) \
  LHM_lh_node_stats_bio(SSL_SESSION,lh,out)
# define lh_SSL_SESSION_node_usage_stats_bio(lh,out) \
  LHM_lh_node_usage_stats_bio(SSL_SESSION,lh,out)
# define lh_SSL_SESSION_stats_bio(lh,out) \
  LHM_lh_stats_bio(SSL_SESSION,lh,out)
# define lh_SSL_SESSION_free(lh) LHM_lh_free(SSL_SESSION,lh)

# ifdef  __cplusplus
}
# endif
#endif
