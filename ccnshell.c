#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "linenoise.h"

void completion(const char *buf, linenoiseCompletions *lc)
{
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc, "hello");
        linenoiseAddCompletion(lc, "hello there");
    }
}

char *hints(const char *buf, int *color, int *bold)
{
    if (!strcasecmp(buf, "hello")) {
        *color = 35;
        *bold = 0;
        return " World";
    }
    return NULL;
}

int main(int argc, char **argv)
{
    char *line;
    char *prgname = argv[0];
    int async = 0;

    /* Parse options, with --multiline we enable multi line editing. */
    while (argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv, "--multiline")) {
            linenoiseSetMultiLine(1);
            printf("Multi-line mode enabled.\n");
        } else if (!strcmp(*argv, "--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        } else if (!strcmp(*argv, "--async")) {
            async = 1;
        } else {
            fprintf(stderr, "Usage: %s [--multiline] [--keycodes] [--async]\n",
                    prgname);
            exit(1);
        }
    }

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */

    /* Now this is the main loop of the typical linenoise-based application.
     * The call to linenoise() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * linenoise, so the user needs to free() it. */

    while (1) {
        if (!async) {
            line = linenoise("CCNShell >> ");
            if (line == NULL)
                break;
        } else {
            /* Asynchronous mode using the multiplexing API: wait for
             * data on stdin, and simulate async data coming from some source
             * using the select(2) timeout. */
            struct linenoiseState ls;
            char buf[1024];
            linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), "CCNShell >> ");
            while (1) {
                fd_set readfds;
                struct timeval tv;
                int retval;

                FD_ZERO(&readfds);
                FD_SET(ls.ifd, &readfds);
                tv.tv_sec = 1;  // 1 sec timeout
                tv.tv_usec = 0;

                retval = select(ls.ifd + 1, &readfds, NULL, NULL, &tv);
                if (retval == -1) {
                    perror("select()");
                    exit(1);
                } else if (retval) {
                    line = linenoiseEditFeed(&ls);
                    /* A NULL return means: line editing is continuing.
                     * Otherwise the user hit enter or stopped editing
                     * (CTRL+C/D). */
                    if (line != linenoiseEditMore)
                        break;
                } else {
                    // Timeout occurred
                    static int counter = 0;
                    linenoiseHide(&ls);
                    printf("Async output %d.\n", counter++);
                    linenoiseShow(&ls);
                }
            }
            linenoiseEditStop(&ls);
            if (line == NULL)
                exit(0); /* Ctrl+D/C. */
        }

        // TODO: Make it modular
        /* Do something with the string. */
        if (line[0] != '\0' && line[0] != '/') {
            linenoiseHistoryAdd(line);           /* Add to the history. */
            linenoiseHistorySave("history.txt"); /* Save the history on disk. */
            int itr = 0, arg_idx = 1;
            // At most 32 arguments. It will be copied to memory as argv for new
            // process SO THE FIRST ONE SHOULD BE THE PATH OF EXECUTABLE (BY
            // CONVENTION) This is weird. Who would use the first one? Every
            // thing as sting is weird as well.....
            char *args[32];
            char *envp[] = {NULL};
            char path[128] = "/usr/bin/";
            memset(args, 0, 32 * sizeof(char *));

            // TODO: should trim trailing spaces first
            args[0] = line;
            while (line[itr + 1] != '\0') {
                if (line[itr] == ' ') {
                    line[itr] = '\0';
                    args[arg_idx] = line + itr + 1;
                    arg_idx++;
                }
                itr++;
            }

            // FIXME: C-c sends signale to both parent and child, resulting
            // ending both
            pid_t pid = fork();
            if (pid == -1) {
                printf("failed to fork\n");
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                execve(strncat(path, line, 127 - 9), args, envp);
                fprintf(stderr, "|| Failed to execute '%s': %s ||\n", line,
                        strerror(errno));
                exit(EXIT_FAILURE);  // What is the difference with _exit()?
            }
        } else if (!strncmp(line, "/historylen", 11)) {
            /* The "/historylen" command will change the history len. */
            int len = atoi(line + 11);
            linenoiseHistorySetMaxLen(len);
        } else if (!strncmp(line, "/mask", 5)) {
            linenoiseMaskModeEnable();
        } else if (!strncmp(line, "/unmask", 7)) {
            linenoiseMaskModeDisable();
        } else if (line[0] == '/') {
            printf("Unreconized command: %s\n", line);
        }
        free(line);
    }
    return 0;
}
