#include <stdbool.h>

#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_memory.h>

#include "cleanflow.h"
#include "log_ratelimit.h"

struct log_ratelimit_state {
	uint64_t interval_cycles;
	uint32_t burst;
	uint32_t printed;
	uint32_t suppressed;
	uint64_t end;
} __rte_cache_aligned;

static struct log_ratelimit_state log_ratelimit_states[RTE_MAX_LCORE];

static bool enabled;

void
log_ratelimit_enable(void)
{
	enabled = true;
}

static inline void
log_ratelimit_reset(struct log_ratelimit_state *lrs, uint64_t now)
{
	lrs->printed = 0;
	if (lrs->suppressed > 0) {
		rte_log(RTE_LOG_NOTICE, cleanflow_logtype,
			"CLEANFLOW: %u log entries were suppressed at lcore %u during the last ratelimit interval\n",
			lrs->suppressed, rte_lcore_id());
	}
	lrs->suppressed = 0;
	lrs->end = now + lrs->interval_cycles;
}

void
log_ratelimit_state_init(unsigned lcore_id, uint32_t interval, uint32_t burst)
{
	struct log_ratelimit_state *lrs;

	RTE_VERIFY(lcore_id < RTE_MAX_LCORE);

	lrs = &log_ratelimit_states[lcore_id];
	lrs->interval_cycles = interval * cycles_per_ms;
	lrs->burst = burst;
	lrs->suppressed = 0;
	log_ratelimit_reset(lrs, rte_rdtsc());
}

/*
 * Rate limiting log entries.
 *
 * Returns:
 * - true means go ahead and do it.
 * - false means callbacks will be suppressed.
 */
static bool
log_ratelimit_allow(struct log_ratelimit_state *lrs)
{
	uint64_t now;

	/* unlikely() reason: @enabled is only false during startup. */
	if (unlikely(!enabled))
		return true;

	/* unlikely() reason: all logs are rate-limited in production. */
	if (unlikely(lrs->interval_cycles == 0))
		return true;

	now = rte_rdtsc();

	/*
	 * unlikely() reason: there is only one
	 * reset every @lrs->interval_cycles.
	 */
	if (unlikely(lrs->end < now))
		log_ratelimit_reset(lrs, now);

	if (lrs->burst > lrs->printed) {
		lrs->printed++;
		return true;
	}

	lrs->suppressed++;

	return false;
}

int
rte_log_ratelimit(uint32_t level, uint32_t logtype, const char *format, ...)
{
	struct log_ratelimit_state *lrs;
	int ratelimit_level = rte_log_get_level(logtype);
	if (unlikely(ratelimit_level < 0))
		return -1;

	lrs = &log_ratelimit_states[rte_lcore_id()];
	if (level <= (typeof(level))ratelimit_level &&
			log_ratelimit_allow(lrs)) {
		int ret;
		va_list ap;

		va_start(ap, format);
		ret = rte_vlog(level, logtype, format, ap);
		va_end(ap);

		return ret;
	}

	return 0;
}