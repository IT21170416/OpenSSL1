/*
 * Copyright 2016-2017 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* Internal tests for the modes module */

#include <stdio.h>
#include <string.h>

#include <openssl/aes.h>
#include <openssl/modes.h>
#include "../crypto/modes/modes_lcl.h"
#include "testutil.h"
#include "e_os.h"

typedef struct {
    size_t size;
    const unsigned char *data;
}  SIZED_DATA;

/**********************************************************************
 *
 * Test of cts128
 *
 ***/

/* cts128 test vectors from RFC 3962 */
static const unsigned char cts128_test_key[16] = "chicken teriyaki";
static const unsigned char cts128_test_input[64] =
    "I would like the" " General Gau's C"
    "hicken, please, " "and wonton soup.";
static const unsigned char cts128_test_iv[] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static const unsigned char vector_17[17] = {
    0xc6, 0x35, 0x35, 0x68, 0xf2, 0xbf, 0x8c, 0xb4,
    0xd8, 0xa5, 0x80, 0x36, 0x2d, 0xa7, 0xff, 0x7f,
    0x97
};

static const unsigned char vector_31[31] = {
    0xfc, 0x00, 0x78, 0x3e, 0x0e, 0xfd, 0xb2, 0xc1,
    0xd4, 0x45, 0xd4, 0xc8, 0xef, 0xf7, 0xed, 0x22,
    0x97, 0x68, 0x72, 0x68, 0xd6, 0xec, 0xcc, 0xc0,
    0xc0, 0x7b, 0x25, 0xe2, 0x5e, 0xcf, 0xe5
};

static const unsigned char vector_32[32] = {
    0x39, 0x31, 0x25, 0x23, 0xa7, 0x86, 0x62, 0xd5,
    0xbe, 0x7f, 0xcb, 0xcc, 0x98, 0xeb, 0xf5, 0xa8,
    0x97, 0x68, 0x72, 0x68, 0xd6, 0xec, 0xcc, 0xc0,
    0xc0, 0x7b, 0x25, 0xe2, 0x5e, 0xcf, 0xe5, 0x84
};

static const unsigned char vector_47[47] = {
    0x97, 0x68, 0x72, 0x68, 0xd6, 0xec, 0xcc, 0xc0,
    0xc0, 0x7b, 0x25, 0xe2, 0x5e, 0xcf, 0xe5, 0x84,
    0xb3, 0xff, 0xfd, 0x94, 0x0c, 0x16, 0xa1, 0x8c,
    0x1b, 0x55, 0x49, 0xd2, 0xf8, 0x38, 0x02, 0x9e,
    0x39, 0x31, 0x25, 0x23, 0xa7, 0x86, 0x62, 0xd5,
    0xbe, 0x7f, 0xcb, 0xcc, 0x98, 0xeb, 0xf5
};

static const unsigned char vector_48[48] = {
    0x97, 0x68, 0x72, 0x68, 0xd6, 0xec, 0xcc, 0xc0,
    0xc0, 0x7b, 0x25, 0xe2, 0x5e, 0xcf, 0xe5, 0x84,
    0x9d, 0xad, 0x8b, 0xbb, 0x96, 0xc4, 0xcd, 0xc0,
    0x3b, 0xc1, 0x03, 0xe1, 0xa1, 0x94, 0xbb, 0xd8,
    0x39, 0x31, 0x25, 0x23, 0xa7, 0x86, 0x62, 0xd5,
    0xbe, 0x7f, 0xcb, 0xcc, 0x98, 0xeb, 0xf5, 0xa8
};

static const unsigned char vector_64[64] = {
    0x97, 0x68, 0x72, 0x68, 0xd6, 0xec, 0xcc, 0xc0,
    0xc0, 0x7b, 0x25, 0xe2, 0x5e, 0xcf, 0xe5, 0x84,
    0x39, 0x31, 0x25, 0x23, 0xa7, 0x86, 0x62, 0xd5,
    0xbe, 0x7f, 0xcb, 0xcc, 0x98, 0xeb, 0xf5, 0xa8,
    0x48, 0x07, 0xef, 0xe8, 0x36, 0xee, 0x89, 0xa5,
    0x26, 0x73, 0x0d, 0xbc, 0x2f, 0x7b, 0xc8, 0x40,
    0x9d, 0xad, 0x8b, 0xbb, 0x96, 0xc4, 0xcd, 0xc0,
    0x3b, 0xc1, 0x03, 0xe1, 0xa1, 0x94, 0xbb, 0xd8
};

#define CTS128_TEST_VECTOR(len)                 \
    {                                           \
        sizeof(vector_##len), vector_##len      \
    }
static const SIZED_DATA cts128_vectors[] = {
    CTS128_TEST_VECTOR(17),
    CTS128_TEST_VECTOR(31),
    CTS128_TEST_VECTOR(32),
    CTS128_TEST_VECTOR(47),
    CTS128_TEST_VECTOR(48),
    CTS128_TEST_VECTOR(64),
};

static AES_KEY *cts128_encrypt_key_schedule()
{
    static int init_key = 1;
    static AES_KEY ks;

    if (init_key) {
        AES_set_encrypt_key(cts128_test_key, 128, &ks);
        init_key = 0;
    }
    return &ks;
}

static AES_KEY *cts128_decrypt_key_schedule()
{
    static int init_key = 1;
    static AES_KEY ks;

    if (init_key) {
        AES_set_decrypt_key(cts128_test_key, 128, &ks);
        init_key = 0;
    }
    return &ks;
}

typedef struct {
    const char *case_name;
    int num;
    size_t (*transform_output)(const unsigned char *in, unsigned char *out,
                               size_t len);
    size_t (*encrypt_block)(const unsigned char *in,
                            unsigned char *out, size_t len,
                            const void *key, unsigned char ivec[16],
                            block128_f block);
    size_t (*encrypt)(const unsigned char *in, unsigned char *out,
                      size_t len, const void *key,
                      unsigned char ivec[16], cbc128_f cbc);
    size_t (*decrypt_block)(const unsigned char *in,
                            unsigned char *out, size_t len,
                            const void *key, unsigned char ivec[16],
                            block128_f block);
    size_t (*decrypt)(const unsigned char *in, unsigned char *out,
                      size_t len, const void *key,
                      unsigned char ivec[16], cbc128_f cbc);
} CTS128_FIXTURE;


static CTS128_FIXTURE setup_cts128(const char *const test_case_name)
{
    CTS128_FIXTURE fixture;
    fixture.case_name = test_case_name;
    return fixture;
}

static size_t transform_output(const unsigned char *in, unsigned char *out,
                               size_t len)
{
    size_t tail;

    memcpy(out, in, len);
    if ((tail = len % 16) == 0)
        tail = 16;
    tail += 16;

    return tail;
}

static size_t transform_output_nist(const unsigned char *in, unsigned char *out,
                                    size_t len)
{
    size_t tail;

    if ((tail = len % 16) == 0)
        tail = 16;
    len -= 16 + tail;
    memcpy(out, in, len);
    /* flip two last blocks */
    memcpy(out + len, in + len + 16, tail);
    memcpy(out + len + tail, in + len, 16);
    len += 16 + tail;
    tail = 16;

    return tail;
}

static int execute_cts128(CTS128_FIXTURE fixture)
{
    const unsigned char *test_iv = cts128_test_iv;
    size_t test_iv_len = sizeof(cts128_test_iv);
    const unsigned char *orig_vector = cts128_vectors[fixture.num].data;
    size_t len = cts128_vectors[fixture.num].size;
    const unsigned char *test_input = cts128_test_input;
    const AES_KEY *encrypt_key_schedule = cts128_encrypt_key_schedule();
    const AES_KEY *decrypt_key_schedule = cts128_decrypt_key_schedule();
    unsigned char iv[16];
    /* The largest test inputs are = 64 bytes. */
    unsigned char cleartext[64], ciphertext[64], vector[64];
    size_t tail;

    TEST_info("%s_vector_%lu", fixture.case_name, (unsigned long)len);

    tail = fixture.transform_output(orig_vector, vector, len);

    /* test block-based encryption */
    memcpy(iv, test_iv, test_iv_len);
    fixture.encrypt_block(test_input, ciphertext, len,
                          encrypt_key_schedule, iv,
                          (block128_f)AES_encrypt);
    if (!TEST_mem_eq(ciphertext, len, vector, len))
        return 0;

    if (!TEST_mem_eq(iv, sizeof(iv), vector + len - tail, sizeof(iv)))
        return 0;

    /* test block-based decryption */
    memcpy(iv, test_iv, test_iv_len);
    fixture.decrypt_block(ciphertext, cleartext, len,
                          decrypt_key_schedule, iv,
                          (block128_f)AES_decrypt);
    if (!TEST_mem_eq(cleartext, len, test_input, len))
        return 0;
    if (!TEST_mem_eq(iv, sizeof(iv), vector + len - tail, sizeof(iv)))
        return 0;

    /* test streamed encryption */
    memcpy(iv, test_iv, test_iv_len);
    fixture.encrypt(test_input, ciphertext, len, encrypt_key_schedule,
                    iv, (cbc128_f) AES_cbc_encrypt);
    if (!TEST_mem_eq(ciphertext, len, vector, len))
        return 0;
    if (!TEST_mem_eq(iv, sizeof(iv), vector + len - tail, sizeof(iv)))
        return 0;

    /* test streamed decryption */
    memcpy(iv, test_iv, test_iv_len);
    fixture.decrypt(ciphertext, cleartext, len, decrypt_key_schedule, iv,
                    (cbc128_f)AES_cbc_encrypt);
    if (!TEST_mem_eq(cleartext, len, test_input, len))
        return 0;
    if (!TEST_mem_eq(iv, sizeof(iv), vector + len - tail, sizeof(iv)))
        return 0;

    return 1;
}

static int test_cts128(int idx)
{
    SETUP_TEST_FIXTURE(CTS128_FIXTURE, setup_cts128);
    fixture.transform_output = transform_output;
    fixture.encrypt_block = CRYPTO_cts128_encrypt_block;
    fixture.encrypt = CRYPTO_cts128_encrypt;
    fixture.decrypt_block = CRYPTO_cts128_decrypt_block;
    fixture.decrypt = CRYPTO_cts128_decrypt;
    fixture.case_name = "cts128";
    fixture.num = idx;
    EXECUTE_TEST_NO_TEARDOWN(execute_cts128);
}

static int test_cts128_nist(int idx)
{
    SETUP_TEST_FIXTURE(CTS128_FIXTURE, setup_cts128);
    fixture.transform_output = transform_output_nist;
    fixture.encrypt_block = CRYPTO_nistcts128_encrypt_block;
    fixture.encrypt = CRYPTO_nistcts128_encrypt;
    fixture.decrypt_block = CRYPTO_nistcts128_decrypt_block;
    fixture.decrypt = CRYPTO_nistcts128_decrypt;
    fixture.case_name = "cts128_nist";
    fixture.num = idx;
    EXECUTE_TEST_NO_TEARDOWN(execute_cts128);
}

/*
 *
 * Test of gcm128
 *
 */

/* Test Case 1 */
static const u8 K1[16], P1[] = { 0 }, A1[] = { 0 }, IV1[12], C1[] = { 0 };
static const u8 T1[] = {
    0x58, 0xe2, 0xfc, 0xce, 0xfa, 0x7e, 0x30, 0x61,
    0x36, 0x7f, 0x1d, 0x57, 0xa4, 0xe7, 0x45, 0x5a
};

/* Test Case 2 */
# define K2 K1
# define A2 A1
# define IV2 IV1
static const u8 P2[16];
static const u8 C2[] = {
    0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
    0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78
};

static const u8 T2[] = {
    0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec, 0x13, 0xbd,
    0xf5, 0x3a, 0x67, 0xb2, 0x12, 0x57, 0xbd, 0xdf
};

/* Test Case 3 */
# define A3 A2
static const u8 K3[] = {
    0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
    0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
};

static const u8 P3[] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55
};

static const u8 IV3[] = {
    0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
    0xde, 0xca, 0xf8, 0x88
};

static const u8 C3[] = {
    0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
    0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
    0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
    0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
    0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
    0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
    0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
    0x3d, 0x58, 0xe0, 0x91, 0x47, 0x3f, 0x59, 0x85
};

static const u8 T3[] = {
    0x4d, 0x5c, 0x2a, 0xf3, 0x27, 0xcd, 0x64, 0xa6,
    0x2c, 0xf3, 0x5a, 0xbd, 0x2b, 0xa6, 0xfa, 0xb4
};

/* Test Case 4 */
# define K4 K3
# define IV4 IV3
static const u8 P4[] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39
};

static const u8 A4[] = {
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xab, 0xad, 0xda, 0xd2
};

static const u8 C4[] = {
    0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
    0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
    0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
    0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
    0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
    0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
    0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
    0x3d, 0x58, 0xe0, 0x91
};

static const u8 T4[] = {
    0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb,
    0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47
};

/* Test Case 5 */
# define K5 K4
# define P5 P4
# define A5 A4
static const u8 IV5[] = {
    0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad
};

static const u8 C5[] = {
    0x61, 0x35, 0x3b, 0x4c, 0x28, 0x06, 0x93, 0x4a,
    0x77, 0x7f, 0xf5, 0x1f, 0xa2, 0x2a, 0x47, 0x55,
    0x69, 0x9b, 0x2a, 0x71, 0x4f, 0xcd, 0xc6, 0xf8,
    0x37, 0x66, 0xe5, 0xf9, 0x7b, 0x6c, 0x74, 0x23,
    0x73, 0x80, 0x69, 0x00, 0xe4, 0x9f, 0x24, 0xb2,
    0x2b, 0x09, 0x75, 0x44, 0xd4, 0x89, 0x6b, 0x42,
    0x49, 0x89, 0xb5, 0xe1, 0xeb, 0xac, 0x0f, 0x07,
    0xc2, 0x3f, 0x45, 0x98
};

static const u8 T5[] = {
    0x36, 0x12, 0xd2, 0xe7, 0x9e, 0x3b, 0x07, 0x85,
    0x56, 0x1b, 0xe1, 0x4a, 0xac, 0xa2, 0xfc, 0xcb
};

/* Test Case 6 */
# define K6 K5
# define P6 P5
# define A6 A5
static const u8 IV6[] = {
    0x93, 0x13, 0x22, 0x5d, 0xf8, 0x84, 0x06, 0xe5,
    0x55, 0x90, 0x9c, 0x5a, 0xff, 0x52, 0x69, 0xaa,
    0x6a, 0x7a, 0x95, 0x38, 0x53, 0x4f, 0x7d, 0xa1,
    0xe4, 0xc3, 0x03, 0xd2, 0xa3, 0x18, 0xa7, 0x28,
    0xc3, 0xc0, 0xc9, 0x51, 0x56, 0x80, 0x95, 0x39,
    0xfc, 0xf0, 0xe2, 0x42, 0x9a, 0x6b, 0x52, 0x54,
    0x16, 0xae, 0xdb, 0xf5, 0xa0, 0xde, 0x6a, 0x57,
    0xa6, 0x37, 0xb3, 0x9b
};

static const u8 C6[] = {
    0x8c, 0xe2, 0x49, 0x98, 0x62, 0x56, 0x15, 0xb6,
    0x03, 0xa0, 0x33, 0xac, 0xa1, 0x3f, 0xb8, 0x94,
    0xbe, 0x91, 0x12, 0xa5, 0xc3, 0xa2, 0x11, 0xa8,
    0xba, 0x26, 0x2a, 0x3c, 0xca, 0x7e, 0x2c, 0xa7,
    0x01, 0xe4, 0xa9, 0xa4, 0xfb, 0xa4, 0x3c, 0x90,
    0xcc, 0xdc, 0xb2, 0x81, 0xd4, 0x8c, 0x7c, 0x6f,
    0xd6, 0x28, 0x75, 0xd2, 0xac, 0xa4, 0x17, 0x03,
    0x4c, 0x34, 0xae, 0xe5
};

static const u8 T6[] = {
    0x61, 0x9c, 0xc5, 0xae, 0xff, 0xfe, 0x0b, 0xfa,
    0x46, 0x2a, 0xf4, 0x3c, 0x16, 0x99, 0xd0, 0x50
};

/* Test Case 7 */
static const u8 K7[24], P7[] = { 0 }, A7[] = { 0 }, IV7[12], C7[] = { 0 };
static const u8 T7[] = {
    0xcd, 0x33, 0xb2, 0x8a, 0xc7, 0x73, 0xf7, 0x4b,
    0xa0, 0x0e, 0xd1, 0xf3, 0x12, 0x57, 0x24, 0x35
};

/* Test Case 8 */
# define K8 K7
# define IV8 IV7
# define A8 A7
static const u8 P8[16];
static const u8 C8[] = {
    0x98, 0xe7, 0x24, 0x7c, 0x07, 0xf0, 0xfe, 0x41,
    0x1c, 0x26, 0x7e, 0x43, 0x84, 0xb0, 0xf6, 0x00
};

static const u8 T8[] = {
    0x2f, 0xf5, 0x8d, 0x80, 0x03, 0x39, 0x27, 0xab,
    0x8e, 0xf4, 0xd4, 0x58, 0x75, 0x14, 0xf0, 0xfb
};

/* Test Case 9 */
# define A9 A8
static const u8 K9[] = {
    0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
    0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
    0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c
};

static const u8 P9[] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55
};

static const u8 IV9[] = {
    0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
    0xde, 0xca, 0xf8, 0x88
};

static const u8 C9[] = {
    0x39, 0x80, 0xca, 0x0b, 0x3c, 0x00, 0xe8, 0x41,
    0xeb, 0x06, 0xfa, 0xc4, 0x87, 0x2a, 0x27, 0x57,
    0x85, 0x9e, 0x1c, 0xea, 0xa6, 0xef, 0xd9, 0x84,
    0x62, 0x85, 0x93, 0xb4, 0x0c, 0xa1, 0xe1, 0x9c,
    0x7d, 0x77, 0x3d, 0x00, 0xc1, 0x44, 0xc5, 0x25,
    0xac, 0x61, 0x9d, 0x18, 0xc8, 0x4a, 0x3f, 0x47,
    0x18, 0xe2, 0x44, 0x8b, 0x2f, 0xe3, 0x24, 0xd9,
    0xcc, 0xda, 0x27, 0x10, 0xac, 0xad, 0xe2, 0x56
};

static const u8 T9[] = {
    0x99, 0x24, 0xa7, 0xc8, 0x58, 0x73, 0x36, 0xbf,
    0xb1, 0x18, 0x02, 0x4d, 0xb8, 0x67, 0x4a, 0x14
};

/* Test Case 10 */
# define K10 K9
# define IV10 IV9
static const u8 P10[] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39
};

static const u8 A10[] = {
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xab, 0xad, 0xda, 0xd2
};

static const u8 C10[] = {
    0x39, 0x80, 0xca, 0x0b, 0x3c, 0x00, 0xe8, 0x41,
    0xeb, 0x06, 0xfa, 0xc4, 0x87, 0x2a, 0x27, 0x57,
    0x85, 0x9e, 0x1c, 0xea, 0xa6, 0xef, 0xd9, 0x84,
    0x62, 0x85, 0x93, 0xb4, 0x0c, 0xa1, 0xe1, 0x9c,
    0x7d, 0x77, 0x3d, 0x00, 0xc1, 0x44, 0xc5, 0x25,
    0xac, 0x61, 0x9d, 0x18, 0xc8, 0x4a, 0x3f, 0x47,
    0x18, 0xe2, 0x44, 0x8b, 0x2f, 0xe3, 0x24, 0xd9,
    0xcc, 0xda, 0x27, 0x10
};

static const u8 T10[] = {
    0x25, 0x19, 0x49, 0x8e, 0x80, 0xf1, 0x47, 0x8f,
    0x37, 0xba, 0x55, 0xbd, 0x6d, 0x27, 0x61, 0x8c
};

/* Test Case 11 */
# define K11 K10
# define P11 P10
# define A11 A10
static const u8 IV11[] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad };

static const u8 C11[] = {
    0x0f, 0x10, 0xf5, 0x99, 0xae, 0x14, 0xa1, 0x54,
    0xed, 0x24, 0xb3, 0x6e, 0x25, 0x32, 0x4d, 0xb8,
    0xc5, 0x66, 0x63, 0x2e, 0xf2, 0xbb, 0xb3, 0x4f,
    0x83, 0x47, 0x28, 0x0f, 0xc4, 0x50, 0x70, 0x57,
    0xfd, 0xdc, 0x29, 0xdf, 0x9a, 0x47, 0x1f, 0x75,
    0xc6, 0x65, 0x41, 0xd4, 0xd4, 0xda, 0xd1, 0xc9,
    0xe9, 0x3a, 0x19, 0xa5, 0x8e, 0x8b, 0x47, 0x3f,
    0xa0, 0xf0, 0x62, 0xf7
};

static const u8 T11[] = {
    0x65, 0xdc, 0xc5, 0x7f, 0xcf, 0x62, 0x3a, 0x24,
    0x09, 0x4f, 0xcc, 0xa4, 0x0d, 0x35, 0x33, 0xf8
};

/* Test Case 12 */
# define K12 K11
# define P12 P11
# define A12 A11
static const u8 IV12[] = {
    0x93, 0x13, 0x22, 0x5d, 0xf8, 0x84, 0x06, 0xe5,
    0x55, 0x90, 0x9c, 0x5a, 0xff, 0x52, 0x69, 0xaa,
    0x6a, 0x7a, 0x95, 0x38, 0x53, 0x4f, 0x7d, 0xa1,
    0xe4, 0xc3, 0x03, 0xd2, 0xa3, 0x18, 0xa7, 0x28,
    0xc3, 0xc0, 0xc9, 0x51, 0x56, 0x80, 0x95, 0x39,
    0xfc, 0xf0, 0xe2, 0x42, 0x9a, 0x6b, 0x52, 0x54,
    0x16, 0xae, 0xdb, 0xf5, 0xa0, 0xde, 0x6a, 0x57,
    0xa6, 0x37, 0xb3, 0x9b
};

static const u8 C12[] = {
    0xd2, 0x7e, 0x88, 0x68, 0x1c, 0xe3, 0x24, 0x3c,
    0x48, 0x30, 0x16, 0x5a, 0x8f, 0xdc, 0xf9, 0xff,
    0x1d, 0xe9, 0xa1, 0xd8, 0xe6, 0xb4, 0x47, 0xef,
    0x6e, 0xf7, 0xb7, 0x98, 0x28, 0x66, 0x6e, 0x45,
    0x81, 0xe7, 0x90, 0x12, 0xaf, 0x34, 0xdd, 0xd9,
    0xe2, 0xf0, 0x37, 0x58, 0x9b, 0x29, 0x2d, 0xb3,
    0xe6, 0x7c, 0x03, 0x67, 0x45, 0xfa, 0x22, 0xe7,
    0xe9, 0xb7, 0x37, 0x3b
};

static const u8 T12[] = {
    0xdc, 0xf5, 0x66, 0xff, 0x29, 0x1c, 0x25, 0xbb,
    0xb8, 0x56, 0x8f, 0xc3, 0xd3, 0x76, 0xa6, 0xd9
};

/* Test Case 13 */
static const u8 K13[32], P13[] = { 0 }, A13[] = { 0 }, IV13[12], C13[] = { 0 };
static const u8 T13[] = {
    0x53, 0x0f, 0x8a, 0xfb, 0xc7, 0x45, 0x36, 0xb9,
    0xa9, 0x63, 0xb4, 0xf1, 0xc4, 0xcb, 0x73, 0x8b
};

/* Test Case 14 */
# define K14 K13
# define A14 A13
static const u8 P14[16], IV14[12];
static const u8 C14[] = {
    0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60, 0x6b, 0x6e,
    0x07, 0x4e, 0xc5, 0xd3, 0xba, 0xf3, 0x9d, 0x18
};

static const u8 T14[] = {
    0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99, 0x6b, 0xf0,
    0x26, 0x5b, 0x98, 0xb5, 0xd4, 0x8a, 0xb9, 0x19
};

/* Test Case 15 */
# define A15 A14
static const u8 K15[] = {
    0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
    0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
    0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
    0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
};

static const u8 P15[] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55
};

static const u8 IV15[] = {
    0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
    0xde, 0xca, 0xf8, 0x88
};

static const u8 C15[] = {
    0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07,
    0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d,
    0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9,
    0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa,
    0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
    0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38,
    0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a,
    0xbc, 0xc9, 0xf6, 0x62, 0x89, 0x80, 0x15, 0xad
};

static const u8 T15[] = {
    0xb0, 0x94, 0xda, 0xc5, 0xd9, 0x34, 0x71, 0xbd,
    0xec, 0x1a, 0x50, 0x22, 0x70, 0xe3, 0xcc, 0x6c
};

/* Test Case 16 */
# define K16 K15
# define IV16 IV15
static const u8 P16[] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39
};

static const u8 A16[] = {
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xab, 0xad, 0xda, 0xd2
};

static const u8 C16[] = {
    0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07,
    0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d,
    0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9,
    0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa,
    0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
    0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38,
    0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a,
    0xbc, 0xc9, 0xf6, 0x62
};

static const u8 T16[] = {
    0x76, 0xfc, 0x6e, 0xce, 0x0f, 0x4e, 0x17, 0x68,
    0xcd, 0xdf, 0x88, 0x53, 0xbb, 0x2d, 0x55, 0x1b
};

/* Test Case 17 */
# define K17 K16
# define P17 P16
# define A17 A16
static const u8 IV17[] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad };

static const u8 C17[] = {
    0xc3, 0x76, 0x2d, 0xf1, 0xca, 0x78, 0x7d, 0x32,
    0xae, 0x47, 0xc1, 0x3b, 0xf1, 0x98, 0x44, 0xcb,
    0xaf, 0x1a, 0xe1, 0x4d, 0x0b, 0x97, 0x6a, 0xfa,
    0xc5, 0x2f, 0xf7, 0xd7, 0x9b, 0xba, 0x9d, 0xe0,
    0xfe, 0xb5, 0x82, 0xd3, 0x39, 0x34, 0xa4, 0xf0,
    0x95, 0x4c, 0xc2, 0x36, 0x3b, 0xc7, 0x3f, 0x78,
    0x62, 0xac, 0x43, 0x0e, 0x64, 0xab, 0xe4, 0x99,
    0xf4, 0x7c, 0x9b, 0x1f
};

static const u8 T17[] = {
    0x3a, 0x33, 0x7d, 0xbf, 0x46, 0xa7, 0x92, 0xc4,
    0x5e, 0x45, 0x49, 0x13, 0xfe, 0x2e, 0xa8, 0xf2
};

/* Test Case 18 */
# define K18 K17
# define P18 P17
# define A18 A17
static const u8 IV18[] = {
    0x93, 0x13, 0x22, 0x5d, 0xf8, 0x84, 0x06, 0xe5,
    0x55, 0x90, 0x9c, 0x5a, 0xff, 0x52, 0x69, 0xaa,
    0x6a, 0x7a, 0x95, 0x38, 0x53, 0x4f, 0x7d, 0xa1,
    0xe4, 0xc3, 0x03, 0xd2, 0xa3, 0x18, 0xa7, 0x28,
    0xc3, 0xc0, 0xc9, 0x51, 0x56, 0x80, 0x95, 0x39,
    0xfc, 0xf0, 0xe2, 0x42, 0x9a, 0x6b, 0x52, 0x54,
    0x16, 0xae, 0xdb, 0xf5, 0xa0, 0xde, 0x6a, 0x57,
    0xa6, 0x37, 0xb3, 0x9b
};

static const u8 C18[] = {
    0x5a, 0x8d, 0xef, 0x2f, 0x0c, 0x9e, 0x53, 0xf1,
    0xf7, 0x5d, 0x78, 0x53, 0x65, 0x9e, 0x2a, 0x20,
    0xee, 0xb2, 0xb2, 0x2a, 0xaf, 0xde, 0x64, 0x19,
    0xa0, 0x58, 0xab, 0x4f, 0x6f, 0x74, 0x6b, 0xf4,
    0x0f, 0xc0, 0xc3, 0xb7, 0x80, 0xf2, 0x44, 0x45,
    0x2d, 0xa3, 0xeb, 0xf1, 0xc5, 0xd8, 0x2c, 0xde,
    0xa2, 0x41, 0x89, 0x97, 0x20, 0x0e, 0xf8, 0x2e,
    0x44, 0xae, 0x7e, 0x3f
};

static const u8 T18[] = {
    0xa4, 0x4a, 0x82, 0x66, 0xee, 0x1c, 0x8e, 0xb0,
    0xc8, 0xb5, 0xd4, 0xcf, 0x5a, 0xe9, 0xf1, 0x9a
};

/* Test Case 19 */
# define K19 K1
# define P19 P1
# define IV19 IV1
# define C19 C1
static const u8 A19[] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55,
    0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07,
    0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d,
    0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9,
    0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa,
    0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
    0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38,
    0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a,
    0xbc, 0xc9, 0xf6, 0x62, 0x89, 0x80, 0x15, 0xad
};

static const u8 T19[] = {
    0x5f, 0xea, 0x79, 0x3a, 0x2d, 0x6f, 0x97, 0x4d,
    0x37, 0xe6, 0x8e, 0x0c, 0xb8, 0xff, 0x94, 0x92
};

/* Test Case 20 */
# define K20 K1
# define A20 A1
/* this results in 0xff in counter LSB */
static const u8 IV20[64] = { 0xff, 0xff, 0xff, 0xff };

static const u8 P20[288];
static const u8 C20[] = {
    0x56, 0xb3, 0x37, 0x3c, 0xa9, 0xef, 0x6e, 0x4a,
    0x2b, 0x64, 0xfe, 0x1e, 0x9a, 0x17, 0xb6, 0x14,
    0x25, 0xf1, 0x0d, 0x47, 0xa7, 0x5a, 0x5f, 0xce,
    0x13, 0xef, 0xc6, 0xbc, 0x78, 0x4a, 0xf2, 0x4f,
    0x41, 0x41, 0xbd, 0xd4, 0x8c, 0xf7, 0xc7, 0x70,
    0x88, 0x7a, 0xfd, 0x57, 0x3c, 0xca, 0x54, 0x18,
    0xa9, 0xae, 0xff, 0xcd, 0x7c, 0x5c, 0xed, 0xdf,
    0xc6, 0xa7, 0x83, 0x97, 0xb9, 0xa8, 0x5b, 0x49,
    0x9d, 0xa5, 0x58, 0x25, 0x72, 0x67, 0xca, 0xab,
    0x2a, 0xd0, 0xb2, 0x3c, 0xa4, 0x76, 0xa5, 0x3c,
    0xb1, 0x7f, 0xb4, 0x1c, 0x4b, 0x8b, 0x47, 0x5c,
    0xb4, 0xf3, 0xf7, 0x16, 0x50, 0x94, 0xc2, 0x29,
    0xc9, 0xe8, 0xc4, 0xdc, 0x0a, 0x2a, 0x5f, 0xf1,
    0x90, 0x3e, 0x50, 0x15, 0x11, 0x22, 0x13, 0x76,
    0xa1, 0xcd, 0xb8, 0x36, 0x4c, 0x50, 0x61, 0xa2,
    0x0c, 0xae, 0x74, 0xbc, 0x4a, 0xcd, 0x76, 0xce,
    0xb0, 0xab, 0xc9, 0xfd, 0x32, 0x17, 0xef, 0x9f,
    0x8c, 0x90, 0xbe, 0x40, 0x2d, 0xdf, 0x6d, 0x86,
    0x97, 0xf4, 0xf8, 0x80, 0xdf, 0xf1, 0x5b, 0xfb,
    0x7a, 0x6b, 0x28, 0x24, 0x1e, 0xc8, 0xfe, 0x18,
    0x3c, 0x2d, 0x59, 0xe3, 0xf9, 0xdf, 0xff, 0x65,
    0x3c, 0x71, 0x26, 0xf0, 0xac, 0xb9, 0xe6, 0x42,
    0x11, 0xf4, 0x2b, 0xae, 0x12, 0xaf, 0x46, 0x2b,
    0x10, 0x70, 0xbe, 0xf1, 0xab, 0x5e, 0x36, 0x06,
    0x87, 0x2c, 0xa1, 0x0d, 0xee, 0x15, 0xb3, 0x24,
    0x9b, 0x1a, 0x1b, 0x95, 0x8f, 0x23, 0x13, 0x4c,
    0x4b, 0xcc, 0xb7, 0xd0, 0x32, 0x00, 0xbc, 0xe4,
    0x20, 0xa2, 0xf8, 0xeb, 0x66, 0xdc, 0xf3, 0x64,
    0x4d, 0x14, 0x23, 0xc1, 0xb5, 0x69, 0x90, 0x03,
    0xc1, 0x3e, 0xce, 0xf4, 0xbf, 0x38, 0xa3, 0xb6,
    0x0e, 0xed, 0xc3, 0x40, 0x33, 0xba, 0xc1, 0x90,
    0x27, 0x83, 0xdc, 0x6d, 0x89, 0xe2, 0xe7, 0x74,
    0x18, 0x8a, 0x43, 0x9c, 0x7e, 0xbc, 0xc0, 0x67,
    0x2d, 0xbd, 0xa4, 0xdd, 0xcf, 0xb2, 0x79, 0x46,
    0x13, 0xb0, 0xbe, 0x41, 0x31, 0x5e, 0xf7, 0x78,
    0x70, 0x8a, 0x70, 0xee, 0x7d, 0x75, 0x16, 0x5c
};

static const u8 T20[] = {
    0x8b, 0x30, 0x7f, 0x6b, 0x33, 0x28, 0x6d, 0x0a,
    0xb0, 0x26, 0xa9, 0xed, 0x3f, 0xe1, 0xe8, 0x5f
};

#define GCM128_TEST_VECTOR(n)                   \
    {                                           \
        {sizeof(K##n), K##n},                   \
        {sizeof(IV##n), IV##n},                 \
        {sizeof(A##n), A##n},                   \
        {sizeof(P##n), P##n},                   \
        {sizeof(C##n), C##n},                   \
        {sizeof(T##n), T##n}                    \
    }
static struct gcm128_data {
    const SIZED_DATA K;
    const SIZED_DATA IV;
    const SIZED_DATA A;
    const SIZED_DATA P;
    const SIZED_DATA C;
    const SIZED_DATA T;
} gcm128_vectors[] = {
    GCM128_TEST_VECTOR(1),
    GCM128_TEST_VECTOR(2),
    GCM128_TEST_VECTOR(3),
    GCM128_TEST_VECTOR(4),
    GCM128_TEST_VECTOR(5),
    GCM128_TEST_VECTOR(6),
    GCM128_TEST_VECTOR(7),
    GCM128_TEST_VECTOR(8),
    GCM128_TEST_VECTOR(9),
    GCM128_TEST_VECTOR(10),
    GCM128_TEST_VECTOR(11),
    GCM128_TEST_VECTOR(12),
    GCM128_TEST_VECTOR(13),
    GCM128_TEST_VECTOR(14),
    GCM128_TEST_VECTOR(15),
    GCM128_TEST_VECTOR(16),
    GCM128_TEST_VECTOR(17),
    GCM128_TEST_VECTOR(18),
    GCM128_TEST_VECTOR(19),
    GCM128_TEST_VECTOR(20)
};

static int test_gcm128(int idx)
{
    unsigned char out[512];
    SIZED_DATA K = gcm128_vectors[idx].K;
    SIZED_DATA IV = gcm128_vectors[idx].IV;
    SIZED_DATA A = gcm128_vectors[idx].A;
    SIZED_DATA P = gcm128_vectors[idx].P;
    SIZED_DATA C = gcm128_vectors[idx].C;
    SIZED_DATA T = gcm128_vectors[idx].T;
    GCM128_CONTEXT ctx;
    AES_KEY key;

    /* Size 1 inputs are special-cased to signal NULL. */
    if (A.size == 1)
        A.data = NULL;
    if (P.size == 1)
        P.data = NULL;
    if (C.size == 1)
        C.data = NULL;

    AES_set_encrypt_key(K.data, K.size * 8, &key);

    CRYPTO_gcm128_init(&ctx, &key, (block128_f)AES_encrypt);
    CRYPTO_gcm128_setiv(&ctx, IV.data, IV.size);
    memset(out, 0, P.size);
    if (A.data != NULL)
        CRYPTO_gcm128_aad(&ctx, A.data, A.size);
    if (P.data != NULL)
        CRYPTO_gcm128_encrypt( &ctx, P.data, out, P.size);
    if (!TEST_false(CRYPTO_gcm128_finish(&ctx, T.data, 16))
            || (C.data != NULL
                    && !TEST_mem_eq(out, P.size, C.data, P.size)))
        return 0;

    CRYPTO_gcm128_setiv(&ctx, IV.data, IV.size);
    memset(out, 0, P.size);
    if (A.data != NULL)
        CRYPTO_gcm128_aad(&ctx, A.data, A.size);
    if (C.data != NULL)
        CRYPTO_gcm128_decrypt(&ctx, C.data, out, P.size);
    if (!TEST_false(CRYPTO_gcm128_finish(&ctx, T.data, 16))
            || (P.data != NULL
                    && !TEST_mem_eq(out, P.size, P.data, P.size)))
        return 0;

    return 1;
}

static void benchmark_gcm128(const unsigned char *K, size_t Klen,
                             const unsigned char *IV, size_t IVlen)
{
#ifdef OPENSSL_CPUID_OBJ
    GCM128_CONTEXT ctx;
    AES_KEY key;
    size_t start, gcm_t, ctr_t, OPENSSL_rdtsc();
    union {
        u64 u;
        u8 c[1024];
    } buf;

    AES_set_encrypt_key(K, Klen * 8, &key);
    CRYPTO_gcm128_init(&ctx, &key, (block128_f) AES_encrypt);
    CRYPTO_gcm128_setiv(&ctx, IV, IVlen);

    CRYPTO_gcm128_encrypt(&ctx, buf.c, buf.c, sizeof(buf));
    start = OPENSSL_rdtsc();
    CRYPTO_gcm128_encrypt(&ctx, buf.c, buf.c, sizeof(buf));
    gcm_t = OPENSSL_rdtsc() - start;

    CRYPTO_ctr128_encrypt(buf.c, buf.c, sizeof(buf),
                          &key, ctx.Yi.c, ctx.EKi.c, &ctx.mres,
                          (block128_f) AES_encrypt);
    start = OPENSSL_rdtsc();
    CRYPTO_ctr128_encrypt(buf.c, buf.c, sizeof(buf),
                          &key, ctx.Yi.c, ctx.EKi.c, &ctx.mres,
                          (block128_f) AES_encrypt);
    ctr_t = OPENSSL_rdtsc() - start;

    printf("%.2f-%.2f=%.2f\n",
           gcm_t / (double)sizeof(buf),
           ctr_t / (double)sizeof(buf),
           (gcm_t - ctr_t) / (double)sizeof(buf));
# ifdef GHASH
    {
        void (*gcm_ghash_p) (u64 Xi[2], const u128 Htable[16],
                             const u8 *inp, size_t len) = ctx.ghash;

        GHASH((&ctx), buf.c, sizeof(buf));
        start = OPENSSL_rdtsc();
        for (i = 0; i < 100; ++i)
            GHASH((&ctx), buf.c, sizeof(buf));
        gcm_t = OPENSSL_rdtsc() - start;
        printf("%.2f\n", gcm_t / (double)sizeof(buf) / (double)i);
    }
# endif
#else
    fprintf(stderr,
            "Benchmarking of modes isn't available on this platform\n");
#endif
}

int setup_tests(void)
{
    if (test_has_option("-h")) {
        printf("-h\tThis help\n");
        printf("-b\tBenchmark gcm128 in addition to the tests\n");
        return 1;
    }

    ADD_ALL_TESTS(test_cts128, OSSL_NELEM(cts128_vectors));
    ADD_ALL_TESTS(test_cts128_nist, OSSL_NELEM(cts128_vectors));
    ADD_ALL_TESTS(test_gcm128, OSSL_NELEM(gcm128_vectors));
    return 1;
}

void cleanup_tests(void)
{
    if (test_has_option("-b"))
        benchmark_gcm128(K1, sizeof(K1), IV1, sizeof(IV1));
}
