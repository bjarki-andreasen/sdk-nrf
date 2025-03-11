/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_trace.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf_trace, CONFIG_NRF_TRACE_LOG_LEVEL);

#define CALLS_SIZE CONFIG_NRF_TRACE_MAX_CALLS

static void *calls[CALLS_SIZE];
static volatile uint32_t calls_len = CALLS_SIZE;
static volatile uint32_t recorded_len;
static K_SEM_DEFINE(semlock, 1, 1);

static __no_instrument_function void sort_ascending(uint32_t len)
{
	void *swap;

	if (len < 2) {
		return;
	}

	while (true) {
		swap = NULL;

		for (size_t i = 1; i < len; i++) {
			if (calls[i-1] < calls[i]) {
				swap = calls[i-1];
				calls[i-1] = calls[i];
				calls[i] = swap;
			}
		}

		if (swap == NULL) {
			break;
		}
	}
}

static __no_instrument_function uint32_t remove_duplicates(uint32_t len)
{
	size_t unique_len;

	if (len < 2) {
		return len;
	}

	unique_len = 0;

	for (size_t i = 1; i < len; i++) {
		if (calls[unique_len] != calls[i]) {
			unique_len++;
			calls[unique_len] = calls[i];
		}
	}

	return unique_len;
}

__no_instrument_function void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	calls_len++;

	if (likely(calls_len > CALLS_SIZE)) {
		return;
	}

	calls[calls_len - 1] = this_fn;
}

__no_instrument_function void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
	ARG_UNUSED(this_fn);
	ARG_UNUSED(call_site);
}

int nrf_trace_start(void)
{
	return calls_len = 0;
}

int nrf_trace_stop(void)
{
	uint32_t len;

	if (recorded_len) {
		return -EALREADY;
	}

	len = calls_len;

	if (len > CALLS_SIZE) {
		return -ENOMEM;
	}

	recorded_len = len;
	return 0;
}

int nrf_trace_dump_unique(void)
{
	uint32_t len;
	int ret = 0;

	(void)k_sem_take(&semlock, K_FOREVER);

	len = recorded_len;

	if (len == 0) {
		ret = -ENODATA;
		goto unlock_exit;
	}

	sort_ascending(len);

	len = remove_duplicates(len);

	for (size_t i = 0; i < len; i++) {
#if CONFIG_NRF_TRACE_BACKEND_LOG
		LOG_INF("call: %08x", (uint32_t)calls[i]);
#elif CONFIG_NRF_TRACE_BACKEND_CONSOLE
		printk("call: %08x\n", (uint32_t)calls[i]);
#endif
	}

	recorded_len = 0;
	ret = 0;

unlock_exit:
	(void)k_sem_give(&semlock);
	return ret;
}
