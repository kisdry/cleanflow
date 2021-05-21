#if !defined(CLEANFLOW_H)
#define CLEANFLOW_H

extern int cleanflow_logtype;

extern uint64_t cycles_per_sec;
extern uint64_t cycles_per_ms;
extern uint64_t picosec_per_cycle;

#define G_LOG(level, ...)		        \
	rte_log_ratelimit(RTE_LOG_ ## level,	\
	cleanflow_logtype, "CLEANFLOW: " __VA_ARGS__)

#endif // CLEANFLOW_H
