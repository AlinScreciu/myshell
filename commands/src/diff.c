#include "../../util.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

char* getfile(const char* path, long* size)
{   char* file = read_file(path, size);
    return file;
}
int main(int argc, char *argv[])
{
    const char missing[] = "diff: missing operand after '%s'\ndiff: Try 'diff --help' for more information.\n";
    if (argc < 2)
    {
        fprintf(stderr, missing, argv[0]);
        exit(1);
    }
    bool brief_flag = false, as_text = false, binary = false;

    int files = 0;
    char *file_path[2];
    char *last_missing;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-q") == 0)
            {
                brief_flag = true;
                last_missing = strdup(argv[i]);
            }
            else if (strcmp(argv[i], "-a") == 0)
            {
                as_text = true;
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
    free(last_missing);
    long size1;
    char *file1 = getfile(file_path[0], &size1);
    int lines_file1 = count_lines(file1, size1, &binary) + 1;
    int lines1_size[lines_file1];
    char** line1 = malloc(lines_file1*sizeof(char*));
    line1 = get_lines( file1, size1, lines1_size);
    for (int i = 0; i < lines_file1; i++)
    {
        printf("%d: %s", i, line1[i]);
    }

    free(file1); 
    free(file_path[0]);
    free(file_path[1]);
    return 0;
}