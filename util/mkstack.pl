#!/usr/local/bin/perl -w

my $safestack = "include/openssl/safestack.h";
my $do_write = 0;

foreach ( @ARGV ) {
    $do_write = 1 if $_ eq "-write";
}

my @asn1setlst;
my @p12stklst;
my @lhashlst;
my @source = (<include/openssl/*.h>, <crypto/include/internal/*.h>, <crypto/*.[ch]>, <crypto/*/*.[ch]>, <ssl/*.[ch]>, <apps/*.[ch]>);
foreach $file (@source) {
    next if -l $file;

    # Open the .c/.h file for reading
    open(IN, "< $file") || die "Can't open $file for reading, $!";

    while(<IN>) {
        next unless /^DECLARE_/;
        if (/^DECLARE_LHASH_OF\(([^)]+)\)/) {
            push @lhashlst, $1;
        }
    }
    close(IN);
}

my $new_stackfile = <<'EOF';
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

EOF

my $old_stackfile;
{
    local $/ = undef;
    open(IN, "$safestack") || die "Can't open $safestack, $!";
    $old_stackfile = <IN>;
    close(IN);
}

my $type_thing;

foreach $type_thing (sort @lhashlst) {
    my $lc_tt = lc $type_thing;
    $new_stackfile .= <<EOF;

# define lh_${type_thing}_new() LHM_lh_new(${type_thing},${lc_tt})
# define lh_${type_thing}_insert(lh,inst) LHM_lh_insert(${type_thing},lh,inst)
# define lh_${type_thing}_retrieve(lh,inst) LHM_lh_retrieve(${type_thing},lh,inst)
# define lh_${type_thing}_delete(lh,inst) LHM_lh_delete(${type_thing},lh,inst)
# define lh_${type_thing}_doall(lh,fn) LHM_lh_doall(${type_thing},lh,fn)
# define lh_${type_thing}_doall_arg(lh,fn,arg_type,arg) \\
  LHM_lh_doall_arg(${type_thing},lh,fn,arg_type,arg)
# define lh_${type_thing}_error(lh) LHM_lh_error(${type_thing},lh)
# define lh_${type_thing}_num_items(lh) LHM_lh_num_items(${type_thing},lh)
# define lh_${type_thing}_down_load(lh) LHM_lh_down_load(${type_thing},lh)
# define lh_${type_thing}_node_stats_bio(lh,out) \\
  LHM_lh_node_stats_bio(${type_thing},lh,out)
# define lh_${type_thing}_node_usage_stats_bio(lh,out) \\
  LHM_lh_node_usage_stats_bio(${type_thing},lh,out)
# define lh_${type_thing}_stats_bio(lh,out) \\
  LHM_lh_stats_bio(${type_thing},lh,out)
# define lh_${type_thing}_free(lh) LHM_lh_free(${type_thing},lh)
EOF
}

$new_stackfile .= <<'EOF';

# ifdef  __cplusplus
}
# endif
#endif
EOF

if ($new_stackfile eq $old_stackfile) {
    print "No changes to $safestack.\n";
}
elsif ($do_write) {
    print "Writing new $safestack.\n";
    open OUT, ">$safestack" || die "Can't open $safestack for writing, $!";
    print OUT $new_stackfile;
    close OUT;
}

exit 0;
