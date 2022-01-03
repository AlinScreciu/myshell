// Online C compiler to run C program online
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int count_lines(char *fileContent, size_t size)
{
    int nrOfLines = 1;
    for (size_t i = 0; i < size; i++)
    {
        if (fileContent[i] == '\n')
        {
            nrOfLines++;
           
        }
    }
    return nrOfLines;
}
int main() {
    char* file = strdup("This\nis\na\ntest!");
    size_t size = strlen(file);
    int lines = count_lines(file, size);
    char **line = (char **)calloc(lines + 1, sizeof(char *));
    line[lines] = NULL;

    int sizeofLine[lines];
    for (int i = 0; i < lines; i++)
        sizeofLine[i] = 0;

    int li = 0;
    for (size_t i = 0; i < size; i++)
    {
        sizeofLine[li]++;
        if (file[i] == '\n')
            li++;
    }
   
    for (int i = 0; i < lines; i++)
        line[i] = (char*) calloc(sizeofLine[i] + 1, sizeof(char));

    int file_it=0;
    for (int i = 0; i < lines; i++)
        for (int j = 0; j < sizeofLine[i]; j++)
            line[i][j] = file[file_it++];

        

    
    for (int i = 0; i < lines; i++)
        printf("%s",line[i]);

    return 0;
}