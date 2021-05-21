#include <argp.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <rte_branch_prediction.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_log.h>
#include <rte_timer.h>

#include "cleanflow.h"
#include "lib/log_ratelimit.h"

uint64_t cycles_per_sec;
uint64_t cycles_per_ms;
uint64_t picosec_per_cycle;
int cleanflow_logtype;

volatile int exiting = false;

static void signal_handler(int signum) {
  if (signum == SIGINT)
    fprintf(stderr, "caught sigint\n");
  else if (signum == SIGTERM)
    fprintf(stderr, "caught sigterm\n");
  else
    fprintf(stderr, "caught unknown signal (%d)\n", signum);
  exiting = true;
}

static int signal_process(void) {
  int ret = -1;
  sig_t pipe_handler;
  struct sigaction new_action;
  struct sigaction old_int_action;
  struct sigaction old_term_action;

  new_action.sa_handler = signal_handler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;

  ret = sigaction(SIGINT, &new_action, &old_int_action);
  if (ret < 0)
    goto out;

  ret = sigaction(SIGTERM, &new_action, &old_term_action);
  if (ret < 0)
    goto int_action;

  pipe_handler = signal(SIGPIPE, SIG_IGN);
  if (pipe_handler == SIG_ERR) {
    fprintf(stderr, "error: failed to ignore sigpipe - %s\n", strerror(errno));
    goto term_action;
  }

  goto out;

term_action:
  sigaction(SIGTERM, &old_term_action, NULL);
int_action:
  sigaction(SIGINT, &old_int_action, NULL);
out:
  return ret;
}

static int time_resolution_init(void) {
  int ret;
  uint64_t diff_ns;
  uint64_t cycles;
  uint64_t tsc_start;
  struct timespec tp_start;

  tsc_start = rte_rdtsc();
  ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp_start);
  if (ret < 0)
    return ret;

  while (1) {
    uint64_t tsc_now;
    struct timespec tp_now;

    ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp_now);
    tsc_now = rte_rdtsc();
    if (ret < 0)
      return ret;

    diff_ns = (uint64_t)(tp_now.tv_sec - tp_start.tv_sec) * 1000000000ul +
              (uint64_t)(tp_now.tv_nsec - tp_start.tv_nsec);

    if (diff_ns >= 1000000000ul) {
      cycles = tsc_now - tsc_start;
      break;
    }
  }

  cycles_per_sec = cycles * 1000000000ul / diff_ns;
  cycles_per_ms = cycles_per_sec / 1000ul;
  picosec_per_cycle = 1000ul * diff_ns / cycles;

  G_LOG(NOTICE,
        "main: cycles/second = %" PRIu64 ", cycles/millisecond = %" PRIu64
        ", picosec/cycle = %" PRIu64 "\n",
        cycles_per_sec, cycles_per_ms, picosec_per_cycle);

  return 0;
}

int main(int argc, char **argv) {

  int ret;

  rte_log_register("cleanflow");

  RTE_VERIFY(prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == 0);

  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "error: rte_eal_init failed.");
  }
  argc -= ret;
  argv += ret;

  rte_timer_subsystem_init();

  ret = signal_process();
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "error: signal_process failed.");
    return ret;
  }

  ret = time_resolution_init();
  if (ret < 0) {
    return ret;
  }

  while (likely(!exiting)) {
  }

  rte_eal_mp_wait_lcore();

  return ret;
}
