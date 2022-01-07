#include "util.h"

void envset(char **env, char *name, char *value)
{
    int i = 0;
    while (*(env + i) != NULL)
    {
        if (strncmp(*(env + i), name, strlen(name)) == 0)
        {
            // printf("found value: %s with the name: %s\n",*(env+i),name);
            // printf("replacing\n");
            *(env + i) = malloc((strlen(value) + 1) * sizeof(char));
            sprintf(*(env + i), "%s=%s", name, value);
            // printf("new value: %s with the name: %s\n",*(env+i),name);
        }
        i++;
    }
}

void remove_whitespace(char *str)
{
    // printf("\e[48;5;253m\e[38;5;0m%s\e[0m\n", str);
    char *rw_str = malloc(strlen(str) * sizeof(char) + 1);
    int it = 0;

    while (isspace(*(str + it)))
    {
        it++;
    }
    strcpy(rw_str, str + it);
    strcpy(str, rw_str);
    int rit = strlen(str) - 1;
    while (isspace(*(str + rit)))
    {
        rit--;
    }

    strncpy(rw_str, str, rit + 1);
    rw_str[rit + 1] = '\0';
    strcpy(str, rw_str);
}
int count_args(const char *line, char *delim)
{
    int argc = 0;
    if (strtok(strdup(line), delim) != NULL)
        argc++;
    else
        return 0;
    while (strtok(NULL, delim) != NULL)
        argc++;
    return argc;
}
void parse_args(char *line, char *args[], int argc)
{
    char line_copy2[strlen(line) + 1];
    strcpy(line_copy2, line);

    args[argc] = malloc(sizeof(char));
    args[argc] = NULL;

    char *token = strtok(line_copy2, " ");
    int i = 0;
    while (token != NULL)
    {
        remove_whitespace(token);
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        i++;
        token = strtok(NULL, " ");
    }
    free(token);
}
void parse_pipe(char *line, char *args[], int argc)
{
    char line_copy2[strlen(line) + 1];
    strcpy(line_copy2, line);

    char *token = strtok(line_copy2, "|");
    int i = 0;
    while (token != NULL)
    {
        remove_whitespace(token);
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        i++;
        token = strtok(NULL, "|");
    }
    free(token);
}

char *read_file(const char *file, long *_size, bool newline)
{
    if (access(file, F_OK) < 0)
    {
        fprintf(stderr, "cannot access '%s': No such file or directory\n", file);
        exit(1);
    }
    if (access(file, R_OK) < 0)
    {
        fprintf(stderr, "'%s': Permission denied\n", file);
        exit(1);
    }
    int fd = open(file, O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        exit(1);
    }
    struct stat st;
    if (stat(file, &st) < 0)
    {
        perror("stat");
        exit(1);
    }

    long size = st.st_size;
    *_size = newline ? size + 1 : size;
    char *content = (char *)malloc((size + 1) * sizeof(char));
    int rerr = read(fd, content, size);
    if (rerr < 0)
    {
        perror("read");
        exit(1);
    }
    if (newline)
    {
        content[size] = '\n';
        content[size + 1] = '\0';
    } else content[size] = '\0';
    if (close(fd) < 0)
    {
        perror("close");
    }
    return content;
}

char **get_lines(const char *file, long size, int line_size[])
{
    int lines = count_lines(file, size, NULL) + 1;
    char **line = (char **)malloc(lines * sizeof(char *));
    int j = 0;
    long last = 0;
    for (long i = 0; i < size; i++)
    {
        if (file[i] == '\n')
        {
            line[j] = malloc(sizeof(char) * (i - last + 2));
            memcpy(line[j++], file + last, i - last + 1);
            line[j - 1][i - last + 2] = '\0';
            line_size[j-1] = i - last + 1;
            last = i + 1;
        }
    }

    return line;
}
int count_lines(const char *file, long size, bool *binary)
{
    int lines = 0;
    for (long i = 0; i < size - 1; i++)
    {
        if (file[i] == '\n')
        {
            lines++;
        }
        if (binary != NULL)
        {
            if (!isascii(file[i]))
                *binary = true;
        }
    }
    return lines;
}