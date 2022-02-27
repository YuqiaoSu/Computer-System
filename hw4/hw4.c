/* See Chapter 5 of Advanced UNIX Programming:  http://www.basepath.com/aup/
 *   for further related examples of systems programming.  (That home page
 *   has pointers to download this chapter free.
 *
 * Copyright (c) Gene Cooperman, 2006; May be freely copied as long as this
 *   copyright notice remains.  There is no warranty.
 */

/* To know which "includes" to ask for, do 'man' on each system call used.
 * For example, "man fork" (or "man 2 fork" or man -s 2 fork") requires:
 *   <sys/types.h> and <unistd.h>
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXLINE 200  /* This is how we declare constants in C */
#define MAXARGS 20
#define PIPE_READ 0
#define PIPE_WRITE 1
/* In C, "static" means not visible outside of file.  This is different
 * from the usage of "static" in Java.
 * Note that end_ptr is an output parameter.
 */
static char * getword(char * begin, char **end_ptr) {
    char * end = begin;

    while ( *begin == ' ' )
        begin++;  /* Get rid of leading spaces. */
        end = begin;
    while ( *end != '\0' && *end != '\n' && *end != ' ' && *end != '#')//handle comment
        end++;  /* Keep going. */
    if ( end == begin )
        return NULL;  /* if no more words, return NULL */
    *end = '\0';  /* else put string terminator at end of this word. */
    *end_ptr = end;
    if (begin[0] == '$') { /* if this is a variable to be expanded */
        begin = getenv(begin+1); /* begin+1, to skip past '$' */
	if (begin == NULL) {
	    perror("getenv");
	    begin = "UNDEFINED";
        }
    }
    return begin; /* This word is now a null-terminated string.  return it. */
}

/* In C, "int" is used instead of "bool", and "0" means false, any
 * non-zero number (traditionally "1") means true.
 */
/* argc is _count_ of args (*argcp == argc); argv is array of arg _values_*/
static void getargs(char cmd[], int *argcp, char *argv[])
{
    char *cmdp = cmd;
    char *end;
    int i = 0;

    /* fgets creates null-terminated string. stdin is pre-defined C constant
     *   for standard intput.  feof(stdin) tests for file:end-of-file.
     */
    if (fgets(cmd, MAXLINE, stdin) == NULL && feof(stdin)) {
        printf("Couldn't read from standard input. End of file? Exiting ...\n");
        exit(1);  /* any non-zero value for exit means failure. */
    }
    while ( (cmdp = getword(cmdp, &end)) != NULL ) { /* end is output param */
        /* getword converts word into null-terminated string */
            argv[i++] = cmdp;
        /* "end" brings us only to the '\0' at end of string */
	    cmdp = end + 1;
    }
    argv[i] = NULL; /* Create additional null word at end for safety. */
    *argcp = i;
}

static void execute(int argc, char *argv[])
{
    pid_t childpid; /* child process ID */
    pid_t childpid2 = 1; /* child process ID */


    int flag = 0;
    int fd[2];
    int waitflag = 1;

    // Error checking
    if (pipe(fd) == -1) {
        printf("An error occurred with opening the pipe\n");
        return;
    }

    childpid = fork();
    int i;
    for (i = 0; argv[i] != NULL; i++) {
        if (*argv[i] == '|') {
            if(childpid > 0) {
                childpid2 = fork();//if we are piping, fork the second child to handle pipe
                break;
            }
        }
    }
   

    if (childpid == -1) { /* in parent (returned error) */
        perror("fork"); /* perror => print error string of last system call */
        printf("  (failed to execute command)\n");
    }

    // First process
    if (childpid == 0 && childpid2 != 0) { /* child:  in child, childpid was set to 0 */
        /* Executes command in argv[0];  It searches for that file in
	 *  the directories specified by the environment variable PATH.
         */
        // Search for command characters in user input
        int i;
        for (i = 0; argv[i] != NULL; i++) {
            if (*argv[i] == '>') {
                close(1);
                fd[PIPE_WRITE] = open(argv[i + 1], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                flag = 1;
                int j = i;
                while (argv[j] != NULL) {
                    argv[j] = NULL;
                    j++;
                }
            } else if (*argv[i] == '<') {
                fd[PIPE_WRITE] = open(argv[i + 1], O_RDONLY, 0644);
                close(STDIN_FILENO);
                dup(fd[PIPE_WRITE]);
                close(fd[PIPE_WRITE]);
                flag = 1;
                int j = i;
                while (argv[j] != NULL) {
                    argv[j] = NULL;
                    j++;
                }
            } else if (*argv[i] == '&') {
                waitflag = 0;
                int j = i;
                while (argv[j] != NULL) {
                    argv[j] = NULL;
                    j++;
                }
                break;
            } else if (*argv[i] == '|') {
                close(STDOUT_FILENO);
                dup2(fd[PIPE_WRITE],STDOUT_FILENO); // If pipe, we first store things from executing first command
                close(fd[PIPE_READ]);//close out file descriptor
                close(fd[PIPE_WRITE]);
                int j = i;
                while (argv[j] != NULL) {
                    argv[j] = NULL;
                    j++;
                }
                break;
            }
        }

        if (-1 == execvp(argv[0], argv)) {
          perror("execvp");
          printf("  (couldn't find command)\n");
        }
	/* NOT REACHED unless error occurred */
        exit(1);

    // Second Process
    } else if (childpid2 == 0) {

        int i;
        for (i = 0; argv[i] != NULL; i++) {
            if (*argv[i] == '|') {
                close(STDIN_FILENO);
                dup2(fd[PIPE_READ],STDIN_FILENO);// retrieve what we stored and use that to execute second command
                close(fd[PIPE_READ]);
                close(fd[PIPE_WRITE]);
                argv[0] = argv[i + 1];
                argv[1] = NULL;
            }
        }

        if (-1 == execvp(argv[0], argv)) {
          perror("execvp");
          printf("  (couldn't find command)\n");
        }

         exit(1);       

    }
     else { /* parent:  in parent, childpid was set to pid of child process */

        close(fd[PIPE_READ]);//close fd in parent so that ptr problem professor said in class won't happen
        close(fd[PIPE_WRITE]);

        int i;
        for (i = 0; argv[i] != NULL; i++) {
            if (*argv[i] == '&') {
                waitflag = 0;
                int j = i;
                while (argv[j] != NULL) {
                    argv[j] = NULL;
                    j++;
                }
                break;
            }
            if (*argv[i] == '|' && childpid2 == 1) {
                childpid2 = fork();// If piped, we have processed the first part of command, and fork for second child for second command.
                break;
            }

        }
        if (waitflag == 1) {
            waitpid(childpid, NULL, 0);  /* wait until child process finishes */
        }
        waitpid(childpid2, NULL, 0);
        // return;
    }
  return; 
}

void interrupt_handler(int signum) {
   printf("  # [control-C] \n");
}


int main(int argc, char *argv[])
{
    char cmd[MAXLINE];
    char *childargv[MAXARGS];
    int childargc;

    signal(SIGINT, interrupt_handler); // handle third bug

    if (argc > 1) {
        freopen(argv[1], "r", stdin); //handle second bug
    }

    while (1) {
        printf("%% "); /* printf uses %d, %s, %x, etc.  See 'man 3 printf' */
        fflush(stdout); /* flush from output buffer to terminal itself */
	getargs(cmd, &childargc, childargv); /* childargc and childargv are
            output args; on input they have garbage, but getargs sets them. */
        /* Check first for built-in commands. */
        if ( childargc > 0 && strcmp(childargv[0], "exit") == 0 ) {
                exit(0);
        }
        else if ( childargc > 0 && strcmp(childargv[0], "logout") == 0 ) {
                exit(0);
        }
        else {
            execute(childargc, childargv);
        }
    }
    /* NOT REACHED */
}
