/*
 * Copyright 2004-2016 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#define APPMACROS_ONLY
#include "applink.c"

extern void *OPENtls_UplinkTable[];

#define UP_stdin  (*(void *(*)(void))OPENtls_UplinkTable[APPLINK_STDIN])()
#define UP_stdout (*(void *(*)(void))OPENtls_UplinkTable[APPLINK_STDOUT])()
#define UP_stderr (*(void *(*)(void))OPENtls_UplinkTable[APPLINK_STDERR])()
#define UP_fprintf (*(int (*)(void *,const char *,...))OPENtls_UplinkTable[APPLINK_FPRINTF])
#define UP_fgets  (*(char *(*)(char *,int,void *))OPENtls_UplinkTable[APPLINK_FGETS])
#define UP_fread  (*(size_t (*)(void *,size_t,size_t,void *))OPENtls_UplinkTable[APPLINK_FREAD])
#define UP_fwrite (*(size_t (*)(const void *,size_t,size_t,void *))OPENtls_UplinkTable[APPLINK_FWRITE])
#define UP_fsetmod (*(int (*)(void *,char))OPENtls_UplinkTable[APPLINK_FSETMOD])
#define UP_feof   (*(int (*)(void *))OPENtls_UplinkTable[APPLINK_FEOF])
#define UP_fclose (*(int (*)(void *))OPENtls_UplinkTable[APPLINK_FCLOSE])

#define UP_fopen  (*(void *(*)(const char *,const char *))OPENtls_UplinkTable[APPLINK_FOPEN])
#define UP_fseek  (*(int (*)(void *,long,int))OPENtls_UplinkTable[APPLINK_FSEEK])
#define UP_ftell  (*(long (*)(void *))OPENtls_UplinkTable[APPLINK_FTELL])
#define UP_fflush (*(int (*)(void *))OPENtls_UplinkTable[APPLINK_FFLUSH])
#define UP_ferror (*(int (*)(void *))OPENtls_UplinkTable[APPLINK_FERROR])
#define UP_clearerr (*(void (*)(void *))OPENtls_UplinkTable[APPLINK_CLEARERR])
#define UP_fileno (*(int (*)(void *))OPENtls_UplinkTable[APPLINK_FILENO])

#define UP_open   (*(int (*)(const char *,int,...))OPENtls_UplinkTable[APPLINK_OPEN])
#define UP_read   (*(otls_ssize_t (*)(int,void *,size_t))OPENtls_UplinkTable[APPLINK_READ])
#define UP_write  (*(otls_ssize_t (*)(int,const void *,size_t))OPENtls_UplinkTable[APPLINK_WRITE])
#define UP_lseek  (*(long (*)(int,long,int))OPENtls_UplinkTable[APPLINK_LSEEK])
#define UP_close  (*(int (*)(int))OPENtls_UplinkTable[APPLINK_CLOSE])
