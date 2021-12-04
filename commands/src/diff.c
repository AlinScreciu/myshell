#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//#include "../../util.c"
void parse_file(char *, char **, int);
char *read_file(char *, int);
void compare_files(char *, char *);
int main(int argc, char **argv)
{
    int files = 0;
    int a_flag = 0;
    int q_flag = 0;
    int opt = 0;
    for (int i = 1; i < argc; i++)
        if (argv[i][0] != '-')
            files++;
    if (files < 2)
    {
        fprintf(stderr, "Not enough files specified!\n");
        return -1;
    }
    // if enough files are specified, find them;
    char **file_paths = malloc(sizeof(char *) * files);
    int j = 0;
    for (int i = 1; i < argc; i++)
    {

        if (argv[i][0] != '-')
        {
            file_paths[j] = strdup(argv[i]);
            j++;
        }
    }
    while ((opt = getopt(argc, argv, "aq")) != -1)
    {

        switch (opt)
        {
        case 'a':
            a_flag = 1;
            break;
        case 'q':
            q_flag = 1;
            break;
        }
    }

    char *file1 = read_file(file_paths[0], a_flag);
    char *file2 = read_file(file_paths[1], a_flag);

    if (q_flag)
    {
        printf("whatup %d\n",strcmp(file1,file2));
        if (strcmp(file1, file2) == 0)
            return 0;
        else
        {   
            printf("Files %s and %s differ\n", file_paths[0], file_paths[1]);
            return 0;
        }
    }
    if (!q_flag)
    {
        compare_files(file1, file2);
    }
    free(file1);
    free(file2);
}
// void parse_file(char *line, char *args[], int argc)
// {
//     char line_copy2[strlen(line) + 1];
//     strcpy(line_copy2, line);

//     args[argc] = malloc(sizeof(char));
//     args[argc] = NULL;

//     char *token = strtok(line_copy2, "\n");
//     int i = 0;
//     while (token != NULL)
//     {
//         remove_whitespace(token);
//         args[i] = malloc(strlen(token) + 1);
//         strcpy(args[i], token);
//         i++;
//         token = strtok(NULL, "\n");
//     }
//     free(token);
// }
char *read_file(char *path, int a_flag)
{
    char *buffer;
    if (a_flag)
    {
        long s;
        FILE *fh = fopen(path, "r");
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
        printf("%s\n", buffer);
    }
    else
    {

    }
    
    return buffer;
}
int count_lines(char* file)
{
    int lines = 0;
    for (size_t i = 0; i < strlen(file) + 1; i++)
    {
        if (file[i]=='\n') lines++ ;
    }
    return lines;
}
void compare_files(char *file1 , char *file2)
{
    
}