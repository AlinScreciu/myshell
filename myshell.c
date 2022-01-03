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
static sigjmp_buf env;
static volatile sig_atomic_t jumpable = 0;
void help()
{
    printf(
        "The implemented commands are:\n"
        "1. cd\n2. hist\n3. env\n4. cwd\n5. chmod not the same"
        );
}
void sigint_handler()
{
    if (!jumpable)
        return;

    siglongjmp(env, 42);
}
char *built_in_commands[] = {"hist", "cd", "env","help"};
int nr_of_built_in_commands = 4;

char *my_commands[] = {"cwd","more","diff", "chmod"};
int nr_of_my_commands = 4;

void hist()
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
typedef enum redir_mode{OUT_TRUNC,OUT_APP,IN}
redir_mode;
typedef struct 
{
    redir_mode mode;
    int mode_pos;
}  redir;

redir* init_redir()
{
    redir* _redir = malloc(sizeof(redir));
    _redir->mode = (redir_mode) NULL;
    _redir->mode_pos = 0;
    return _redir;
}
redir* check_redir(char **argv, int argc)
{
    redir* mode = init_redir();
    // >> > << < 
    // (cmd 1) <> r) | (cmd 2) <> r) | (cmd 3) <>
    
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "<") == 0)
        {
            mode->mode=IN;
            mode->mode_pos = i;
        }
        if (strcmp(argv[i], ">>") == 0)
        {
            mode->mode=OUT_APP;
            mode->mode_pos = i;

        }
        if (strcmp(argv[i], ">") == 0)
        {
            mode->mode=OUT_TRUNC;
            mode->mode_pos = i;
        }
    }
    return mode;

}
int exec_no_p(char *line, bool mine, char *cwd, char *home, char **env){
    

    int cmdargc = count_args(line, " ");
    char *args[cmdargc + 1];
    redir* mode = init_redir();
    parse_args(line, args, cmdargc);
    mode = check_redir(args, cmdargc);
    int saved_stdout,saved_stderr,saved_stdin;
    saved_stderr = dup(STDERR_FILENO);
    saved_stdout = dup(STDOUT_FILENO);
    saved_stdin = dup(STDIN_FILENO);    
    int fd;
    pid_t pid;
    int status;
    // cmd arg >/>>/</ { file to redir to}
    // cmd arg NULL file
    if (mode->mode_pos != 0)
    {
        if (mode->mode == OUT_TRUNC)
        {

            if (args[mode->mode_pos + 1] != NULL)
            {
                
                fd = open(args[mode->mode_pos + 1], O_WRONLY | O_CREAT);
                fchmod(fd,S_IROTH  | S_IRGRP | S_IRUSR | S_IWUSR );
                dup2(fd, STDOUT_FILENO);
                args[mode->mode_pos] = NULL;
            }
        }
        if (mode->mode == OUT_APP)
        {
            if (args[mode->mode_pos + 1] != NULL)
            { 
                fd = open(args[mode->mode_pos + 1], O_WRONLY | O_APPEND);
                dup2(fd, STDOUT_FILENO);
                args[mode->mode_pos] = NULL;
            }
        }
        if (mode->mode == IN)
        {
            if (args[mode->mode_pos + 1] != NULL)
            {
                fd = open(args[mode->mode_pos + 1], O_RDONLY);
                dup2(fd, STDIN_FILENO);
                args[mode->mode_pos] = NULL;
            }
        }
    }
    if ((pid = fork()) < 0)
    {
        printf("*** ERROR: forking child process failed\n");
        exit(1);
    }
    else if (pid == 0)
    {

        // execv("cmds/bin/cd" {"cd"}) //
        struct sigaction s_child;
        s_child.sa_handler = sigint_handler;
        sigemptyset(&s_child.sa_mask);
        s_child.sa_flags = SA_RESTART;
        sigaction(SIGINT, &s_child, NULL);
        
        if (!mine)
            if (execvpe(*args, args, env) < 0)
            {

                printf("Execution failed or the command doesn't exist on this system!\n");
                perror(*args);
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
            sprintf(path, "%s/myshell/commands/bin/%s", home, args[0]);
            if (execve(path, args, env) < 0)
            {
                perror(path);
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
    close(fd);
    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stdout);
    close(saved_stderr);   
    close(saved_stdin);
    
    return status;
}

static void
pipeline(char **cmd, char* cwd,char *home, char **env)
{
    int fd[2];
    pid_t pid;
    int fdd = 0; /* Backup */
    while (*cmd != NULL)
    {
        pipe(fd); /* Sharing bidiflow */
        bool mine = check_mine(*cmd);
        if ((pid = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }
        else if (pid == 0)
        {
            struct sigaction s_child;
            s_child.sa_handler = sigint_handler;
            sigemptyset(&s_child.sa_mask);
            s_child.sa_flags = SA_RESTART;
            sigaction(SIGINT, &s_child, NULL);
            dup2(fdd, 0);

            if (*(cmd + 1) != NULL)
            {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            exec_no_p(*cmd,mine, cwd, home, env);
            exit(0);
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
char **make_cmd_arr(char *line)
{
    int cmdc = count_args(line, "|");

    char **parsed_cmd = malloc(sizeof(char*)*cmdc + 1);
    parse_pipe(line, parsed_cmd, cmdc);

    parsed_cmd[cmdc] = NULL;
    return parsed_cmd;
}

void exec_pipe(char *line, char *cwd, char *home, char **env)
{
    char **commands = make_cmd_arr(line);
    pipeline(commands, cwd, home, env);
    free(commands);
}

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
void handle_external(char *line, char *cwd, char *home, char **env)
{
    bool pipeF = check_pipe(line);
    bool mine = check_mine(line);
    // parsing
    if (!pipeF)
    {
        exec_no_p(line, mine, cwd, home, env);
    }
    if (pipeF)
    {
        exec_pipe(line, cwd, home, env);
    }

}
void cd(int argc, char **args, char *cwd, char *home, char **env)
{
    int home_len = strlen(home);
    if (strcmp(cwd, "~") == 0)
    {
        envset(env, "OLDPWD", "~");
    }
    if (argc == 1)
        chdir(home);
        
    else if (chdir(args[1]) != 0)
    {
        perror("cd");
    };
    getcwd(cwd, PATH_MAX);

    if (strncmp(home, cwd, home_len) == 0)
    {
        sprintf(cwd, "~%s", cwd + home_len);
    }
    envset(env, "PWD", cwd);
}
void handle_built_in(char *line, char *cwd, char *home, char **env)
{
    int cmdargc = count_args(line, " ");

    char *args[cmdargc + 1];

    parse_args(line, args, cmdargc);

    if (strcmp(args[0], "cd") == 0)
    {
        cd(cmdargc, args, cwd, home, env);
    }
    if( strcmp(args[0], "hist") == 0)
    {
        hist();
    }
    if( strcmp(args[0], "help") == 0)
    {
        help();
    }
}
int main(int argc, char **argv, char **envp)
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

    char* cwd;
    cwd = getcwd(NULL, 0);
    char *home = (char *)malloc(sizeof(char) * (LOGIN_NAME_MAX + 6) + 1); //"/home/username/";
    sprintf(home, "/home/%s", user_name);
    int home_len = strlen(home);

    if (strncmp(home, cwd, home_len) == 0)
    {
        sprintf(cwd, "~%s", cwd + home_len);
    }

    char *prompt = (char *)malloc((strlen(user_name) + strlen(host_name) + PATH_MAX + 21) * sizeof(char));
    while (run)
    {
       if (sigsetjmp(env, 1) == 42)
        {
            printf("SIGINT\n");
        }
        jumpable = 1;
        sprintf(prompt, green "%s@%s" reset ":" magenta "%s" reset "$ ", user_name, host_name, cwd);

        char *line = readline(prompt);

        if (!line)
        {
            printf("\n");
            exit(1);
        }

        // Handle line
        

        if (strcmp(line, "exit") == 0)
        {
            run = false;
            free(line);
            continue;
        }

        if (strcmp(line, "") == 0)
            continue;
        if (*line)
        {
            add_history(line);
            remove_whitespace(line);
        }
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