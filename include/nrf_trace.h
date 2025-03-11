/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_TRACE_H__
#define NRF_TRACE_H__

#include <zephyr/kernel.h>

/**
 * @brief Start nRF trace
 *
 * @details nRF trace will record all functions called, excluding
 * the nrf_trace APIs themselves, until manually stopped or the
 * internal call buffer running out.
 *
 * @note Safe to call from any context
 *
 * @retval 0 Successful
 * @retval -errno code Failure
 */
__no_instrument_function int nrf_trace_start(void);

/**
 * @brief Stop nRF trace
 *
 * @note Safe to call from any context
 *
 * @retval 0 Successful
 * @retval -EALREADY nRF trace was already stopped
 */
__no_instrument_function int nrf_trace_stop(void);

/**
 * @brief Dump unique nRF trace calls
 *
 * @details Dumps unique recorded nRF trace calls in unspecified order.
 *
 * @note Should be called from thread context as operation
 * is slow.
 *
 * @retval 0 Successful
 * @retval -ENODATA if nRF trace has no calls recorded
 * @retval -EBUSY if nRF trace is started
 */
__no_instrument_function int nrf_trace_dump_unique(void);

#endif /* NRF_TRACE_H__ */
