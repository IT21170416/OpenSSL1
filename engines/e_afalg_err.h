/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The Opentls Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.opentls.org/source/license.html
 */

#ifndef Otls_ENGINES_E_AFALG_ERR_H
# define Otls_ENGINES_E_AFALG_ERR_H

# include <opentls/opentlsconf.h>
# include <opentls/symhacks.h>


# define AFALGerr(f, r) ERR_AFALG_error(0, (r), OPENtls_FILE, OPENtls_LINE)


/*
 * AFALG function codes.
 */
# ifndef OPENtls_NO_DEPRECATED_3_0
#  define AFALG_F_AFALG_CHK_PLATFORM                       0
#  define AFALG_F_AFALG_CREATE_SK                          0
#  define AFALG_F_AFALG_INIT_AIO                           0
#  define AFALG_F_AFALG_SETUP_ASYNC_EVENT_NOTIFICATION     0
#  define AFALG_F_AFALG_SET_KEY                            0
#  define AFALG_F_BIND_AFALG                               0
# endif

/*
 * AFALG reason codes.
 */
# define AFALG_R_EVENTFD_FAILED                           108
# define AFALG_R_FAILED_TO_GET_PLATFORM_INFO              111
# define AFALG_R_INIT_FAILED                              100
# define AFALG_R_IO_SETUP_FAILED                          105
# define AFALG_R_KERNEL_DOES_NOT_SUPPORT_AFALG            101
# define AFALG_R_KERNEL_DOES_NOT_SUPPORT_ASYNC_AFALG      107
# define AFALG_R_MEM_ALLOC_FAILED                         102
# define AFALG_R_SOCKET_ACCEPT_FAILED                     110
# define AFALG_R_SOCKET_BIND_FAILED                       103
# define AFALG_R_SOCKET_CREATE_FAILED                     109
# define AFALG_R_SOCKET_OPERATION_FAILED                  104
# define AFALG_R_SOCKET_SET_KEY_FAILED                    106

#endif
