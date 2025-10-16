/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company)
 * or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef ORIGINAL_VECTOR_API_H
#define ORIGINAL_VECTOR_API_H

#include <stdint.h>

#include "config_impl.h"
#include "psa/client.h"

/**
 * Structure to hold original input and output vectors and their counts
 */
typedef struct tfm_original_iovec_t {
    psa_invec invec[PSA_MAX_IOVEC];
    psa_outvec outvec[PSA_MAX_IOVEC];
    size_t invec_count;
    size_t outvec_count;
} tfm_original_iovec_t;

/**
 * \brief Retrieve original invec and outvec as passed to psa_call()
 *
 * \param[in]  msg_handle   Handle for the client's message.
 * \param[out] io_vec       Pointer to tfm_original_iovec_t to be filled.
 *
 * \retval PSA_SUCCESS      Success.
 */
psa_status_t original_iovec(psa_handle_t msg_handle, tfm_original_iovec_t *io_vec);

#endif /* ORIGINAL_VECTOR_API_H */
