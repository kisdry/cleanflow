#include <argp.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/resource.h>
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

static int signal_proc(void) {
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

int main_loop(void *arg) {
  unsigned lcore = rte_lcore_id();
  while (exiting == false) {
    printf("hello,wolrd - %d\n", lcore);
  }

  return 0;
}

void corefile_init(void) {
  struct rlimit rlim;
  struct rlimit rlim_new;

  if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
    rlim_new.rlim_cur = RLIM_INFINITY;
    rlim_new.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &rlim_new) != 0) {
      rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
      (void)setrlimit(RLIMIT_CORE, &rlim_new);
    }
  }
  return;
}

int main(int argc, char **argv) {

  int ret;

  corefile_init();

  RTE_VERIFY(prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == 0);

  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "error: rte_eal_init failed.");
  }
  argc -= ret;
  argv += ret;

  rte_timer_subsystem_init();

  ret = signal_proc();
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "error: signal_proc failed.");
    return ret;
  }

  rte_eal_mp_remote_launch(main_loop, NULL, CALL_MAIN);

  return ret;
}
