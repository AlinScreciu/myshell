
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
#define red     "\033[31m"
#define green   "\033[32m"
#define yellow  "\033[33m"
#define blue    "\033[34m"
#define magenta "\033[35m"
#define cyan    "\033[36m"
#define reset   "\033[0m"
#define ARG_MAX 131072
#include "util.c"


void handle_external(char *line)
{
    char line_copy[strlen(line) + 1];
    char line_copy2[strlen(line) + 1];

    strcpy(line_copy, line);
    strcpy(line_copy2, line);

    int cmdargc = 0;

    char *it = strtok(line_copy, " ");

    while (it != NULL)
    {
        cmdargc++;
        it = strtok(NULL, " ");
    }
    free(it);

    char *args[cmdargc + 1];
    args[cmdargc] = malloc(sizeof(char));
    args[cmdargc] = NULL;
    if (cmdargc > 1)
    {
        char *token = strtok(line_copy2, " ");
        int i = 0;
        while (token != NULL)
        {
            args[i] = malloc(strlen(token) + 1);
            strcpy(args[i], token);
            i++;
            token = strtok(NULL, " ");
        }

        free(token);
    }
    else
    {
        args[0] = malloc(sizeof(line_copy2) + 1);
        strcpy(args[0], line_copy2);
    }

    if (cmdargc == 1)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            printf("\nFailed forking child..\n");
            return;
        }
        else if (pid == 0)
        {
            if (execvp(args[0], args) < 0)
            {
                printf("Could not execute command..\n");
            }
            exit(0);
        }
        else
        {
            // waiting for child to terminate
            wait(NULL);
            return;
        }
    }
    else if (cmdargc >= 2)
    {
        pid_t pid1;
        int status;

        if ((pid1 = fork()) < 0)
        { /* fork a child process           */
            printf("*** ERROR: forking child process failed\n");
            exit(1);
        }
        else if (pid1 == 0)
        { /* for the child process:         */
            if (execvp(*args, args) < 0)
            { /* execute the command  */
                printf("Execution failed or the command\n");
                exit(1);
            }
        }
        else
        {                                 /* for the parent:      */
            while (wait(&status) != pid1) /* wait for completion  */
                ;
        }
    }
}
char *internal_commands[] = {"hist", "cd", "cwd"};
int nr_of_internal_commands = 3;
void handle_internal(char *line, char *cwd)
{

    bool has_args = true;
    
    for (int i = 0; i < nr_of_internal_commands; i++)
    {
        if (strcmp(internal_commands[i], line) == 0)
            has_args = false;
    }
    if (!has_args)
    {
        if (strcmp(line, "hist") == 0)
        {
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
        }
        if (strcmp(line, "cwd") == 0)
        {
            printf("The current working directory is: %s\n", cwd);
        }
        if (strcmp(line,"cd") == 0 )
        {
            printf("%s \n",getenv("HOME"));
            chdir(getenv("HOME"));
            getcwd(cwd,PATH_MAX);
        }
            
    }
    else
    {
        char line_copy[strlen(line) + 1];
        char line_copy2[strlen(line) + 1];

        strcpy(line_copy, line);
        strcpy(line_copy2, line);

        int cmdargc = 0;

        char *it = strtok(line_copy, " ");

        while (it != NULL)
        {
            cmdargc++;
            it = strtok(NULL, " ");
        }
        free(it);

        char *args[cmdargc + 1];
        args[cmdargc] = malloc(sizeof(char));
        args[cmdargc] = NULL;
        char *token = strtok(line_copy2, " ");
        int i = 0;
        while (token != NULL)
        {
            args[i] = malloc(strlen(token) + 1);
            strcpy(args[i], token);
            i++;
            token = strtok(NULL, " ");
        }
        
        free(token);
        if (strcmp(args[0], "cd") == 0)
        {
                char *path = (char *)malloc(sizeof(char) * (strlen(args[1]) + 4));
                strcpy(path, args[1]);
                chdir(path);
                getcwd(cwd, PATH_MAX);
        }
    }
}
void check_origin(bool *isinternal, char *line_to_copy)
{
    char *line = malloc(sizeof(char) * strlen(line_to_copy) + 1);
    strcpy(line, line_to_copy);
    char *command = strtok(line, " ");
    *isinternal = false;
    for (int i = 0; i < nr_of_internal_commands; i++)
    {
        if (strcmp(command, internal_commands[i]) == 0)
            *isinternal = true;
    }

    free(command);
}
int main(int argc, char **argv, char **envp)
{

    bool run = true;
    bool internal = false;
    
    char *user_name = (char *)malloc(sizeof(char) * LOGIN_NAME_MAX);
    char *host_name = (char *)malloc(sizeof(char) * HOST_NAME_MAX);
    cuserid(user_name);
    gethostname(host_name, HOST_NAME_MAX);
    
    
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    char *prompt = (char *)malloc((strlen(user_name) + strlen(host_name) + 20 + PATH_MAX) * sizeof(char));
    while (run)
    {
        
        sprintf(prompt,green"%s"red"-"cyan"%s"magenta"%s"green"$>" reset, host_name, user_name,cwd);

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

        if (strcmp(line,"") == 0) continue;

        check_origin(&internal, line);

        if (internal)
        {
            handle_internal(line, cwd);
        }

        if (!internal)
        {
            handle_external(line);
        }
        
    }
    
    // avoid memory leaks
    free(prompt);
    free(host_name);
    return 0;
}