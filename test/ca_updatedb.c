/*
 * Copyright 2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifdef OPENSSL_SYS_UNIX
# include <unistd.h>
#endif
#include "../apps/include/ca.h"
#include "../apps/include/ca_logic.h"
#include "../apps/include/apps.h"

/* tweaks needed for Windows */
#ifdef _WIN32
# define access _access
# define timezone _timezone
#endif

char *default_config_file = NULL;

int main(int argc, char *argv[])
{
    CA_DB *db = NULL;
    BIO *channel;
    ASN1_TIME *tmps = NULL;
    char *p;
    struct tm *testdate_tm = NULL;
    time_t *testdatelocal = NULL;
    time_t *testdateutc = NULL;
    size_t len;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s indexfile testdate\n", argv[0]);
        fprintf(stderr, "       testdate format: YYYY-MM-DDThh:mm\n");
	exit(EXIT_FAILURE);
    }
    if (access(argv[1], F_OK) != 0) {
        fprintf(stderr, "Error: dbfile '%s' is not readable\n", argv[1]);
	exit(EXIT_FAILURE);
    }

    len = strlen(argv[2])+1;
    tmps = ASN1_STRING_new();
    if (!ASN1_STRING_set(tmps, NULL, len))
    {
        ASN1_STRING_free(tmps);
        exit(EXIT_FAILURE);
    }

    testdatelocal = app_malloc(sizeof(time_t), "testdatelocal");
    testdate_tm = app_malloc(sizeof(struct tm), "testdate_tm");

    if (strlen(argv[2]) == 13)
    	tmps->type = V_ASN1_UTCTIME;
    else
        tmps->type = V_ASN1_GENERALIZEDTIME;
    p = (char*)tmps->data;
    tmps->length = BIO_snprintf(p, len, argv[2]);

    if (!(ASN1_TIME_to_tm(tmps, testdate_tm))) {
        free(testdatelocal);
        free(testdate_tm);
        ASN1_STRING_free(tmps);
        fprintf(stderr, "Error: testdate '%s' is invalid\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    *testdatelocal = mktime(testdate_tm);
    testdateutc = app_malloc(sizeof(time_t), "testdateutc");
    *testdateutc = *testdatelocal - timezone;
    free(testdatelocal);

    channel = BIO_push(BIO_new(BIO_f_prefix()), dup_bio_err(FORMAT_TEXT));
    bio_err = dup_bio_err(FORMAT_TEXT);

    default_config_file = CONF_get1_default_config_file();
    if (default_config_file == NULL) {
        ASN1_STRING_free(tmps);
        BIO_free_all(bio_err);
        BIO_free_all(channel);
        free(testdate_tm);
        free(testdateutc);
        fprintf(stderr, "Error: could not get default config file\n");
        exit(EXIT_FAILURE);
    }

    db = load_index(argv[1], NULL);

    do_updatedb(db, testdateutc);

    ASN1_STRING_free(tmps);
    free(default_config_file);
    free_index(db);
    free(testdate_tm);
    free(testdateutc);
    BIO_free_all(bio_err);
    BIO_free_all(channel);
    exit(EXIT_SUCCESS);
}
