#include <argp.h>
#include <fcntl.h>
// #include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_branch_prediction.h>
#include <rte_log.h>

#include "cleanflow.h"

volatile int exiting = false;

static void signal_handler(int signum) {
  if (signum == SIGINT)
    fprintf(stderr, "caught SIGINT\n");
  else if (signum == SIGTERM)
    fprintf(stderr, "caught SIGTERM\n");
  else
    fprintf(stderr, "caught unknown signal (%d)\n", signum);
  exiting = true;
}

static int run_signal_handler(void) {
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
    fprintf(stderr, "Error: failed to ignore SIGPIPE - %s\n", strerror(errno));
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

int main(int argc, char **argv) {
  int ret = 0;

  rte_log_register("cleanflow");

  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Error: rte_eal_init failed!");
  }

  argc -= ret;
  argv += ret;

  ret = run_signal_handler();
  if (ret < 0) {
    return 0;
  }

  while (likely(!exiting)) {
  }

  rte_eal_mp_wait_lcore();

  return ret;
}
