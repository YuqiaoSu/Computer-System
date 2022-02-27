/*************************************************************************
 *  Copyright 2020-2021 Gene Cooperman                                        *
 *                                                                       *
 *  This file is part of McMini.                                         *
 *                                                                       *
 *  McMini is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by *
 *  the Free Software Foundation, either version 3 of the License, or    *
 *  (at your option) any later version.                                  *
 *                                                                       *
 *  McMini is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                         *
 *                                                                       *
 *  You should have received a copy of the GNU General Public License    *
 *  along with McMini.  If not, see <https://www.gnu.org/licenses/>.     *
 *************************************************************************/

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

int main(int argc, char* argv[]) {
  char **cur_arg = argv+1;
  if (argc == 1) { cur_arg[0] = "--help"; cur_arg[1] = NULL; }
  while (cur_arg[0] != NULL && cur_arg[0][0] == '-') {
    if (strcmp(cur_arg[0], "--max-trace-length") == 0) {
      setenv("MCMINI_MAX_TRACE_LENGTH", cur_arg[1], 1);
      cur_arg += 2;
    } else if (strcmp(cur_arg[0], "--max-frontier-length") == 0) {
      setenv("MCMINI_MAX_FRONTIER_LENGTH", cur_arg[1], 1);
      cur_arg += 2;
    } else if (strcmp(cur_arg[0], "--parallel") == 0) {
      setenv("MCMINI_PARALLEL", "1", 1);
      cur_arg++;
    } else if (strcmp(cur_arg[0], "--verbose") == 0) {
      setenv("MCMINI_VERBOSE", "1", 1);
      cur_arg++;
    } else if (strcmp(cur_arg[0], "--initial-trace") == 0) {
      if (cur_arg[1] == NULL) {
        cur_arg[0] = "--help";
      } else {
        setenv("MCMINI_INITIAL_TRACE", cur_arg[1], 1);
        cur_arg += 2;
        // NOTE:  Could also directly set DEFAULT_INITIAL_TRACE in mc.c
      }
    } else if (strcmp(cur_arg[0], "--gdb") == 0) {
      setenv("MCMINI_GDB", "true", 1);
      cur_arg ++;
    } else if (strcmp(cur_arg[0], "--help") == 0 ||
               strcmp(cur_arg[0], "-h") == 0) {
      // --parallel runs NUM_PARRALLEL (10) traces in parallel.
      fprintf(stderr, "Usage: mcmini [--max-trace-length <num>] "
                      "[--max-frontier-length <num>]\n"
                      "              [--initial-trace '<trace>'] [--gdb]\n"
                      "              [--verbose] [--parallel] [--help|-h] "
                      "target_executable\n");
      exit(1);
    } else {
      printf("mcmini: unrecognized option: %s\n", cur_arg[0]);
      exit(1);
    }
  }
  struct stat stat_buf;
  if (cur_arg[0] == NULL || stat(cur_arg[0], &stat_buf) == -1) {
    fprintf(stderr, "*** Missing target_executable or no such file.\n\n");
    exit(1);
  }
  if (getenv("MCMINI_GDB") && ! getenv("MCMINI_INITIAL_TRACE")) {
    fprintf(stderr,
             "*** --gdb invoked without specifying --initial-trace.\n\n");
    exit(1);
  }

  char buf[1000];
  buf[sizeof(buf)-1] = '\0';
  // We add ours to the end of any PRELOAD of the target application.
  snprintf(buf, sizeof buf, "%s:%s/libmc.so",
           (getenv("LD_PRELOAD") ? getenv("LD_PRELOAD") : ""),
           dirname(argv[0]));
  // Guard against buffer overrun:
  assert(buf[sizeof(buf)-1] == '\0');
  setenv("LD_PRELOAD", buf, 1);

  // FIXME: Write a mcmini-gdbinit.PID script first, and then source it in
  //        with "-x", "source /tmp/mcmini-gdbinit.PID", and then unlink
  //        it with "-x" "shell rm /tmp/mcmini-gdbinit.PID".
  //        And use 'define posthook-continue' to call 'where'.
  //        Writing script is safer for security than using a pre-existing file.
  // Useful GDB commands:  display *mc_trace@20; set print array-indexes
  char *gdb_args[100] = {"gdb",
    "-ex", "set follow-fork-mode child",
    "-ex", "set breakpoint pending on",
    "-ex", "break mc_yield_to_thread",
    "-ex", "break _exit",
    "-ex", "display mc_trace_index",
    // "-ex", "dprintf mc_yield_to_thread, \"%s\", mc_debug()",
    "-ex", "python def mcDebug(event):gdb.execute"
                                            "('printf \"%s\", mc_debug()')",
    "-ex", "python gdb.events.stop.connect(mcDebug)",
    "-ex", "python def showStack(event):gdb.execute('where')",
    "-ex", "python gdb.events.stop.connect(showStack)",
    "-ex", "echo \n  *** '--gdb' was invoked:\n"
    "  ***   NOTE:  For debugging initial trace, breakpoint already set at: \n"
    "               'mc_yield_to_thread', and 'set follow-fork-mode child'\n"
    "  ***   NOTE:  For debugging mc.c (all trace schedules), do \n"
    "               'break model_check' and 'set follow-fork-mode parent'\n\n"
    "  ***   NOTE:  CONSIDER: (gdb) dprintf FUNCTION, "", mc_debug()",
    "--args", "env", NULL};
  if (getenv("MCMINI_GDB")) {
    snprintf(buf, sizeof buf, "LD_PRELOAD=%s", getenv("LD_PRELOAD"));
    unsetenv("LD_PRELOAD");
    int i, j;
    for (i = 0; gdb_args[i] != NULL; i++) { }
    gdb_args[i++] = buf; // This comes right after 'env' arg
    for (j = 0; cur_arg[j] != NULL; i++, j++) {
      gdb_args[i] = cur_arg[j];
    }
    gdb_args[i] = NULL;
    cur_arg = gdb_args;
  }
  int rc = execvp(cur_arg[0], cur_arg);
  fprintf(stderr, "Executable '%s' not found.\n", cur_arg[0]);
  perror("mcmini");
  return 1;
}
