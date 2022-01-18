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
#define red "\001\033[31m\002"
#define green "\001\033[38;5;10m\002"
#define yellow "\001\033[33m\002"
#define blue "\001\033[34m\002"
#define magenta "\001\033[38;5;13m\002"
#define cyan "\001\033[36m\002"
#define reset "\001\033[0m\002"
#define greenlen strlen(green)
#define redlen strlen(red)
#define yellowlen strlen(yellow)
#define bluelen strlen(blue)
#define magentalen strlen(magenta)
#define cyanlen strlen(cyan)
#define resetlen strlen(reset)
//
int help();
int hist();
int get_env();
int cd(char **, char **);
int version()
{
    fputs("This is version \033[31m2\033[0m of \033[31mMyShell\033[0m\n", stdout);
    fputs("Author: \033[31mScreciu Alin-Constantin\033[0m\n", stdout);

    return 0;
}
char *built_in_commands[] = {"hist", "cd", "env", "help", "version"};
int (*built_in_func[])(char **, char **) = {&hist, &cd, &get_env, &help, &version};
int num_builtins = sizeof(built_in_commands) / sizeof(char *);
char *custom_commands[] = {"chmod", "cp", "cwd", "diff", "more"};
int num_custom = sizeof(custom_commands) / sizeof(char *);
//

// ctrl c magic
static sigjmp_buf sig_env;
static volatile sig_atomic_t jumpable = 0;
void sigint_handler();
//

// bread and butter
char *get_cwd(const char *);
bool check_pipe(const char *);
char **parse_line(const char *, const char *);
int execute(const char *, char **);
int exec_pipeline(const char *, char **);
char *remove_whitespace(const char *str);
int run_command(char **, char **);
//

// is this even useful?
//  void env_set(char **env, char *name, char *value);
//

int main(int argc, char **argv, char **envp)
{
    bool server = false;
    setsid();

    if (argc > 1)
    {
        server = true;
    }
    struct sigaction s;
    s.sa_handler = sigint_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = SA_RESTART;
    sigaction(SIGINT, &s, NULL);
    char **env = envp;
    char *user_name = (char *)malloc(sizeof(char) * LOGIN_NAME_MAX);
    char *host_name = (char *)malloc(sizeof(char) * HOST_NAME_MAX);
    cuserid(user_name);
    gethostname(host_name, HOST_NAME_MAX);
    bool run = true;
    char *cwd;
    using_history();
    while (run)
    {
        jumpable = 1;
        if (sigsetjmp(sig_env, 1) == 42)
        {
            printf("\n");
        }
        cwd = get_cwd(user_name);
        char *prompt = (char *)malloc((strlen(host_name) + strlen(user_name) + strlen(cwd) + 2 * resetlen + greenlen + magentalen + 5) * sizeof(char));
        sprintf(prompt, green "%s@%s" reset ":" magenta "%s" reset "$ ", user_name, host_name, cwd);
        char *line;
        if (!server)
            line = readline(prompt);
        else
            line = strdup(argv[1]);
        if (line == NULL)
        {
            printf("\n");
            continue;
        }
        if (strcmp(line, "") == 0)
        {

            continue;
        }
        if (strcmp(line, "exit") == 0)
        {
            free(prompt);
            free(line);
            free(cwd);
            free(host_name);
            free(user_name);
            exit(1);
        }
        add_history(line);
        bool pipe = check_pipe(line);
        int status;
        if (pipe)
        {
            status = exec_pipeline(line, env);
        }
        else
        {
            status = execute(line, env);
        }

        free(prompt);
        free(line);
        free(cwd);
        if (server)
            break;
    }
    free(host_name);
    free(user_name);
    return 0;
}
char *get_cwd(const char *user)
{
    char *cwd = (char *)malloc(sizeof(char) * PATH_MAX);
    getcwd(cwd, sizeof(char) * PATH_MAX);
    char *home = (char *)malloc(sizeof(char) * (LOGIN_NAME_MAX + 6) + 1); //"/home/username/";
    sprintf(home, "/home/%s", user);
    int home_len = strlen(home);
    if (strncmp(home, cwd, home_len) == 0)
    {
        sprintf(cwd, "~%s", cwd + home_len);
    }
    free(home);
    return cwd;
}
bool check_pipe(const char *line)
{
    bool q = false;
    for (size_t i = 0; i < strlen(line); i++)
    {
        if (line[i] == '\'' || line[i] == '"')
        {
            q = !q;
            continue;
        }
        if (!q && line[i] == '|')
        {
            return true;
        }
    }
    return false;
}

void sigint_handler()
{
    if (!jumpable)
        return;
    siglongjmp(sig_env, 42);
}
char **parse_line(const char *line, const char *delim)
{
    int count = 0;
    char *copy = strdup(line);
    char *token = strtok(copy, delim);
    char **tokens = (char **)malloc((count + 1) * sizeof(char *));
    while (token != NULL)
    {
        // [b] [a] [null]
        tokens[count++] = remove_whitespace(token);
        char **check = (char **)realloc(tokens, (count + 1) * sizeof(char *));
        if (check == NULL)
        {
            fprintf(stderr, "myshell: realloc failure for token parsing: ");
            perror(NULL);
            exit(1);
        }
        else
            tokens = check;
        token = strtok(NULL, delim);
    }
    free(token);
    free(copy);
    tokens[count] = NULL;
    return tokens;
}
typedef enum redir_mode
{
    OUT_TRUNC,
    OUT_APP,
    IN
} redir_mode;
typedef struct
{
    redir_mode mode;
    int mode_pos;
} redir;

redir *init_redir()
{
    redir *_redir = (redir *)malloc(sizeof(redir));
    _redir->mode = (redir_mode)NULL;
    _redir->mode_pos = -1;
    return _redir;
}
redir *check_redir(char **argv)
{
    redir *mode = init_redir();

    for (int i = 0; argv[i] != NULL; i++)
    {
        if (strcmp(argv[i], "<") == 0)
        {
            mode->mode = IN;
            mode->mode_pos = i;
        }
        if (strcmp(argv[i], ">>") == 0)
        {
            mode->mode = OUT_APP;
            mode->mode_pos = i;
        }
        if (strcmp(argv[i], ">") == 0)
        {
            mode->mode = OUT_TRUNC;
            mode->mode_pos = i;
        }
    }
    return mode;
}
int execute(const char *line, char **env)
{

    char **argv = parse_line(line, " \t\r\n\v\f");
    redir *mode = check_redir(argv);
    int saved_stdout, saved_stderr, saved_stdin, fd, status;
    if (mode->mode_pos != -1)
    {
        saved_stderr = dup(STDERR_FILENO);
        saved_stdout = dup(STDOUT_FILENO);
        saved_stdin = dup(STDIN_FILENO);
        if (mode->mode == OUT_TRUNC)
        {

            if (argv[mode->mode_pos + 1] != NULL)
            {

                fd = open(argv[mode->mode_pos + 1], O_WRONLY | O_CREAT | O_TRUNC);
                fchmod(fd, S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);
                dup2(fd, STDOUT_FILENO);
                argv[mode->mode_pos] = NULL;
            }
        }
        if (mode->mode == OUT_APP)
        {
            if (argv[mode->mode_pos + 1] != NULL)
            {
                fd = open(argv[mode->mode_pos + 1], O_WRONLY | O_APPEND);
                dup2(fd, STDOUT_FILENO);
                argv[mode->mode_pos] = NULL;
            }
        }
        if (mode->mode == IN)
        {
            if (argv[mode->mode_pos + 1] != NULL)
            {
                fd = open(argv[mode->mode_pos + 1], O_RDONLY);
                fchmod(fd, S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);
                dup2(fd, STDIN_FILENO);
                argv[mode->mode_pos] = NULL;
            }
        }
    }
    bool builtin = false;
    for (int i = 0; i < num_builtins; i++)
    {
        if (strcmp(argv[0], built_in_commands[i]) == 0)
        {
            builtin = true;
            status = (*built_in_func[i])(argv, env);
        }
    }
    if (!builtin)
    {
        status = run_command(argv, env);
    }
    if (mode->mode_pos != -1)
    {
        close(fd);
        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stdout);
        close(saved_stderr);
        close(saved_stdin);
    }
    for (int i = 0; argv[i] != NULL; i++)
    {
        free(argv[i]);
    }
    free(argv);
    free(mode);
    return status;
}
int exec_pipeline(const char *line, char **env)
{

    char **cmds = parse_line(line, "|");
    int fd[2];
    pid_t pid, wpid;
    int in = 0, cit = 0, status = 0;

    while (cmds[cit] != NULL)
    {

        pipe(fd);
        pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "couldn't fork: ");
            perror(NULL);
            return -1;
        }
        if (pid == 0)
        {
            dup2(in, 0);
            if (cmds[cit + 1] != NULL)
            {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            status = execute(cmds[cit], env);
            exit(status);
        }
        else if (pid > 0)
        {
            do
            {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            close(fd[1]);
            in = fd[0];
            cit++;
        }
    }

    for (int i = 0; cmds[i] != NULL; i++)
    {
        free(cmds[i]);
    }
    free(cmds);
    return status;
}
int help()
{
    printf(
        "The implemented commands are:\n"
        "1. cd\n2. hist\n3. env\n4. cwd\n5. chmod\n");
    return 0;
}
int hist()
{
    HIST_ENTRY **hist_ent = history_list();
    if (hist_ent)
    {
        int i = 0;
        while (hist_ent[i])
        {
            printf("%d: %s\n", i + 1, hist_ent[i]->line);
            i++;
        }
    }
    return 0;
}
int get_env()
{
    printf("env\n");
    return 0;
}
int cd(char **argv, char **env)
{

    if (argv[1] == NULL)
    {
        // env_set(env, "OLDPWD", getenv("HOME"));
        chdir(getenv("HOME"));
        return 0;
    }
    if (access(argv[1], F_OK) < 0)
    {
        fprintf(stderr, "myshell: cd: %s: ", argv[1]);
        perror(NULL);
        return -1;
    }
    // env_set(env, "OLDPWD", getenv("PWD"));
    if (chdir(argv[1]) < 0)
    {
        // env_set(env, "OLDPWD", getenv("PWD"));
        fprintf(stderr, "myshell: couldn't move to dir '%s': ", argv[1]);
        perror(NULL);
        return -1;
    }
    // env_set(env, "PWD", argv[1]);
    return 0;
}
char *remove_whitespace(const char *str)
{
    size_t start_ws = 0, end_ws = strlen(str) - 1;
    while (isspace(str[start_ws]))
        start_ws++;
    while (isspace(str[end_ws]))
        end_ws--;
    if (start_ws == 0)
        end_ws++;
    return strndup(str + start_ws, end_ws);
}
int run_command(char **argv, char **env)
{
    char *com_path;
    char *home = getenv("HOME");
    com_path = strdup(argv[0]);
    for (int i = 0; i < num_custom; i++)
    {
        if (strcmp(argv[0], custom_commands[i]) == 0)
        {
            free(com_path);
            com_path = (char *)malloc(sizeof(char) * PATH_MAX);
            sprintf(com_path, "%s/myshell/commands/bin/%s", home, argv[0]);
        }
    }
    int status;
    pid_t pid, wpid;
    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "couldn't fork: ");
        perror(NULL);
        return -1;
    }
    if (pid == 0)
    {
        if (execvpe(com_path, argv, env) < 0)
        {
            perror("myshell");
            exit(1);
        }
    }
    else if (pid > 0)
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    free(com_path);
    return status;
}
// void env_set(char **env, char *name, char *value)
// {
//     int i = 0;
//     while (*(env + i) != NULL)
//     {
//         if (strncmp(*(env + i), name, strlen(name)) == 0)
//         {
//             *(env + i) = malloc((strlen(value) + 1) * sizeof(char));
//             sprintf(*(env + i), "%s=%s", name, value);
//         }
//         i++;
//     }
// }