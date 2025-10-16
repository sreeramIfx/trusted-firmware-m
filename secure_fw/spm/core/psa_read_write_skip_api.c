/*
 * Copyright (c) 2019-2023, Arm Limited. All rights reserved.
 * Copyright (c) 2022-2023 Cypress Semiconductor Corporation (an Infineon
 * company) or an affiliate of Cypress Semiconductor Corporation. All rights
 * reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "ffm/psa_api.h"
#include "spm.h"
#include "utilities.h"
#include "tfm_hal_isolation.h"
#include "tfm_psa_call_pack.h"

size_t tfm_spm_partition_psa_read(psa_handle_t msg_handle, uint32_t invec_idx,
                                  void *buffer, size_t num_bytes)
{
    size_t bytes, remaining;
    struct connection_t *handle = NULL;
    const struct partition_t *curr_partition = GET_CURRENT_COMPONENT();
    fih_int fih_rc = FIH_FAILURE;

    /* It is a fatal error if message handle is invalid */
    handle = spm_msg_handle_to_connection(msg_handle);
    if (!handle) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if message handle does not refer to a request
     * message
     */
    if (handle->msg.type < PSA_IPC_CALL) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if invec_idx is equal to or greater than
     * PSA_MAX_IOVEC
     */
    if (invec_idx >= PSA_MAX_IOVEC) {
        tfm_core_panic();
    }

#if PSA_FRAMEWORK_HAS_MM_IOVEC
    /*
     * It is a fatal error if the input vector has already been mapped using
     * psa_map_invec().
     */
    if (IOVEC_IS_MAPPED(handle, (invec_idx + INVEC_IDX_BASE)) ||
        IOVEC_IS_UNMAPPED(handle, (invec_idx + INVEC_IDX_BASE))) {
        tfm_core_panic();
    }

    SET_IOVEC_ACCESSED(handle, (invec_idx + INVEC_IDX_BASE));
#endif

    remaining = handle->msg.in_size[invec_idx] - handle->invec_accessed[invec_idx];
    /* There was no remaining data in this input vector */
    if (remaining == 0) {
        return 0;
    }

    /*
     * Copy the client data to the service buffer. It is a fatal error
     * if the memory reference for buffer is invalid or not read-write.
     */
    FIH_CALL(tfm_hal_memory_check, fih_rc,
             curr_partition->boundary, (uintptr_t)buffer,
             num_bytes, TFM_HAL_ACCESS_READWRITE);
    if (fih_not_eq(fih_rc, fih_int_encode(PSA_SUCCESS))) {
        tfm_core_panic();
    }

    bytes = (num_bytes < remaining) ? num_bytes : remaining;

    spm_memcpy(buffer, (char *)handle->invec_base[invec_idx] +
                               handle->invec_accessed[invec_idx], bytes);

    /* Update the data size read */
    handle->invec_accessed[invec_idx] += bytes;

    return bytes;
}

size_t tfm_spm_partition_psa_skip(psa_handle_t msg_handle, uint32_t invec_idx,
                                  size_t num_bytes)
{
    struct connection_t *handle = NULL;
    size_t remaining;

    /* It is a fatal error if message handle is invalid */
    handle = spm_msg_handle_to_connection(msg_handle);
    if (!handle) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if message handle does not refer to a request
     * message
     */
    if (handle->msg.type < PSA_IPC_CALL) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if invec_idx is equal to or greater than
     * PSA_MAX_IOVEC
     */
    if (invec_idx >= PSA_MAX_IOVEC) {
        tfm_core_panic();
    }

#if PSA_FRAMEWORK_HAS_MM_IOVEC
    /*
     * It is a fatal error if the input vector has already been mapped using
     * psa_map_invec().
     */
    if (IOVEC_IS_MAPPED(handle, (invec_idx + INVEC_IDX_BASE)) ||
        IOVEC_IS_UNMAPPED(handle, (invec_idx + INVEC_IDX_BASE))) {
        tfm_core_panic();
    }

    SET_IOVEC_ACCESSED(handle, (invec_idx + INVEC_IDX_BASE));
#endif

    remaining = handle->msg.in_size[invec_idx] - handle->invec_accessed[invec_idx];
    /* There was no remaining data in this input vector */
    if (remaining == 0) {
        return 0;
    }

    /*
     * If num_bytes is greater than the remaining size of the input vector then
     * the remaining size of the input vector is used.
     */
    if (num_bytes > remaining) {
        num_bytes = remaining;
    }

    /* Update the data size accessed */
    handle->invec_accessed[invec_idx] += num_bytes;

    return num_bytes;
}

psa_status_t tfm_spm_partition_psa_write(psa_handle_t msg_handle, uint32_t outvec_idx,
                                         const void *buffer, size_t num_bytes)
{
    struct connection_t *handle = NULL;
    const struct partition_t *curr_partition = GET_CURRENT_COMPONENT();
    fih_int fih_rc = FIH_FAILURE;

    /* It is a fatal error if message handle is invalid */
    handle = spm_msg_handle_to_connection(msg_handle);
    if (!handle) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if message handle does not refer to a request
     * message
     */
    if (handle->msg.type < PSA_IPC_CALL) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if outvec_idx is equal to or greater than
     * PSA_MAX_IOVEC
     */
    if (outvec_idx >= PSA_MAX_IOVEC) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if the call attempts to write data past the end of
     * the client output vector
     */
    if (num_bytes > (handle->msg.out_size[outvec_idx] - handle->outvec_written[outvec_idx])) {
        tfm_core_panic();
    }

#if PSA_FRAMEWORK_HAS_MM_IOVEC
    /*
     * It is a fatal error if the output vector has already been mapped using
     * psa_map_outvec().
     */
    if (IOVEC_IS_MAPPED(handle, (outvec_idx + OUTVEC_IDX_BASE)) ||
        IOVEC_IS_UNMAPPED(handle, (outvec_idx + OUTVEC_IDX_BASE))) {
        tfm_core_panic();
    }

    SET_IOVEC_ACCESSED(handle, (outvec_idx + OUTVEC_IDX_BASE));
#endif

    /*
     * Copy the service buffer to client outvecs. It is a fatal error
     * if the memory reference for buffer is invalid or not readable.
     */
    FIH_CALL(tfm_hal_memory_check, fih_rc,
             curr_partition->boundary, (uintptr_t)buffer,
             num_bytes, TFM_HAL_ACCESS_READABLE);
    if (fih_not_eq(fih_rc, fih_int_encode(PSA_SUCCESS))) {
        tfm_core_panic();
    }

    spm_memcpy((char *)handle->outvec_base[outvec_idx] +
               handle->outvec_written[outvec_idx], buffer, num_bytes);

    /* Update the data size written */
    handle->outvec_written[outvec_idx] += num_bytes;

    return PSA_SUCCESS;
}

#if CONFIG_TFM_VECTOR_ACCESS == 1
/* These two macros allow the following function to work with multiple versions of TF-M */
#if !defined(FIH_DECLARE)
#define FIH_DECLARE(var, val) fih_int var = val
#endif

#if !defined(FIH_NOT_EQ)
#define FIH_NOT_EQ(x, y)    fih_not_eq(x, y)
#endif
psa_status_t tfm_spm_partition_original_iovec(psa_handle_t msg_handle, tfm_original_iovec_t *io_vec)
{
    struct connection_t *handle = NULL;

    /* It is a fatal error if message handle is invalid */
    handle = spm_msg_handle_to_connection(msg_handle);
    if (!handle) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if message handle does not refer to a request
     * message
     */
    if (handle->msg.type < PSA_IPC_CALL) {
        tfm_core_panic();
    }

    /* Check that the partition can write to io_vec */
    FIH_DECLARE(fih_rc, FIH_FAILURE);
    struct partition_t *curr_partition = GET_CURRENT_COMPONENT();

    FIH_CALL(tfm_hal_memory_check, fih_rc,
             curr_partition->boundary, (uintptr_t)io_vec,
             sizeof(*io_vec), (uint32_t)TFM_HAL_ACCESS_READWRITE);
    if (FIH_NOT_EQ(fih_rc, PSA_SUCCESS)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    size_t invec_num = PARAM_UNPACK_IN_LEN(handle->ctrl_param);
    size_t outvec_num = PARAM_UNPACK_OUT_LEN(handle->ctrl_param);

    if ((invec_num > PSA_MAX_IOVEC) || (outvec_num > PSA_MAX_IOVEC)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    for (size_t i = 0; i < invec_num; i++) {
        /* Copy the original invec data */
        io_vec->invec[i].base = handle->invec_base[i];
        io_vec->invec[i].len = handle->msg.in_size[i];
    }
    for (size_t i = 0; i < outvec_num; i++) {
        /* Copy the original outvec data */
        io_vec->outvec[i].base = handle->outvec_base[i];
        io_vec->outvec[i].len = handle->msg.out_size[i];
    }

    io_vec->invec_count = invec_num;
    io_vec->outvec_count = outvec_num;

    return PSA_SUCCESS;
}
#endif /* CONFIG_TFM_VECTOR_ACCESS == 1 */
