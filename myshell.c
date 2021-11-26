#define _GNU_SOURCE
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
#define red "\033[31m"
#define green "\033[32m"
#define yellow "\033[33m"
#define blue "\033[34m"
#define magenta "\033[35m"
#define cyan "\033[36m"
#define reset "\033[0m"
#define ARG_MAX 131072 / 4
#include "util.c"
char *built_in_commands[] = {"hist", "cd", "env"};
int nr_of_built_in_commands = 3;

char *my_commands[] = {"cwd"};
int nr_of_my_commands = 1;
/*
register HIST_ENTRY **temp;
            register int i;
            temp = history_list();
            if (temp)
            {
                for (i = 0; temp[i]; i++)
                {
                    printf("%d: %s\n", i + history_base, temp[i]->line);
                }
            }
*/
/*
if (strcmp(line, "cwd") == 0)
        {
            pid_t pid;
            int status;
            if ((pid = fork()) < 0)
            {
                printf("*** ERROR: forking child process failed\n");
                exit(1);
            }
            else if (pid == 0)
            {
                // execv("cmds/bin/cd" {"cd"}) //
                if (execl("../commands/bin/cwd", "cwd", NULL) < 0)
                {
                    printf("Execution failed or the command doesn't exist on this system!\n");
                    exit(1);
                }
            }
            else
            {
                while (wait(&status) != pid)
                    ;
            }
*/
bool check_origin(char *line_to_copy)
{
    char *line = malloc(sizeof(char) * strlen(line_to_copy) + 1);
    strcpy(line, line_to_copy);
    char *command = strtok(line, " ");
    bool isbuilt_in = false;
    for (int i = 0; i < nr_of_built_in_commands; i++)
    {
        if (strcmp(command, built_in_commands[i]) == 0)
            isbuilt_in = true;
    }

    free(command);
    return isbuilt_in;
}
bool check_mine(char *line_to_copy)
{
    bool mine = false;
    char *line = malloc(sizeof(char) * strlen(line_to_copy) + 1);
    strcpy(line, line_to_copy);
    char *command = strtok(line, " ");
    for (int i = 0; i < nr_of_my_commands; i++)
    {
        if (strcmp(command, my_commands[i]) == 0)
            mine = true;
    }

    free(command);
    return mine;
}
static void
pipeline(char ***cmd)
{
    int fd[2];
    pid_t pid;
    int fdd = 0; /* Backup */
    while (*cmd != NULL)
    {
        pipe(fd); /* Sharing bidiflow */
        if ((pid = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }
        else if (pid == 0)
        {
            dup2(fdd, 0);
            if (*(cmd + 1) != NULL)
            {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            execvp((*cmd)[0], *cmd);
            exit(1);
        }
        else
        {
            wait(NULL); /* Collect childs */
            close(fd[1]);
            fdd = fd[0];
            cmd++;
        }
    }
}
/*{
    // "ls -a -e" "cd -e -b"
    {   {ls},{-a},{-e}, NULL    }, // ls + args end with null
    {   {cd},{-e},{-b}, NULL    }, // cat + args end with null
    NULL // cmd parsed + null
}*/
char ***make_cmd_arr(char *line)
{
    int cmdc = count_args(line, "|");
    
    char ***parsed_cmd_and_args = malloc(sizeof(char **) * (cmdc + 1));
    char *parsed_cmd[cmdc];
    // {{"ls -a"},{"cat -e"}}
    //
    parse_pipe(line, parsed_cmd, cmdc);

    for (int i = 0; i < cmdc; i++)
    {
        int argc = count_args(parsed_cmd[i], " ");
        printf("%d",argc);
        char *args[argc + 1];
        parsed_cmd_and_args[i] = malloc(argc * ARG_MAX);
        printf("%s \n",parsed_cmd[i]);
        parse_args(parsed_cmd[i], args, argc);
        printf("%s \n",args[i]);
        memcpy(parsed_cmd_and_args[i], args, (argc + 1) * (sizeof(char *)));
    }

    parsed_cmd_and_args[cmdc] = NULL;

    return parsed_cmd_and_args;
}
int exec_no_pr(char *line, bool mine, char *cwd, char *home, char **env)
{
    int cmdargc = count_args(line, " ");
    char *args[cmdargc + 1];
    parse_args(line, args, cmdargc);

    pid_t pid;
    int status;
    if ((pid = fork()) < 0)
    {
        printf("*** ERROR: forking child process failed\n");
        exit(1);
    }
    else if (pid == 0)
    {

        // execv("cmds/bin/cd" {"cd"}) //
        if (!mine)
            if (execvpe(*args, args, env) < 0)
            {
                printf("Execution failed or the command doesn't exist on this system!\n");
                exit(1);
            }
            else
            {
                while (wait(&status) != pid)
                    ;
            }
        else
        {
            char *path = (char *)malloc(sizeof(char) * PATH_MAX + 1);
            sprintf(path, "%s/myshell/commands/bin/%s",home,args[0]);
            if (execve(path, args, env) < 0)
            {
                printf("Execution failed or the command doesn't exist on this system!\n");
                exit(1);
            }
            else
            {
                while (wait(&status) != pid)
                    ;
            }
        }
    }
    else
    {
        while (wait(&status) != pid)
            ;
    }
    return status;
}

int exec_pipe(char *line, bool mine, char *cwd, char *home, char **env)
{
    char ***commands = make_cmd_arr(line);
    pipeline(commands);
    free(commands);
};
bool check_pipe(char *line)
{
    bool pipef = false;
    bool inq = false; // in '' or ""

    while (*line++)
    {
        if (*line == '\'' || *line == '"')
        {
            if (inq)
                inq = false;
            else
                inq = true;
        }
        if (*line == '|' && !inq)
            pipef = true;
    }
    return pipef;
}
bool check_redir(char *line)
{
    bool redirf = false;
    bool inq = false; // in '' or ""

    while (*line++)
    {
        if (*line == '\'' || *line == '"')
        {
            if (inq)
                inq = false;
            else
                inq = true;
        }
        if (*line == '<' && !inq)
            redirf = true;
    }
    return redirf;
}
void handle_external(char *line, char *cwd, char *home, char **env)
{
    bool pipeF = check_pipe(line);
    bool redirF = check_redir(line);
    bool mine = check_mine(line);
    // parsing
    if (!(pipeF || redirF))
    {
        exec_no_pr(line, mine, cwd, home, env);
    }
    if (pipeF && !redirF)
    {
        exec_pipe(line, mine, cwd, home, env);
    }
}
void handle_built_in(char *line, char *cwd, char *home, char **env)
{
    int cmdargc = count_args(line, " ");

    char *args[cmdargc + 1];

    parse_args(line, args, cmdargc);

    if (strcmp(args[0], "cd") == 0)
    {
        int home_len = strlen(home);
        if (strcmp(cwd, "~") == 0)
        {
            envset(env, "OLDPWD", "~");
        }
        if (cmdargc == 1)
            chdir(home);
        else
            chdir(args[1]);
        getcwd(cwd, PATH_MAX);

        if (strncmp(home, cwd, home_len) == 0)
        {
            sprintf(cwd, "~%s", cwd + home_len);
        }
        envset(env, "PWD", cwd);
    }
}
int main(int argc, char **argv, char **envp)
{

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
    char *prompt = (char *)malloc((strlen(user_name) + strlen(host_name) + PATH_MAX + 21) * sizeof(char));
    while (run)
    {

        sprintf(prompt, green "%s" red "@" cyan "%s" reset ":" magenta "%s" green "$ " reset, user_name, host_name, cwd);
        
        char *line = readline(prompt);

        if (!line)
        {
            exit(1);
        }

        // Handle line
        if (*line)
        {
            add_history(line);
            remove_whitespace(line);
        }

        if (strcmp(line, "exit") == 0)
        {
            run = false;
            free(line);
            continue;
        }

        if (strcmp(line, "") == 0)
            continue;

        built_in = check_origin(line);

        if (built_in)
        {
            handle_built_in(line, cwd, home, environ);
        }

        if (!built_in)
        {
            handle_external(line, cwd, home, environ);
        }
    }

    // avoid memory leaks
    free(prompt);
    free(host_name);
    return 0;
}