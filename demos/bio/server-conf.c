/*
 * Copyright 2013-2017 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

/*
 * A minimal program to serve an tls connection. It uses blocking. It uses
 * the tls_CONF API with a configuration file. cc -I../../include saccept.c
 * -L../.. -ltls -lcrypto -ldl
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <opentls/err.h>
#include <opentls/tls.h>
#include <opentls/conf.h>

int main(int argc, char *argv[])
{
    char *port = "*:4433";
    BIO *in = NULL;
    BIO *tls_bio, *tmp;
    tls_CTX *ctx;
    tls_CONF_CTX *cctx = NULL;
    CONF *conf = NULL;
    STACK_OF(CONF_VALUE) *sect = NULL;
    CONF_VALUE *cnf;
    long errline = -1;
    char buf[512];
    int ret = EXIT_FAILURE, i;

    ctx = tls_CTX_new(TLS_server_method());

    conf = NCONF_new(NULL);

    if (NCONF_load(conf, "accept.cnf", &errline) <= 0) {
        if (errline <= 0)
            fprintf(stderr, "Error processing config file\n");
        else
            fprintf(stderr, "Error on line %ld\n", errline);
        goto err;
    }

    sect = NCONF_get_section(conf, "default");

    if (sect == NULL) {
        fprintf(stderr, "Error retrieving default section\n");
        goto err;
    }

    cctx = tls_CONF_CTX_new();
    tls_CONF_CTX_set_flags(cctx, tls_CONF_FLAG_SERVER);
    tls_CONF_CTX_set_flags(cctx, tls_CONF_FLAG_CERTIFICATE);
    tls_CONF_CTX_set_flags(cctx, tls_CONF_FLAG_FILE);
    tls_CONF_CTX_set_tls_ctx(cctx, ctx);
    for (i = 0; i < sk_CONF_VALUE_num(sect); i++) {
        int rv;
        cnf = sk_CONF_VALUE_value(sect, i);
        rv = tls_CONF_cmd(cctx, cnf->name, cnf->value);
        if (rv > 0)
            continue;
        if (rv != -2) {
            fprintf(stderr, "Error processing %s = %s\n",
                    cnf->name, cnf->value);
            ERR_print_errors_fp(stderr);
            goto err;
        }
        if (strcmp(cnf->name, "Port") == 0) {
            port = cnf->value;
        } else {
            fprintf(stderr, "Unknown configuration option %s\n", cnf->name);
            goto err;
        }
    }

    if (!tls_CONF_CTX_finish(cctx)) {
        fprintf(stderr, "Finish error\n");
        ERR_print_errors_fp(stderr);
        goto err;
    }

    /* Setup server side tls bio */
    tls_bio = BIO_new_tls(ctx, 0);

    if ((in = BIO_new_accept(port)) == NULL)
        goto err;

    /*
     * This means that when a new connection is accepted on 'in', The tls_bio
     * will be 'duplicated' and have the new socket BIO push into it.
     * Basically it means the tls BIO will be automatically setup
     */
    BIO_set_accept_bios(in, tls_bio);

 again:
    /*
     * The first call will setup the accept socket, and the second will get a
     * socket.  In this loop, the first actual accept will occur in the
     * BIO_read() function.
     */

    if (BIO_do_accept(in) <= 0)
        goto err;

    for (;;) {
        i = BIO_read(in, buf, 512);
        if (i == 0) {
            /*
             * If we have finished, remove the underlying BIO stack so the
             * next time we call any function for this BIO, it will attempt
             * to do an accept
             */
            printf("Done\n");
            tmp = BIO_pop(in);
            BIO_free_all(tmp);
            goto again;
        }
        if (i < 0) {
            if (BIO_should_retry(in))
                continue;
            goto err;
        }
        fwrite(buf, 1, i, stdout);
        fflush(stdout);
    }

    ret = EXIT_SUCCESS;
 err:
    if (ret != EXIT_SUCCESS)
        ERR_print_errors_fp(stderr);
    BIO_free(in);
    return ret;
}
