#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
//#include "../../util.c"
void parse_file(char *, char **, int);
char *read_file(char *, int);
void compare_files(char *, char *);
int main(int argc, char **argv)
{
    int opt;
    bool a_flag = false, q_flag = false;

    while ((opt = getopt(argc, argv, "aq")) != -1)
    {
        switch (opt)
        {
        case 'a':
            a_flag = true;
            break;
        case 'q':
            q_flag = true;
            break;
        }
    }

    int files = argc - 1 - (a_flag ? 1 : 0) - (q_flag ? 1 : 0);
    if (files < 2)
    {
        fprintf(stderr, "Not enough files given!\n");
        exit(EXIT_FAILURE);
    }

    char** file_paths = (char**) malloc(files * sizeof(char*) + 1);
    file_paths[files] = NULL;
    
    int j = 0;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            file_paths[j] = strdup(argv[i]);
            j++;
        }
    }

    for (int i = 0 ; i < files ; i++)
    {
        struct stat temp;
        if (stat(file_paths[i], &temp) != 0) 
        {
            fprintf(stderr, "File %s doesn't exist or it's not accessible!\n", file_paths[i]);
            exit(EXIT_FAILURE);
        }
    }

    char** file = (char**) malloc(files * sizeof(char*) + 1);
    file[files] = NULL;

    for (int i = 0; i < files; i++)
    {
        file[i] = read_file(file_paths[i], true);
    }
    if (q_flag) 
    {
        if(strcmp(file[0],file[1]) != 0) printf("Files %s and %s differ\n", file_paths[0],file_paths[1]);
        else exit(EXIT_SUCCESS);
    }
    for(int i = 0 ; i < files ; i++)
    {
        printf("%s\n",file[i]);
    }
    return 0;
}

char *read_file(char *path, int a_flag)
{
    char *buffer;
    if (a_flag)
    {
        long s;
        FILE *fh = fopen(path, "rb");
        if (fh != NULL)
        {
            fseek(fh, 0L, SEEK_END);
            s = ftell(fh);
            rewind(fh);
            buffer = malloc(s + 1);
            if (buffer != NULL)
            {
                fread(buffer, s, 1, fh);
                // we can now close the file
                fclose(fh);
                fh = NULL;
            }
            if (fh != NULL)
                fclose(fh);
        }
        buffer[s] = '\0';
    }
    return buffer;
}
int count_lines(char *file)
{
    int lines = 0;
    for (size_t i = 0; i < strlen(file) + 1; i++)
    {
        if (file[i] == '\n')
            lines++;
    }
    return lines;
}
void compare_files(char *file1, char *file2)
{
    printf("Comparing...\n");
}