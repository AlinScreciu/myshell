#include "../../util.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

char *getfile(const char *path, long *size);
void handle_op(int argc, char **argv, bool *brief, bool *text, char *p1, char *p2);
int main(int argc, char *argv[])
{
    bool brief = false, text = false;
    char *path1, *path2;
    const char missing[] = "diff: missing operand after '%s'\ndiff: Try 'diff --help' for more information.\n";
    if (argc < 2)
    {
        fprintf(stderr, missing, argv[0]);
        exit(1);
    }
    int files = 0;
    char *file_path[2];
    char *last_missing = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-q") == 0)
            {
                brief = true;
                last_missing = strdup(argv[i]);
            }
            else if (strcmp(argv[i], "-a") == 0)
            {
                text = true;
                last_missing = strdup(argv[i]);
            }
            else if (strcmp(argv[i], "-") == 0)
                file_path[files++] = strdup(argv[i]);
            else
            {
                fprintf(stderr, "diff: invalid options -- '%c'\n", argv[i][1]);
                exit(1);
            }
        }
        else if (argv[i][0] != '-')
            file_path[files++] = strdup(argv[i]);
    }
    if (files == 0)
    {
        fprintf(stderr, missing, last_missing);
        exit(1);
    }
    if (files == 1)
    {
        fprintf(stderr, missing, file_path[0]);
        exit(1);
    }
    path1 = strdup(file_path[0]);
    path2 = strdup(file_path[1]);
    free(file_path[0]);
    free(file_path[1]);
    if (last_missing != NULL)
        free(last_missing);
    long size1;
    bool binary = false;
    char *file1 = getfile(path1, &size1);
    int lines_file1 = count_lines(file1, size1, &binary) + 1;
    int lines1_size[lines_file1];
    char **line1 = get_lines(file1, size1, lines1_size);
    free(file1);
    lines1_size[lines_file1 - 1]--;

    long size2;
    char *file2 = getfile(path2, &size2);
    int lines_file2 = count_lines(file2, size2, &binary) + 1;
    int lines2_size[lines_file2];
    char **line2 = get_lines(file2, size2, lines2_size);
    free(file2);
    lines2_size[lines_file2 - 1]--;
    if (brief)
    {
        if (size1 != size2)
        {
            printf("Files %s and %s differ\n", path1, path2);
        }
        else
        {
            if (!memcmp(path1, path2, size1 * sizeof(char)) == 0)
            {

                printf("Files %s and %s differ\n", path1, path2);
            }
        }
    }
    for (int i = 0; i < lines_file1; i++)
    {
        free(line1[i]);
    }
    for (int i = 0; i < lines_file2; i++)
    {
        free(line2[i]);
    }
    free(path1);
    free(path2);
    return 0;
}
char *getfile(const char *path, long *size)
{
    if (strcmp(path, "-") != 0)
    {
        char *file = read_file(path, size);
        return file;
    }
    else
    {
        int i = 0;
        char c;
        char *s = malloc(sizeof(char));
        while ((c = getchar()) != EOF)
        {
            s[i++] = c;
            if (realloc(s, sizeof(char) * i) == NULL)
                exit(1);
        }
        if (realloc(s, sizeof(char) * (i + 2)) == NULL)
            exit(1);
        s[i++] = '\n';
        s[i] = '\0';
        *size = i;
        return s;
    }
}
void handle_op(int argc, char **argv, bool *brief, bool *text, char *p1, char *p2)
{
}
