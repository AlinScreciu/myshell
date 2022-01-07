#include "../../util.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

char *getfile(const char *path, long *size);
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
            if (!(memcmp(path1, path2, size1 * sizeof(char)) == 0))
            {

                printf("Files %s and %s differ\n", path1, path2);
            }
        }
    }
    if (binary && !text && !brief)
    {
        if (size1 != size2)
        {
            printf("Binary files %s and %s differ\n", path1, path2);
        }
        else
        {
            if (!(memcmp(path1, path2, size1 * sizeof(char)) == 0))
            {

                printf("Binary files %s and %s differ\n", path1, path2);
            }
        }
    }

    printf("normal case\n");





    



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
        char *file = read_file(path, size, true);
        return file;
    }
    else
    {
        int c;
    size_t p4kB = 4096, i = 0;
    void *newPtr = NULL;
    char *ptrString = malloc(p4kB * sizeof (char));

    while (ptrString != NULL && (c = getchar()) != '\n' && c != EOF)
    {
        if (i == p4kB * sizeof (char))
        {
            p4kB += 4096;
            if ((newPtr = realloc(ptrString, p4kB * sizeof (char))) != NULL)
                ptrString = (char*) newPtr;
            else
            {
                free(ptrString);
                return NULL;
            }
        }
        ptrString[i++] = c;
    }

    if (ptrString != NULL)
    {
        ptrString[i] = '\0';
        ptrString = realloc(ptrString, strlen(ptrString) + 1);
    } 
    else return NULL;

    ptrString = realloc(ptrString, strlen(ptrString) + 1);
    ptrString[i++] = '\n';
    ptrString[i] = '\0';
    *size = i;
    return ptrString;
    }
}
