#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <setjmp.h>
#define red "\033[31m"
#define green "\033[38;5;10m"
#define yellow "\033[33m"
#define blue "\033[34m"
#define magenta "\033[38;5;13m"
#define cyan "\033[36m"
#define reset "\033[0m"
#include "util.c"

int main(int argc, char **argv, char** envp)
{
    setsid();
    struct sigaction s;
    s.sa_handler = sigint_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = SA_RESTART;
    sigaction(SIGINT, &s, NULL);


    bool run = true;
    bool built_in = false;

    char *user_name = (char *)malloc(sizeof(char) * LOGIN_NAME_MAX);
    char *host_name = (char *)malloc(sizeof(char) * HOST_NAME_MAX);
    cuserid(user_name);
    gethostname(host_name, HOST_NAME_MAX);

    char **environ = envp;

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    char *home = (char *)malloc(sizeof(char) * (LOGIN_NAME_MAX + 8)); //"/home/username/";
    sprintf(home, "/home/%s", user_name);
    int home_len = strlen(home);

    if (strncmp(home, cwd, home_len) == 0)
    {
        sprintf(cwd, "~%s", cwd + home_len);
    }

    char *prompt = (char *)malloc((strlen(user_name) + strlen(host_name) + PATH_MAX + strlen(green) + 2 * strlen(reset) + strlen(magenta) + 2) * sizeof(char) + 1);

     while (run)
    {
        if (sigsetjmp(env, 1) == 42)
        {
            printf("\n");
            continue;
        }
        jumpable = 1;
        sprintf(prompt, green "%s@%s" reset ":" magenta "%s" reset "$ ", user_name, host_name, cwd);

        char *line = readline(prompt);

        if (!line)
        {
            printf("\n");
            exit(1);
        }
        
        if (*line)
        {
            add_history(line);
            remove_whitespace(line);
        }
    }

}
